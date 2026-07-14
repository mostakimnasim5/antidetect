#!/system/bin/sh
###############################################################################
# Anti-Detect Initialization Script
# Runs inside the Android emulator to set up anti-detection
###############################################################################

echo "[ANTIDETECT] Starting anti-detection setup..."

# Remove common emulator detection markers
echo "[ANTIDETECT] Removing emulator markers..."

# Remove qemu props
/system/bin/settings delete global qemu_props_initialized 2>/dev/null
/system/bin/settings delete global qemu_adb_last_touch_time 2>/dev/null

# Set realistic device info
setprop ro.product.model "Pixel 8 Pro"
setprop ro.product.brand "google"
setprop ro.product.name "husky"
setprop ro.product.device "husky"
setprop ro.product.board "husky"
setprop ro.product.manufacturer "Google"

# Hide virtual device flags
setprop ro.kernel.qemu 0
setprop ro.hardware "redroid"
setprop ro.bootloader "unknown"
setprop ro.build.characteristics "tablet"

# Fingerprint
setprop ro.build.fingerprint "google/husky/husky:14/UP1A.231005.007/10861975:user/release-keys"
setprop ro.build.description "husky-user 14 UP1A.231005.007 10861975 release-keys"

# Network info
setprop net.eth0.dns1 "8.8.8.8"
setprop net.eth0.dns2 "8.8.4.4"
setprop net.dns1 "8.8.8.8"
setprop net.dns2 "8.8.4.4"

# Disable debugging features
setprop debug.atrace.tags.enableflags 0
setprop persist.traced.enable 0

# Battery simulation (realistic values)
setprop sys.battery.capacity 100
setprop sys.battery.status "discharging"
setprop sys.battery.health "good"
setprop sys.battery.voltage 4000
setprop sys.battery.temperature 25
setprop sys.battery.present true

# Location (if GPS is enabled)
setprop mock.location.enabled 0
setprop gsm.sim.operator.numeric "47003"
setprop gsm.operator.numeric "47003"
setprop gsm.sim.operator.alpha "Banglalink"
setprop gsm.operator.alpha "Banglalink"

# Timezone
setprop persist.sys.timezone "Asia/Dhaka"

# Audio
setprop audio.offload.enabled 1

echo "[ANTIDETECT] Anti-detection setup complete!"
