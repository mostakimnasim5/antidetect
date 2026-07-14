// ReDroidController.cpp - Core controller for ReDroid instances
// This file provides container lifecycle management via WSL2 and ADB

#include "ReDroidController.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QThread>
#include <QtDebug>

namespace VirtualPhonePro {

ReDroidController::ReDroidController(QObject* parent)
    : QObject(parent)
    , m_statusCheckTimer(new QTimer(this))
{
    m_instancesPath = QCoreApplication::applicationDirPath() + QDir::separator() + "instances";
    QDir().mkpath(m_instancesPath);
    connect(m_statusCheckTimer, &QTimer::timeout, this, &ReDroidController::checkInstanceStatus);
    m_statusCheckTimer->start(5000);
    checkDockerAvailability();
    checkWslAvailability();
}

ReDroidController::~ReDroidController() {
    for (auto it = m_containers.begin(); it != m_containers.end(); ++it) {
        if (it.value().status == InstanceStatus::Running) stopInstance(it.key(), true);
    }
    qDeleteAll(m_adbProcesses);
    qDeleteAll(m_dockerProcesses);
}

void ReDroidController::setWslDistro(const QString& d) { m_wslDistro = d; }
void ReDroidController::setDockerImage(const QString& i) { m_dockerImage = i; }
void ReDroidController::setBaseAdbPort(int p) { m_baseAdbPort = p; }
void ReDroidController::setInstancesPath(const QString& p) { m_instancesPath = p; QDir().mkpath(m_instancesPath); }

bool ReDroidController::startInstance(const QString& instanceId, const DeviceProfile& profile) {
    if (!m_dockerAvailable) { emit instanceError(instanceId, "Docker unavailable"); return false; }
    if (!profile.isValid()) { emit instanceError(instanceId, profile.getValidationError()); return false; }
    updateInstanceStatus(instanceId, InstanceStatus::Starting);
    if (!createInstanceConfig(instanceId, profile)) { updateInstanceStatus(instanceId, InstanceStatus::Error); return false; }
    int port = findAvailableAdbPort();
    QStringList args = {"run", "-d", "--rm", "--name", instanceId, "--privileged", "-p", QString("%1:5555").arg(port), "-m", QString("%1M").arg(profile.resources.memoryLimitMb), m_dockerImage};
    QMap<QString,QString> env = profile.toEnvironmentMap();
    for (auto it = env.begin(); it != env.end(); ++it) args << "-e" << QString("%1=%2").arg(it.key()).arg(it.value());
    args << "-e" << QString("ANDROIDBOOT.REDROID_WIDTH=%1").arg(profile.display.width);
    args << "-e" << QString("ANDROIDBOOT.REDROID_HEIGHT=%1").arg(profile.display.height);
    QString dockerCmd = "docker " + args.join(" ");
    QPair<int,QString> result = executeWslCommand(dockerCmd, 120000);
    if (result.first != 0) { emit instanceError(instanceId, result.second); updateInstanceStatus(instanceId, InstanceStatus::Error); return false; }
    ContainerInfo info; info.instanceId = instanceId; info.containerId = result.second.trimmed();
    info.adbPort = port; info.status = InstanceStatus::Starting; info.startedAt = QDateTime::currentDateTime(); info.profile = profile;
    m_containers[instanceId] = info;
    if (waitForInstanceReady(instanceId, 60000)) { updateInstanceStatus(instanceId, InstanceStatus::Running); emit instanceStarted(instanceId, port); emit adbConnected(instanceId); }
    return true;
}

bool ReDroidController::stopInstance(const QString& instanceId, bool force) {
    if (!m_containers.contains(instanceId)) return false;
    updateInstanceStatus(instanceId, InstanceStatus::Stopping);
    executeCommand(getAdbPath(), {"disconnect", QString("localhost:%1").arg(m_containers[instanceId].adbPort)}, 5000);
    QPair<int,QString> r = executeWslCommand(QString("docker %1stop %2").arg(force ? "-t 0 " : "").arg(instanceId), 30000);
    if (r.first != 0) { emit instanceError(instanceId, r.second); updateInstanceStatus(instanceId, InstanceStatus::Error); return false; }
    updateInstanceStatus(instanceId, InstanceStatus::Stopped);
    emit instanceStopped(instanceId); emit adbDisconnected(instanceId);
    return true;
}

bool ReDroidController::removeInstance(const QString& instanceId) {
    if (m_containers.contains(instanceId) && m_containers[instanceId].status == InstanceStatus::Running) stopInstance(instanceId, true);
    updateInstanceStatus(instanceId, InstanceStatus::Removing);
    QPair<int,QString> r = executeWslCommand(QString("docker rm -f %1").arg(instanceId), 30000);
    if (r.first == 0) m_containers.remove(instanceId);
    return r.first == 0;
}

bool ReDroidController::restartInstance(const QString& instanceId) {
    if (!m_containers.contains(instanceId)) return false;
    DeviceProfile p = m_containers[instanceId].profile;
    if (!stopInstance(instanceId)) return false;
    QThread::msleep(1000);
    return startInstance(instanceId, p);
}

bool ReDroidController::sendGpsData(const QString& instanceId, double lat, double lon, double alt, double acc) {
    if (!m_containers.contains(instanceId) || m_containers[instanceId].status != InstanceStatus::Running) return false;
    QStringList args = {"-s", QString("localhost:%1").arg(m_containers[instanceId].adbPort), "shell", "am", "broadcast", "-a", "android.location.GPS_FIX_ACTION", "--ei", "latitude", QString::number((int)(lat*1e6)), "--ei", "longitude", QString::number((int)(lon*1e6))};
    return executeCommand(getAdbPath(), args, 10000).first == 0;
}

bool ReDroidController::sendAccelerometerData(const QString&, double, double, double) { return true; }
bool ReDroidController::sendSensorData(const QString& id, const SensorData& d) { return sendGpsData(id, d.latitude, d.longitude, d.altitude, d.accuracy); }

QString ReDroidController::executeShell(const QString& instanceId, const QString& cmd, int timeout) {
    if (!m_containers.contains(instanceId)) return QString();
    return executeCommand(getAdbPath(), {"-s", QString("localhost:%1").arg(m_containers[instanceId].adbPort), "shell", cmd}, timeout).second;
}

bool ReDroidController::installApk(const QString& instanceId, const QString& apkPath) {
    if (!m_containers.contains(instanceId)) return false;
    return executeCommand(getAdbPath(), {"-s", QString("localhost:%1").arg(m_containers[instanceId].adbPort), "install", "-r", apkPath}, 120000).first == 0;
}

bool ReDroidController::uninstallPackage(const QString& instanceId, const QString& pkg) {
    if (!m_containers.contains(instanceId)) return false;
    return executeCommand(getAdbPath(), {"-s", QString("localhost:%1").arg(m_containers[instanceId].adbPort), "uninstall", pkg}, 60000).first == 0;
}

bool ReDroidController::takeScreenshot(const QString& instanceId, const QString& outPath) {
    if (!m_containers.contains(instanceId)) return false;
    QProcess p; p.setProgram(getAdbPath()); p.setArguments({"-s", QString("localhost:%1").arg(m_containers[instanceId].adbPort), "exec-out", "screencap", "-p"});
    p.start(); p.waitForFinished(30000);
    if (p.exitCode() != 0) return false;
    QFile f(outPath); if (f.open(QIODevice::WriteOnly)) { f.write(p.readAll()); f.close(); return true; }
    return false;
}

InstanceStatus ReDroidController::getInstanceStatus(const QString& id) const { return m_containers.value(id).status; }
QStringList ReDroidController::getRunningInstances() const { QStringList l; for (auto &c : m_containers) if (c.status == InstanceStatus::Running) l.append(c.instanceId); return l; }
ContainerInfo ReDroidController::getContainerInfo(const QString& id) const { return m_containers.value(id); }
bool ReDroidController::isDockerAvailable() const { return m_dockerAvailable; }
bool ReDroidController::isWslAvailable() const { return m_wslAvailable; }

bool ReDroidController::waitForInstanceReady(const QString& instanceId, int timeout) {
    if (!m_containers.contains(instanceId)) return false;
    int port = m_containers[instanceId].adbPort;
    executeCommand(getAdbPath(), {"connect", QString("localhost:%1").arg(port)}, 10000);
    for (int el = 0; el < timeout; el += 1000) {
        QPair<int,QString> r = executeCommand(getAdbPath(), {"devices"}, 5000);
        if (r.first == 0 && r.second.contains(QString("localhost:%1").arg(port)) && r.second.contains("device") && !r.second.contains("offline")) return true;
        QThread::msleep(1000);
    }
    return false;
}

void ReDroidController::onAdbProcessFinished(int, QProcess::ExitStatus s) { QProcess* p = qobject_cast<QProcess*>(sender()); if (p) { QString id = m_adbProcesses.key(p); if (!id.isEmpty() && s == QProcess::CrashExit) emit adbDisconnected(id); m_adbProcesses.remove(id); } }
void ReDroidController::onDockerProcessFinished(int, QProcess::ExitStatus) {}
void ReDroidController::checkInstanceStatus() {
    bool was = m_dockerAvailable; checkDockerAvailability();
    if (was != m_dockerAvailable) emit dockerAvailabilityChanged(m_dockerAvailable);
    for (auto it = m_containers.begin(); it != m_containers.end(); ++it) {
        if (it.value().status == InstanceStatus::Running || it.value().status == InstanceStatus::Starting) {
            QPair<int,QString> r = executeWslCommand(QString("docker inspect -f '{{.State.Running}}' %1").arg(it.value().containerName), 5000);
            if (r.first != 0 || r.second.trimmed() != "true") { updateInstanceStatus(it.key(), InstanceStatus::Stopped); emit instanceStopped(it.key()); emit adbDisconnected(it.key()); }
        }
    }
}
void ReDroidController::updateInstanceStatus(const QString& id, InstanceStatus s) { if (m_containers.contains(id)) m_containers[id].status = s; emit instanceStatusChanged(id, s); }

int ReDroidController::findAvailableAdbPort() {
    QList<int> used; for (auto &c : m_containers) used.append(c.adbPort);
    for (int p = m_baseAdbPort; p < m_baseAdbPort + 100; ++p) if (!used.contains(p)) return p;
    return m_baseAdbPort + 50;
}
QString ReDroidController::windowsToWslPath(const QString& p) { if (p.length() >= 2 && p[1] == ':') { QString r = p.mid(2).replace("\\", "/").replace("\", "/"); return QString("/mnt/%1%2").arg(p[0].toUpper()).arg(r); } return p; }
QString ReDroidController::wslToWindowsPath(const QString& p) { if (p.startsWith("/mnt/")) { QString r = p.mid(5); return QString("%1:\%2").arg(r[0].toUpper()).arg(r.mid(1).replace("/", "\\")); } return p; }
QString ReDroidController::getAdbPath() { QString p = QCoreApplication::applicationDirPath() + "\platform-tools\adb.exe"; return QFile::exists(p) ? p : "adb.exe"; }
QString ReDroidController::getDockerPath() { return "docker.exe"; }

QPair<int,QString> ReDroidController::executeCommand(const QString& prog, const QStringList& args, int timeout) {
    QProcess proc; proc.setProgram(prog); proc.setArguments(args);
    QEventLoop loop; QTimer t; t.setSingleShot(true);
    QObject::connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(&proc, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished), &loop, &QEventLoop::quit);
    t.start(timeout); proc.start(); loop.exec(); t.stop();
    if (!t.isActive()) { proc.kill(); proc.waitForFinished(1000); return qMakePair(-1, QString("Timeout")); }
    return qMakePair(proc.exitCode(), proc.exitCode() == 0 ? proc.readAllStandardOutput() : proc.readAllStandardError());
}

QString ReDroidController::executeWslCommand(const QString& cmd, int timeout) {
    QProcess p; p.setProgram("wsl.exe"); p.setArguments({"-d", m_wslDistro, "--", "bash", "-c", cmd});
    p.start(); p.waitForFinished(timeout);
    return p.readAllStandardOutput() + p.readAllStandardError();
}

bool ReDroidController::createInstanceConfig(const QString& id, const DeviceProfile& prof) {
    QString path = m_instancesPath + QDir::separator() + id + QDir::separator() + "config" + QDir::separator() + "device_identity.conf";
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path); if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QTextStream s(&f); s << "device_model=" << prof.model << "
" << "device_brand=" << prof.brand << "
" << "serial_number=" << prof.identity.serialNumber << "
" << "android_id=" << prof.identity.androidId << "
"; f.close();
    return true;
}

ContainerInfo ReDroidController::parseContainerInfo(const QString& id) { ContainerInfo i; i.containerId = id; return i; }
void ReDroidController::checkDockerAvailability() { m_dockerAvailable = executeWslCommand("docker version", 10000).first() == 0; }
void ReDroidController::checkWslAvailability() { m_wslAvailable = true; }

}
