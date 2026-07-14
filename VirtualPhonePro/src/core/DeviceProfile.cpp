/**
 * @file DeviceProfile.cpp
 * @brief DeviceProfile implementation
 */

#include "DeviceProfile.hpp"
#include <QUuid>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDateTime>

namespace VirtualPhonePro {

// ============================================================================
// DeviceProfile Implementation
// ============================================================================

DeviceProfile::DeviceProfile() {
    // Generate profile ID
    profileId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    // Generate unique identity
    generateUniqueIdentity();
    
    // Set default fingerprint for Pixel 8 Pro
    build.fingerprint = "google/husky/husky:14/UP1A.231005.007/10861975:user/release-keys";
    build.description = "husky-user 14 UP1A.231005.007 10861975 release-keys";
    build.versionIncremental = "10861975";
    build.versionRelease = "14";
    build.versionSdk = 34;
    build.securityPatch = "2024-03-01";
}

// ============================================================================
// Generate Unique Identifiers
// ============================================================================

void DeviceProfile::generateUniqueIdentity() {
    // Generate random serial number (format: SSXXXXXXXX)
    QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString randomPart = uuid.left(8).toUpper();
    identity.serialNumber = "SS" + randomPart;
    
    // Generate Android ID (16 hex characters)
    QString androidIdData = uuid + QDateTime::currentMSecsSinceEpoch();
    QByteArray hash = QCryptographicHash::hash(
        androidIdData.toUtf8(), 
        QCryptographicHash::Md5
    );
    identity.androidId = hash.toHex().left(16);
    
    // Generate IMEI (15 digits with Luhn check digit)
    // TAC (Type Allocation Code) - 8 digits
    QString tac = "358888"; // Google TAC prefix
    for (int i = 0; i < 6; ++i) {
        tac += QString::number(QRandomGenerator::global()->bounded(0, 10));
    }
    // Calculate Luhn check digit
    int sum = 0;
    bool alternate = true;
    for (int i = (int)tac.length() - 1; i >= 0; --i) {
        int n = tac[i].digitValue();
        if (alternate) {
            n *= 2;
            if (n > 9) n -= 9;
        }
        sum += n;
        alternate = !alternate;
    }
    int checkDigit = (10 - (sum % 10)) % 10;
    identity.imei = tac + QString::number(checkDigit);
    
    // Generate MAC addresses with Google OUI prefix
    QString oui = "001A11";
    QString nic = uuid.remove('-').left(6).toUpper();
    identity.wifiMac = oui + ":" + nic.mid(0, 2) + ":" + nic.mid(2, 2) + ":" + nic.mid(4, 2);
    
    // Generate different Bluetooth MAC
    QString btNic = uuid.remove('-').right(6).toUpper();
    identity.bluetoothMac = oui + ":" + btNic.mid(0, 2) + ":" + btNic.mid(2, 2) + ":" + btNic.mid(4, 2);
}

// ============================================================================
// Preset Profile Factories
// ============================================================================

DeviceProfile DeviceProfile::createBankingProfile() {
    DeviceProfile profile;
    profile.type = ProfileType::Banking;
    profile.profileName = "Banking Profile";
    profile.profileDescription = "Optimized for banking and financial applications";
    
    // Samsung Galaxy S24 Ultra configuration
    profile.model = "SM-S928B";
    profile.brand = "samsung";
    profile.manufacturer = "Samsung";
    profile.device = "galaxy_s24_ultra";
    profile.board = "kalama";
    profile.hardware = "qcom";
    profile.productName = "galaxy_s24_ultra";
    profile.modDevice = "galaxy_s24_ultra";
    
    // Samsung-specific fingerprint
    profile.build.fingerprint = "samsung/galaxy_s24_ultra/galaxy_s24_ultra:14/UP1A.231005.007/1234567:user/release-keys";
    profile.build.description = "galaxy_s24_ultra-user 14 UP1A.231005.007 1234567 release-keys";
    profile.build.versionIncremental = "1234567";
    
    // Generate new unique identity
    profile.generateUniqueIdentity();
    
    // Stricter security settings
    profile.bootState.verifiedBootstate = "green";
    profile.bootState.flashLocked = "1";
    
    // Higher resources for banking apps
    profile.resources.memoryLimitMb = 1024;
    profile.resources.memoryReservationMb = 768;
    profile.resources.cpuLimit = 1.5f;
    
    return profile;
}

DeviceProfile DeviceProfile::createSecurityProfile() {
    DeviceProfile profile;
    profile.type = ProfileType::Security;
    profile.profileName = "Security Testing Profile";
    profile.profileDescription = "For security research and penetration testing";
    
    // Xiaomi 14 Pro configuration
    profile.model = "23127PN0CC";
    profile.brand = "xiaomi";
    profile.manufacturer = "Xiaomi";
    profile.device = "darwin";
    profile.board = "taroin";
    profile.hardware = "qcom";
    profile.productName = "darwin";
    profile.modDevice = "darwin";
    
    // Xiaomi-specific fingerprint
    profile.build.fingerprint = "Xiaomi/darwin/darwin:14/UKQ1.231003.001/123456:user/release-keys";
    profile.build.description = "darwin-user 14 UKQ1.231003.001 123456 release-keys";
    
    // Generate new unique identity
    profile.generateUniqueIdentity();
    
    // Relaxed security for testing
    profile.bootState.selinux = "permissive";
    profile.bootState.verifiedBootstate = "yellow";
    profile.bootState.flashLocked = "0";
    
    // Standard resources
    profile.resources.memoryLimitMb = 768;
    profile.resources.memoryReservationMb = 512;
    
    return profile;
}

DeviceProfile DeviceProfile::createSocialMediaProfile() {
    DeviceProfile profile;
    profile.type = ProfileType::SocialMedia;
    profile.profileName = "Social Media Profile";
    profile.profileDescription = "Optimized for social media applications";
    
    // Google Pixel 8 configuration
    profile.model = "Pixel 8";
    profile.brand = "google";
    profile.manufacturer = "Google";
    profile.device = "shiba";
    profile.board = "shiba";
    profile.hardware = "gs201";
    profile.productName = "shiba";
    profile.modDevice = "shiba";
    
    // Pixel 8 fingerprint
    profile.build.fingerprint = "google/shiba/shiba:14/UP1A.231005.007/9876543:user/release-keys";
    profile.build.description = "shiba-user 14 UP1A.231005.007 9876543 release-keys";
    
    // Generate new unique identity
    profile.generateUniqueIdentity();
    
    profile.resources.memoryLimitMb = 768;
    profile.resources.cpuLimit = 1.0f;
    
    return profile;
}

DeviceProfile DeviceProfile::createGamingProfile() {
    DeviceProfile profile;
    profile.type = ProfileType::Gaming;
    profile.profileName = "Gaming Profile";
    profile.profileDescription = "Optimized for gaming applications";
    
    // ASUS ROG Phone configuration
    profile.model = "ASUS_AI2201";
    profile.brand = "asus";
    profile.manufacturer = "ASUS";
    profile.device = "AI2201";
    profile.board = "lahaina";
    profile.hardware = "qcom";
    profile.productName = "AI2201";
    profile.modDevice = "AI2201";
    
    // Gaming fingerprint
    profile.build.fingerprint = "asus/AI2201/AI2201:14/UP1A.231005.007/555555:user/release-keys";
    profile.build.description = "AI2201-user 14 UP1A.231005.007 555555 release-keys";
    
    // Generate new unique identity
    profile.generateUniqueIdentity();
    
    // Higher resources for gaming
    profile.resources.memoryLimitMb = 1024;
    profile.resources.memoryReservationMb = 768;
    profile.resources.cpuLimit = 2.0f;
    
    // Higher FPS
    profile.display.fps = 120;
    
    return profile;
}

// ============================================================================
// Environment Map Conversion
// ============================================================================

QMap<QString, QString> DeviceProfile::toEnvironmentMap() const {
    QMap<QString, QString> env;
    
    // Device Identity
    env["DEVICE_MODEL"] = model;
    env["DEVICE_BRAND"] = brand;
    env["DEVICE_MANUFACTURER"] = manufacturer;
    env["DEVICE_NAME"] = device;
    env["DEVICE_DEVICE"] = device;
    env["DEVICE_BOARD"] = board;
    env["DEVICE_HARDWARE"] = hardware;
    env["DEVICE_PRODUCT_NAME"] = productName;
    env["DEVICE_MOD_DEVICE"] = modDevice;
    
    // Unique Identifiers
    env["SERIAL_NUMBER"] = identity.serialNumber;
    env["ANDROID_ID"] = identity.androidId;
    env["IMEI"] = identity.imei;
    env["WIFI_MAC"] = identity.wifiMac;
    env["BLUETOOTH_MAC"] = identity.bluetoothMac;
    
    // Build Info
    env["BUILD_FINGERPRINT"] = build.fingerprint;
    env["BUILD_DESCRIPTION"] = build.description;
    env["BUILD_VERSION_INCREMENTAL"] = build.versionIncremental;
    env["BUILD_VERSION_RELEASE"] = build.versionRelease;
    env["BUILD_VERSION_SDK"] = QString::number(build.versionSdk);
    env["BUILD_VERSION_SECURITY_PATCH"] = build.securityPatch;
    env["BUILD_BASE_OS"] = build.baseOs;
    
    // Boot State
    env["BOOT_VERIFIED"] = bootState.verifiedBootstate;
    env["BOOT_LOCKED"] = bootState.flashLocked;
    env["BOOT_VERITY_MODE"] = bootState.verityMode;
    env["BOOT_SELINUX"] = bootState.selinux;
    
    // Network
    env["NETWORK_DNS1"] = network.dns1;
    env["NETWORK_DNS2"] = network.dns2;
    env["NETWORK_DNS_COUNT"] = QString::number(network.dnsCount);
    env["NETWORK_PROXY_TYPE"] = network.proxyType;
    env["NETWORK_PROXY_HOST"] = network.proxyHost;
    env["NETWORK_PROXY_PORT"] = QString::number(network.proxyPort);
    
    // Display
    env["DISPLAY_WIDTH"] = QString::number(display.width);
    env["DISPLAY_HEIGHT"] = QString::number(display.height);
    env["DISPLAY_DPI"] = QString::number(display.dpi);
    env["DISPLAY_FPS"] = QString::number(display.fps);
    
    // GPU
    env["GPU_MODE"] = gpu.mode;
    env["GPU_NODE"] = gpu.gpuNode;
    
    // Operator
    env["OPERATOR_NAME"] = operator_.operatorName;
    env["OPERATOR_NUMERIC"] = operator_.operatorNumeric;
    env["SIM_OPERATOR_NAME"] = operator_.simOperatorName;
    env["SIM_OPERATOR_NUMERIC"] = operator_.simOperatorNumeric;
    
    // Resources
    env["MEMORY_LIMIT_MB"] = QString::number(resources.memoryLimitMb);
    env["CPU_LIMIT"] = QString::number(resources.cpuLimit);
    env["MEMORY_RESERVATION_MB"] = QString::number(resources.memoryReservationMb);
    
    // Advanced
    env["USE_MEMFD"] = useMemfd ? "true" : "false";
    env["USE_OVERLAYFS"] = useOverlayfs ? "true" : "false";
    env["ENABLE_GMS"] = enableGms ? "true" : "false";
    
    return env;
}

// ============================================================================
// Validation
// ============================================================================

bool DeviceProfile::isValid() const {
    // Check required fields
    if (model.isEmpty() || brand.isEmpty() || manufacturer.isEmpty()) {
        return false;
    }
    
    if (device.isEmpty() || board.isEmpty()) {
        return false;
    }
    
    if (identity.serialNumber.isEmpty()) {
        return false;
    }
    
    if (identity.androidId.length() != 16) {
        return false;
    }
    
    if (identity.imei.length() != 15) {
        return false;
    }
    
    // Validate display settings
    if (display.width <= 0 || display.height <= 0) {
        return false;
    }
    
    if (display.dpi <= 0 || display.dpi > 640) {
        return false;
    }
    
    // Validate resources
    if (resources.memoryLimitMb < 256 || resources.memoryLimitMb > 4096) {
        return false;
    }
    
    return true;
}

QString DeviceProfile::getValidationError() const {
    if (model.isEmpty()) return "Model is required";
    if (brand.isEmpty()) return "Brand is required";
    if (manufacturer.isEmpty()) return "Manufacturer is required";
    if (device.isEmpty()) return "Device is required";
    if (identity.serialNumber.isEmpty()) return "Serial number is required";
    if (identity.androidId.length() != 16) return "Android ID must be 16 characters";
    if (identity.imei.length() != 15) return "IMEI must be 15 digits";
    if (display.width <= 0) return "Display width must be positive";
    if (display.height <= 0) return "Display height must be positive";
    if (resources.memoryLimitMb < 256) return "Memory limit must be at least 256MB";
    
    return QString();
}

} // namespace VirtualPhonePro
