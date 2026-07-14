#!/bin/bash
###############################################################################
# Anti-Detect Android Emulator - Kernel Module Setup Script
# Installs required kernel modules for redroid
###############################################################################

set -e

echo "=========================================="
echo "  Redroid Anti-Detect Kernel Setup"
echo "=========================================="

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "Please run as root: sudo $0"
    exit 1
fi

# Detect OS
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$ID
else
    echo "Cannot detect OS"
    exit 1
fi

echo "[*] Detected OS: $OS"

# Install kernel modules based on OS
case "$OS" in
    ubuntu|debian|linuxmint|pop)
        echo "[*] Installing for Ubuntu/Debian..."
        
        # Install required packages
        apt update
        apt install -y linux-modules-extra-$(uname -r) || apt install -y linux-modules-extra-generic
        
        # Load binder module
        echo "[*] Loading binder_linux module..."
        modprobe binder_linux devices="binder,hwbinder,vndbinder" 2>/dev/null || {
            echo "[!] binder_linux module not available, trying binderfs..."
        }
        
        # Load ashmem module (for kernel < 5.18)
        echo "[*] Loading ashmem_linux module..."
        modprobe ashmem_linux 2>/dev/null || echo "[*] ashmem not needed for kernel >= 5.18"
        ;;
        
    fedora|rhel|centos|rocky|alma)
        echo "[*] Installing for Fedora/RHEL/CentOS..."
        
        # Install required packages
        yum install -y kernel-modules-extra
        
        # Load binder module
        modprobe binder_linux devices="binder,hwbinder,vndbinder" 2>/dev/null || echo "[!] Module load failed"
        modprobe ashmem_linux 2>/dev/null || echo "[*] ashmem not needed"
        ;;
        
    arch)
        echo "[*] Installing for Arch Linux..."
        
        pacman -Sy --noconfirm linux-headers
        
        # Load modules
        modprobe binder_linux devices="binder,hwbinder,vndbinder" 2>/dev/null || echo "[!] Module load failed"
        modprobe ashmem_linux 2>/dev/null || echo "[*] ashmem not needed"
        ;;
        
    *)
        echo "[!] Unsupported OS: $OS"
        echo "[*] Please manually load binder and ashmem modules"
        ;;
esac

# Verify modules are loaded
echo ""
echo "[*] Verifying kernel modules..."
echo ""

if grep -q "binder" /proc/filesystems 2>/dev/null; then
    echo "[✓] binder filesystem available"
else
    echo "[✗] binder filesystem NOT available"
fi

if grep -q "ashmem" /proc/misc 2>/dev/null; then
    echo "[✓] ashmem device available"
else
    echo "[*] ashmem device status unknown (may use memfd)"
fi

# List loaded modules
echo ""
echo "[*] Loaded redroid modules:"
lsmod | grep -E "binder|ashmem" || echo "    (modules may be built-in)"

echo ""
echo "=========================================="
echo "  Kernel Setup Complete!"
echo "=========================================="
echo ""
echo "Next steps:"
echo "  1. Run: docker compose up -d"
echo "  2. Connect: adb connect localhost:5555"
echo ""
