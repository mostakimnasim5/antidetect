/**
 * @file DeviceProfile.hpp
 * @brief Device profile structure for ReDroid instance configuration
 * @author VirtualPhonePro Team
 * @version 1.0.0
 */

#ifndef VIRTUALPHONEPRO_DEVICE_PROFILE_HPP
#define VIRTUALPHONEPRO_DEVICE_PROFILE_HPP

#include <QString>
#include <QVector>
#include <QMap>

namespace VirtualPhonePro {

enum class ProfileType {
    AntiDetect,
    Banking,
    Security,
    Gaming,
    SocialMedia
};

struct NetworkConfig {
    QString dns1{"8.8.8.8"};
    QString dns2{"8.8.4.4"};
    int dnsCount{2};
    QString proxyType{"none"};
    QString proxyHost;
    int proxyPort{3128};
    QStringList proxyExcludeList;
    QString proxyPac;
};

struct DisplayConfig {
    int width{1080};
    int height{2400};
    int dpi{480};
    int fps{60};
};

struct GpuConfig {
    QString mode{"host"};
    QString gpuNode;
};

struct DeviceIdentity {
    QString serialNumber;
    QString androidId;
    QString imei;
    QString meid;
    QString wifiMac;
    QString bluetoothMac;
};

struct BootState {
    QString verifiedBootstate{"green"};
    QString flashLocked{"1"};
    QString verityMode{"enforcing"};
    QString selinux{"permissive"};
};

struct OperatorConfig {
    QString operatorName{"Banglalink"};
    QString operatorNumeric{"47003"};
    QString simOperatorName{"Banglalink"};
    QString simOperatorNumeric{"47003"};
};

struct BuildInfo {
    QString fingerprint;
    QString description;
    QString versionIncremental;
    QString versionRelease{"14"};
    int versionSdk{34};
    QString securityPatch{"2024-03-01"};
    QString baseOs{"Android"};
};

struct ResourceLimits {
    int memoryLimitMb{768};
    float cpuLimit{1.0f};
    int memoryReservationMb{512};
};

struct DeviceProfile {
    QString profileId;
    ProfileType type{ProfileType::AntiDetect};
    QString profileName{"Default Anti-Detect"};
    QString profileDescription;
    
    QString model{"Pixel 8 Pro"};
    QString brand{"google"};
    QString manufacturer{"Google"};
    QString device{"husky"};
    QString board{"husky"};
    QString hardware{"redroid"};
    QString productName{"husky"};
    QString modDevice{"husky"};
    
    DeviceIdentity identity;
    BuildInfo build;
    BootState bootState;
    NetworkConfig network;
    DisplayConfig display;
    GpuConfig gpu;
    OperatorConfig operator_;
    ResourceLimits resources;
    
    bool useMemfd{true};
    bool useOverlayfs{false};
    bool enableGms{true};
    
    DeviceProfile();
    void generateUniqueIdentity();
    static DeviceProfile createBankingProfile();
    static DeviceProfile createSecurityProfile();
    static DeviceProfile createSocialMediaProfile();
    static DeviceProfile createGamingProfile();
    QMap<QString, QString> toEnvironmentMap() const;
    bool isValid() const;
    QString getValidationError() const;
};

}
#endif
