# 🛡️ Anti-Detect Android Emulator

A GPU-accelerated Android emulator built on redroid with advanced anti-detection features for banking app testing, security testing, and app analysis.

## 📋 Features

- **GPU Accelerated** - Hardware-accelerated rendering using host GPU
- **Multi-Architecture** - Supports both ARM64 and x86_64
- **Anti-Detection** - Realistic device fingerprinting to avoid detection
- **Banking Ready** - Configured for financial app testing
- **Security Testing** - Optimized for security research and penetration testing
- **Easy Management** - Docker Compose based deployment

## 🔧 Requirements

### System Requirements

- **OS**: Linux (Ubuntu 20.04+, Debian 11+, Fedora 36+)
- **Kernel**: 4.14+ (with binder and ashmem/memfd support)
- **Docker**: 20.10+ with docker compose plugin
- **RAM**: 4GB minimum (8GB recommended)
- **Storage**: 10GB minimum

### Kernel Modules

```bash
# Ubuntu/Debian
sudo apt install linux-modules-extra-$(uname -r)
sudo modprobe binder_linux devices="binder,hwbinder,vndbinder"
sudo modprobe ashmem_linux  # For kernel < 5.18

# Or run the setup script
sudo ./scripts/setup-kernel.sh
```

## 🚀 Quick Start

### 1. Setup Kernel Modules
### 2. Start the Emulator
### 3. Connect via ADB
### 4. View Screen

## 📱 Profiles

- Anti-Detect Profile (Default)
- Banking Profile
- Security Profile

## 🛠️ Management Commands

```bash
./scripts/emulator-manager.sh start
./scripts/emulator-manager.sh stop
./scripts/emulator-manager.sh status
./scripts/emulator-manager.sh connect
./scripts/emulator-manager.sh logs
./scripts/emulator-manager.sh shell
./scripts/emulator-manager.sh backup
./scripts/emulator-manager.sh restore
./scripts/emulator-manager.sh cleanup
```

## 🔐 Security Considerations

⚠️ This tool is for legitimate purposes only.

## 📚 References

- [Redroid Official Docs](https://github.com/remote-android/redroid-doc)
- [scrcpy](https://github.com/Genymobile/scrcpy)

---
**Built with ❤️ for Android testing and security research**
