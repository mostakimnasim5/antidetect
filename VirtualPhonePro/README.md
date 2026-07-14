# VirtualPhonePro

Commercial-grade anti-detect Android emulator manager built with C++ and Qt6.

## Overview

VirtualPhonePro manages multiple ReDroid Android 14 instances via Docker Desktop on Windows, with advanced anti-detection features for each instance.

## Features

- **Multi-Instance Management**: Run multiple Android instances simultaneously
- **Anti-Detection**: Realistic device fingerprinting (IMEI, MAC, Device IDs)
- **SafetyNet/PlayIntegrity Spoofing**: Boot state and fingerprint configuration
- **Dynamic Configuration**: Unique identities per instance
- **Sensor Mocking**: GPS, accelerometer data injection
- **Low Resource Usage**: 512MB-768MB RAM per instance

## Requirements

- Windows 11 Pro/Enterprise
- Docker Desktop with WSL2 backend
- WSL2 (Ubuntu 22.04 recommended)
- 16GB RAM minimum (32GB recommended)
- 50GB free disk space

## Quick Start

### 1. Install Dependencies

```powershell
# Install WSL2
wsl --install -d Ubuntu-22.04

# Install Docker Desktop
# Download from https://www.docker.com/products/docker-desktop

# Setup WSL2 with Docker
wsl -d Ubuntu-22.04
./anti-detect-emulator/scripts/setup-kernel.sh
```

### 2. Build Application

```powershell
# Install Qt6 (via vcpkg)
vcpkg install qt6-base:x64-windows

# Configure and build
cmake --preset=msvc-release
cmake --build --preset=prod
```

### 3. Run

```powershell
# List instances
VirtualPhonePro.exe --list

# Start instance
VirtualPhonePro.exe --start banking

# Connect via ADB
adb connect localhost:5555
```

## Architecture

```
VirtualPhonePro.exe (Windows C++)
    ├── ReDroidController      # Container lifecycle management
    ├── DeviceProfile         # Device identity generation
    ├── Logger                # Logging system
    └── QProcess              # Process execution
            │
            ▼
    wsl.exe (WSL2)
            │
            ▼
    Docker Engine (WSL2)
            │
            ▼
    ReDroid Container (Android 14)
```

## Project Structure

```
VirtualPhonePro/
├── include/
│   ├── DeviceProfile.hpp     # Profile data structures
│   ├── ReDroidController.hpp  # Main controller class
│   └── Logger.hpp            # Logging interface
├── src/
│   ├── core/
│   │   ├── DeviceProfile.cpp # Profile implementation
│   │   └── ReDroidController.cpp
│   ├── common/
│   │   └── Logger.cpp
│   └── main.cpp
├── CMakeLists.txt
├── CMakePresets.json
└── README.md
```

## Profiles

| Profile | Device | Use Case |
|---------|--------|----------|
| `anti-detect` | Pixel 8 Pro | General purpose |
| `banking` | Samsung Galaxy S24 Ultra | Banking apps |
| `security` | Xiaomi 14 Pro | Security testing |
| `social` | Google Pixel 8 | Social media |
| `gaming` | ASUS ROG Phone | Gaming |

## API Reference

### ReDroidController

```cpp
// Start instance
bool startInstance(const QString& instanceId, const DeviceProfile& profile);

// Stop instance
bool stopInstance(const QString& instanceId, bool force = false);

// Send GPS data
bool sendGpsData(const QString& instanceId, double lat, double lon);

// Send sensor data
bool sendSensorData(const QString& instanceId, const SensorData& data);

// Execute shell command
QString executeShell(const QString& instanceId, const QString& command);

// Install APK
bool installApk(const QString& instanceId, const QString& apkPath);

// Take screenshot
bool takeScreenshot(const QString& instanceId, const QString& path);
```

## Development

### Building

```bash
# Debug build
cmake --preset=msvc-debug
cmake --build --preset=dev

# Release build
cmake --preset=msvc-release
cmake --build --preset=prod
```

### Testing

```bash
ctest --preset=all
```

## License

Proprietary - All rights reserved

## Authors

VirtualPhonePro Team
