/**
 * @file ReDroidController.hpp
 * @brief Core ReDroid container controller for Windows
 * @author VirtualPhonePro Team
 * @version 1.0.0
 */

#ifndef VIRTUALPHONEPRO_REDROID_CONTROLLER_HPP
#define VIRTUALPHONEPRO_REDROID_CONTROLLER_HPP

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QProcess>
#include <QTimer>
#include <QVariantMap>
#include <QUuid>

#include "DeviceProfile.hpp"

namespace VirtualPhonePro {

/**
 * @brief Instance status enumeration
 */
enum class InstanceStatus {
    Unknown,
    Created,
    Starting,
    Running,
    Stopping,
    Stopped,
    Error,
    Removing
};

/**
 * @brief Container info structure
 */
struct ContainerInfo {
    QString containerId;
    QString instanceId;
    QString containerName;
    InstanceStatus status{InstanceStatus::Unknown};
    int adbPort{0};
    QString wslIp;
    quint64 memoryUsage{0};
    quint64 cpuUsage{0};
    QDateTime startedAt;
    DeviceProfile profile;
};

/**
 * @brief Sensor data structure
 */
struct SensorData {
    double latitude{0.0};
    double longitude{0.0};
    double altitude{0.0};
    double accuracy{1.0};
    double speed{0.0};
    double bearing{0.0};
    double xAccel{0.0};
    double yAccel{0.0};
    double zAccel{0.0};
    double xGyro{0.0};
    double yGyro{0.0};
    double zGyro{0.0};
};

/**
 * @brief ReDroidController - Main controller for ReDroid container management
 */
class ReDroidController : public QObject {
    Q_OBJECT

public:
    explicit ReDroidController(QObject* parent = nullptr);
    virtual ~ReDroidController();

    void setWslDistro(const QString& distro);
    void setDockerImage(const QString& image);
    void setBaseAdbPort(int basePort);
    void setInstancesPath(const QString& path);

    Q_INVOKABLE bool startInstance(const QString& instanceId, const DeviceProfile& profile);
    Q_INVOKABLE bool stopInstance(const QString& instanceId, bool force = false);
    Q_INVOKABLE bool removeInstance(const QString& instanceId);
    Q_INVOKABLE bool restartInstance(const QString& instanceId);

    Q_INVOKABLE bool sendGpsData(const QString& instanceId,
                                  double latitude, double longitude,
                                  double altitude = 0.0, double accuracy = 1.0);
    Q_INVOKABLE bool sendAccelerometerData(const QString& instanceId,
                                           double x, double y, double z);
    Q_INVOKABLE bool sendSensorData(const QString& instanceId, const SensorData& data);

    Q_INVOKABLE QString executeShell(const QString& instanceId,
                                      const QString& command, int timeoutMs = 30000);
    Q_INVOKABLE bool installApk(const QString& instanceId, const QString& apkPath);
    Q_INVOKABLE bool uninstallPackage(const QString& instanceId, const QString& packageName);
    Q_INVOKABLE bool takeScreenshot(const QString& instanceId, const QString& outputPath);

    Q_INVOKABLE InstanceStatus getInstanceStatus(const QString& instanceId) const;
    Q_INVOKABLE QStringList getRunningInstances() const;
    Q_INVOKABLE ContainerInfo getContainerInfo(const QString& instanceId) const;
    Q_INVOKABLE bool isDockerAvailable() const;
    Q_INVOKABLE bool isWslAvailable() const;
    Q_INVOKABLE bool waitForInstanceReady(const QString& instanceId, int timeoutMs = 60000);

signals:
    void instanceStatusChanged(const QString& instanceId, InstanceStatus status);
    void instanceStarted(const QString& instanceId, int adbPort);
    void instanceStopped(const QString& instanceId);
    void instanceError(const QString& instanceId, const QString& error);
    void adbConnected(const QString& instanceId);
    void adbDisconnected(const QString& instanceId);
    void screenshotCaptured(const QString& instanceId, const QString& path);
    void dockerAvailabilityChanged(bool available);
    void wslAvailabilityChanged(bool available);

private slots:
    void onAdbProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onDockerProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void checkInstanceStatus();

private:
    int findAvailableAdbPort();
    QString windowsToWslPath(const QString& windowsPath);
    QString wslToWindowsPath(const QString& wslPath);
    QString getAdbPath();
    QString getDockerPath();
    QPair<int, QString> executeCommand(const QString& program, const QStringList& args, int timeoutMs = 30000);
    QString executeWslCommand(const QString& command, int timeoutMs = 30000);
    bool createInstanceConfig(const QString& instanceId, const DeviceProfile& profile);
    ContainerInfo parseContainerInfo(const QString& containerId);
    void updateInstanceStatus(const QString& instanceId, InstanceStatus status);

private:
    QString m_wslDistro{"Ubuntu"};
    QString m_dockerImage{"virtualphonepro/redroid-spoofed:1.0.0"};
    int m_baseAdbPort{5555};
    QString m_instancesPath;

    QMap<QString, ContainerInfo> m_containers;
    QMap<QString, QProcess*> m_adbProcesses;
    QMap<QString, QProcess*> m_dockerProcesses;

    QTimer* m_statusCheckTimer;
    bool m_dockerAvailable{false};
    bool m_wslAvailable{false};
};

}
#endif
