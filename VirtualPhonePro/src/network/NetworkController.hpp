/**
 * @file NetworkController.hpp
 * @brief Network isolation and leak prevention for ReDroid containers
 */

#ifndef VIRTUALPHONEPRO_NETWORK_CONTROLLER_HPP
#define VIRTUALPHONEPRO_NETWORK_CONTROLLER_HPP

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QPair>
#include <QHostAddress>

namespace VirtualPhonePro {

/**
 * @brief VPN provider types
 */
enum class VpnProvider {
    None,
    WireGuard,
    OpenVPN,
    Tor,
    CustomProxy
};

/**
 * @brief Proxy configuration structure
 */
struct ProxyConfig {
    QString type{"none"};           // none, http, socks5, tor
    QString host;
    int port{0};
    QString username;
    QString password;
    QStringList excludeHosts;
    
    bool isValid() const {
        if (type == "none") return true;
        return !host.isEmpty() && port > 0 && port <= 65535;
    }
};

/**
 * @brief VPN configuration structure
 */
struct VpnConfig {
    VpnProvider provider{VpnProvider::None};
    QString configPath;             // Path to WireGuard/OpenVPN config
    QStringList allowedApps;        // Apps allowed outside VPN
    QString interfaceName;         // e.g., wg0, tun0
    bool killSwitch{true};         // Block non-VPN traffic
};

/**
 * @brief Network identity - assigned IP/MAC for container
 */
struct NetworkIdentity {
    QString assignedIp;             // e.g., 10.200.0.2
    QString subnetMask{"255.255.255.0"};
    QString gateway;
    QString dns1{"8.8.8.8"};
    QString dns2{"8.8.4.4"};
    QString macAddress;              // Spoofed MAC
    QString networkName;             // Docker network name
};

/**
 * @brief NetworkController - Manages network isolation for containers
 */
class NetworkController : public QObject {
    Q_OBJECT

public:
    explicit NetworkController(QObject* parent = nullptr);
    virtual ~NetworkController();

    // ========================================================================
    // Docker Network Management
    // ========================================================================
    
    /**
     * @brief Create isolated Docker network for instance
     * @param instanceId Unique instance identifier
     * @param networkName Optional custom network name
     * @return Network name created
     */
    Q_INVOKABLE QString createIsolatedNetwork(const QString& instanceId, 
                                               const QString& networkName = "");
    
    /**
     * @brief Remove isolated network
     * @param networkName Network to remove
     * @return true if successful
     */
    Q_INVOKABLE bool removeNetwork(const QString& networkName);
    
    /**
     * @brief Get network name for instance
     * @param instanceId Instance identifier
     * @return Network name
     */
    Q_INVOKABLE QString getNetworkForInstance(const QString& instanceId) const;

    // ========================================================================
    // Proxy Configuration
    // ========================================================================
    
    /**
     * @brief Assign proxy to container
     * @param instanceId Instance identifier
     * @param proxy Proxy configuration
     * @return true if successful
     */
    Q_INVOKABLE bool assignProxy(const QString& instanceId, const ProxyConfig& proxy);
    
    /**
     * @brief Remove proxy from container
     * @param instanceId Instance identifier
     * @return true if successful
     */
    Q_INVOKABLE bool removeProxy(const QString& instanceId);
    
    /**
     * @brief Get current proxy for instance
     * @param instanceId Instance identifier
     * @return Proxy configuration
     */
    Q_INVOKABLE ProxyConfig getProxy(const QString& instanceId) const;

    // ========================================================================
    // VPN Integration
    // ========================================================================
    
    /**
     * @brief Start VPN for container
     * @param instanceId Instance identifier
     * @param vpn VPN configuration
     * @return true if started successfully
     */
    Q_INVOKABLE bool startVpn(const QString& instanceId, const VpnConfig& vpn);
    
    /**
     * @brief Stop VPN for container
     * @param instanceId Instance identifier
     * @return true if stopped successfully
     */
    Q_INVOKABLE bool stopVpn(const QString& instanceId);
    
