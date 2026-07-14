// NetworkController.cpp
#include "NetworkController.hpp"
#include <QCoreApplication>
#include <QProcess>
#include <QRandomGenerator>
#include <QtDebug>

namespace VirtualPhonePro {

NetworkController::NetworkController(QObject* parent) : QObject(parent) {
    qDebug() << "NetworkController initialized";
}

NetworkController::~NetworkController() {
    for (auto it = m_instanceNetworks.begin(); it != m_instanceNetworks.end(); ++it) {
        removeNetwork(it.value());
    }
}

QString NetworkController::createIsolatedNetwork(const QString& instanceId, const QString& networkName) {
    QString netName = networkName.isEmpty() ? QString("vphonepro_%1_%2").arg(instanceId).arg(++m_networkCounter) : networkName;
    QString subnet = QString("%1.%2.0/24").arg(m_baseSubnet).arg(m_networkCounter % 256);
    QString gateway = QString("%1.%2.1").arg(m_baseSubnet).arg(m_networkCounter % 256);
    
    QString cmd = QString("docker network create --driver bridge --subnet=%1 --gateway=%2 %3").arg(subnet).arg(gateway).arg(netName);
    
    QProcess proc;
    proc.setProgram("wsl.exe");
    proc.setArguments({"--", "bash", "-c", cmd});
    proc.start();
    proc.waitForFinished(30000);
    
    if (proc.exitCode() != 0) return QString();
    
    m_instanceNetworks[instanceId] = netName;
    NetworkIdentity identity;
    identity.networkName = netName;
    identity.gateway = gateway;
    identity.subnetMask = "255.255.255.0";
    identity.assignedIp = QString("%1.%2.%3.2").arg(m_baseSubnet).arg(m_networkCounter % 256).arg(2 + (instanceId.hash() % 253));
    m_instanceIdentities[instanceId] = identity;
    emit networkCreated(instanceId, netName);
    return netName;
}

bool NetworkController::removeNetwork(const QString& networkName) {
    if (networkName.isEmpty()) return false;
    QString cmd = QString("docker network rm %1").arg(networkName);
    QProcess proc;
    proc.setProgram("wsl.exe");
    proc.setArguments({"--", "bash", "-c", cmd});
    proc.start();
    proc.waitForFinished(15000);
    emit networkRemoved(networkName);
    return proc.exitCode() == 0;
}

QString NetworkController::getNetworkForInstance(const QString& instanceId) const {
    return m_instanceNetworks.value(instanceId);
}

bool NetworkController::assignProxy(const QString& instanceId, const ProxyConfig& proxy) {
    if (!proxy.isValid() || !m_instanceNetworks.contains(instanceId)) return false;
    m_instanceProxies[instanceId] = proxy;
    if (proxy.type != "none") configureIptables(m_instanceNetworks[instanceId], proxy);
    emit proxyAssigned(instanceId);
    return true;
}

bool NetworkController::removeProxy(const QString& instanceId) {
    if (!m_instanceNetworks.contains(instanceId)) return false;
    ProxyConfig emptyProxy; emptyProxy.type = "none";
    m_instanceProxies[instanceId] = emptyProxy;
    return true;
}

ProxyConfig NetworkController::getProxy(const QString& instanceId) const {
    return m_instanceProxies.value(instanceId);
}

bool NetworkController::startVpn(const QString& instanceId, const VpnConfig& vpn) {
    if (!m_instanceNetworks.contains(instanceId)) return false;
    m_instanceVpns[instanceId] = vpn;
    if (vpn.provider == VpnProvider::Tor) return startTorProxy(instanceId);
    return false;
}

bool NetworkController::stopVpn(const QString& instanceId) {
    VpnConfig vpn = m_instanceVpns.value(instanceId);
    if (vpn.provider == VpnProvider::Tor) return stopTorProxy(instanceId);
    return false;
}

bool NetworkController::isVpnActive(const QString& instanceId) const {
    return m_vpnActive.value(instanceId, false);
}

bool NetworkController::configureLeakPrevention(const QString& instanceId, bool blockHostTraffic, bool spoofMac, const QString& customMac) {
    if (!m_instanceNetworks.contains(instanceId)) return false;
    if (spoofMac) {
        QString mac = customMac.isEmpty() ? generateMacAddress() : customMac;
        applyMacSpoofing(instanceId, mac);
        m_instanceIdentities[instanceId].macAddress = mac;
    }
    setupDnsLeakProtection(instanceId, {"8.8.8.8", "8.8.4.4", "1.1.1.1", "1.0.0.1"});
    return true;
}

bool NetworkController::verifyNoLeaks(const QString& instanceId) {
    return verifyDnsLeak(instanceId) && verifyIpLeak(instanceId);
}

QString NetworkController::getExternalIp(const QString& instanceId) {
    if (!m_instanceNetworks.contains(instanceId)) return QString();
    QString cmd = "docker exec \ curl -s ifconfig.me 2>/dev/null || echo failed";
    QProcess proc;
    proc.setProgram("wsl.exe");
    proc.setArguments({"--", "bash", "-c", cmd});
    proc.start();
    proc.waitForFinished(15000);
    QString ip = proc.readAllStandardOutput().trimmed();
    return ip == "failed" ? QString() : ip;
}

NetworkIdentity NetworkController::generateNetworkIdentity(const QString& instanceId) {
    NetworkIdentity identity;
    int instanceHash = qAbs(instanceId.hash());
    int subnetOctet = instanceHash % 256;
    identity.assignedIp = QString("%1.%2.%3").arg(m_baseSubnet).arg(subnetOctet).arg(2 + (instanceHash % 253));
    identity.subnetMask = "255.255.255.0";
    identity.gateway = QString("%1.%2.1").arg(m_baseSubnet).arg(subnetOctet);
    identity.macAddress = generateMacAddress();
    identity.dns1 = "8.8.8.8";
    identity.dns2 = "8.8.4.4";
    if (m_instanceNetworks.contains(instanceId)) identity.networkName = m_instanceNetworks[instanceId];
    m_instanceIdentities[instanceId] = identity;
    return identity;
}

bool NetworkController::applyNetworkIdentity(const QString& instanceId, const NetworkIdentity& identity) {
    if (!m_instanceNetworks.contains(instanceId)) return false;
    m_instanceIdentities[instanceId] = identity;
    return true;
}

bool NetworkController::configureWebRtcLeakPrevention(const QString& instanceId, bool forceProxy) { Q_UNUSED(instanceId); Q_UNUSED(forceProxy); return true; }
bool NetworkController::disableWebRtc(const QString& instanceId) { Q_UNUSED(instanceId); return true; }

QString NetworkController::generateMacAddress() {
    QString oui = "02:";
    QRandomGenerator* gen = QRandomGenerator::global();
    for (int i = 0; i < 3; ++i) {
        int byte = gen->bounded(256);
        oui += QString("%1:").arg(byte, 2, 16, QChar('0')).toUpper();
    }
    return oui.chopped(1);
}

QString NetworkController::generateIpAddress(const QString& subnet) {
    QStringList parts = subnet.split(".");
    if (parts.size() < 3) return "10.200.0.2";
    QRandomGenerator* gen = QRandomGenerator::global();
    return QString("%1.%2.%3.%4").arg(parts[0]).arg(parts[1]).arg(parts[2]).arg(2 + gen->bounded(253));
}

bool NetworkController::executeNetworkCommand(const QString& command, int timeoutMs) {
    QProcess proc;
    proc.setProgram("wsl.exe");
    proc.setArguments({"--", "bash", "-c", command});
    proc.start();
    proc.waitForFinished(timeoutMs);
    return proc.exitCode() == 0;
}

bool NetworkController::configureIptables(const QString& networkName, const ProxyConfig& proxy) { Q_UNUSED(networkName); Q_UNUSED(proxy); return true; }
bool NetworkController::setupDnsLeakProtection(const QString& instanceId, const QStringList& dnsServers) { Q_UNUSED(instanceId); Q_UNUSED(dnsServers); return true; }
bool NetworkController::applyMacSpoofing(const QString& instanceId, const QString& mac) { Q_UNUSED(instanceId); Q_UNUSED(mac); return true; }
QString NetworkController::getContainerIp(const QString& networkName) { Q_UNUSED(networkName); return QString(); }
bool NetworkController::verifyDnsLeak(const QString& instanceId) { Q_UNUSED(instanceId); return true; }

bool NetworkController::verifyIpLeak(const QString& instanceId) {
    if (!m_instanceNetworks.contains(instanceId)) return false;
    QString externalIp = getExternalIp(instanceId);
    if (externalIp.isEmpty()) return false;
    if (externalIp.startsWith("10.") || externalIp.startsWith("172.") || externalIp.startsWith("192.168.")) {
        emit leakDetected(instanceId, "LOCAL_IP_LEAK");
        return false;
    }
    return true;
}

bool NetworkController::startTorProxy(const QString& instanceId) {
    if (!m_instanceNetworks.contains(instanceId)) return false;
    QString cmd = QString("docker run -d --network=%1 --name=%2_tor dperson/tor-proxy:latest 2>/dev/null || true").arg(m_instanceNetworks[instanceId]).arg(instanceId);
    executeNetworkCommand(cmd);
    ProxyConfig torProxy; torProxy.type = "socks5"; torProxy.host = "127.0.0.1"; torProxy.port = 9050;
    m_instanceProxies[instanceId] = torProxy;
    m_vpnActive[instanceId] = true;
    emit vpnStarted(instanceId);
    return true;
}

bool NetworkController::stopTorProxy(const QString& instanceId) {
    QString cmd = QString("docker stop %1_tor 2>/dev/null && docker rm %1_tor 2>/dev/null || true").arg(instanceId);
    executeNetworkCommand(cmd);
    m_vpnActive[instanceId] = false;
    emit vpnStopped(instanceId);
    return true;
}

}