    /**
     * @brief Get VPN status
     * @param instanceId Instance identifier
     * @return true if VPN is active
     */
    Q_INVOKABLE bool isVpnActive(const QString& instanceId) const;

    // ========================================================================
    // Leak Prevention
    // ========================================================================
    
    /**
     * @brief Configure leak prevention for container
     * @param instanceId Instance identifier
     * @param blockHostTraffic Block all non-proxy traffic
     * @param spoofMac Spoof MAC address
     * @param customMac Custom MAC (random if empty)
     * @return true if configured successfully
     */
    Q_INVOKABLE bool configureLeakPrevention(const QString& instanceId,
                                             bool blockHostTraffic = true,
                                             bool spoofMac = true,
                                             const QString& customMac = "");
    
    /**
     * @brief Verify no leaks detected
     * @param instanceId Instance identifier
     * @return true if no leaks detected
     */
    Q_INVOKABLE bool verifyNoLeaks(const QString& instanceId);
    
    /**
     * @brief Get external IP (should go through proxy/VPN)
     * @param instanceId Instance identifier
     * @return External IP or empty string on failure
     */
    Q_INVOKABLE QString getExternalIp(const QString& instanceId);

    // ========================================================================
    // Network Identity Management
    // ========================================================================
    
    /**
     * @brief Generate new network identity for instance
     * @param instanceId Instance identifier
     * @return Generated network identity
     */
    Q_INVOKABLE NetworkIdentity generateNetworkIdentity(const QString& instanceId);
    
    /**
     * @brief Apply network identity to container
     * @param instanceId Instance identifier
     * @param identity Network identity to apply
     * @return true if applied successfully
     */
    Q_INVOKABLE bool applyNetworkIdentity(const QString& instanceId, 
                                            const NetworkIdentity& identity);

    // ========================================================================
    // WebRTC Leak Prevention
    // ========================================================================
    
    /**
     * @brief Configure WebRTC leak prevention
     * @param instanceId Instance identifier
     * @param forceProxy Force WebRTC through proxy
     * @return true if configured
     */
    Q_INVOKABLE bool configureWebRtcLeakPrevention(const QString& instanceId,
                                                    bool forceProxy = true);
    
    /**
     * @brief Disable WebRTC completely
     * @param instanceId Instance identifier
     * @return true if disabled
     */
    Q_INVOKABLE bool disableWebRtc(const QString& instanceId);

signals:
    void networkCreated(const QString& instanceId, const QString& networkName);
    void networkRemoved(const QString& networkName);
    void proxyAssigned(const QString& instanceId);
    void vpnStarted(const QString& instanceId);
    void vpnStopped(const QString& instanceId);
    void leakDetected(const QString& instanceId, const QString& leakType);
    void externalIpChanged(const QString& instanceId, const QString& oldIp, const QString& newIp);

private:
    // Internal methods
    QString generateMacAddress();
    QString generateIpAddress(const QString& subnet);
    bool executeNetworkCommand(const QString& command, int timeoutMs = 30000);
    bool configureIptables(const QString& networkName, const ProxyConfig& proxy);
    bool setupDnsLeakProtection(const QString& instanceId, const QStringList& dnsServers);
    bool applyMacSpoofing(const QString& instanceId, const QString& mac);
    QString getContainerIp(const QString& networkName);
    bool verifyDnsLeak(const QString& instanceId);
    bool verifyIpLeak(const QString& instanceId);
    bool startTorProxy(const QString& instanceId);
    bool stopTorProxy(const QString& instanceId);

private:
    QMap<QString, QString> m_instanceNetworks;      // instanceId -> networkName
    QMap<QString, ProxyConfig> m_instanceProxies;     // instanceId -> proxy
    QMap<QString, VpnConfig> m_instanceVpns;           // instanceId -> vpn
    QMap<QString, NetworkIdentity> m_instanceIdentities; // instanceId -> identity
    QMap<QString, bool> m_vpnActive;                  // instanceId -> active
    
    int m_networkCounter{0};
    QString m_baseSubnet{"10.200"};
};

}
#endif
