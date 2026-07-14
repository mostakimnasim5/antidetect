#!/bin/bash
###############################################################################
# Anti-Detect Android Emulator - Management Script
# Control and manage redroid containers
###############################################################################

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

show_help() {
    cat << EOF
${BLUE}╔══════════════════════════════════════════════════════════╗
║     Anti-Detect Android Emulator - Manager                  ║
╚══════════════════════════════════════════════════════════╝${NC}

${GREEN}Usage:${NC} ./emulator-manager.sh [COMMAND] [OPTIONS]

${GREEN}Commands:${NC}
  start           Start the anti-detect emulator
  stop            Stop the emulator
  restart         Restart the emulator
  status          Show emulator status
  connect         Connect via ADB
  disconnect      Disconnect ADB
  shell           Open shell in emulator
  logs            View emulator logs
  backup          Backup emulator data
  restore         Restore emulator data
  cleanup         Remove all containers and images

${GREEN}Profiles:${NC}
  anti-detect     Default anti-detection profile
  banking         Banking app testing profile
  security        Security testing profile

${GREEN}Examples:${NC}
  ./emulator-manager.sh start
  ./emulator-manager.sh start banking
  ./emulator-manager.sh connect
  ./emulator-manager.sh logs -f

EOF
}

check_docker() {
    if ! command -v docker &> /dev/null; then
        echo -e "${RED}[✗] Docker not installed!${NC}"
        exit 1
    fi
    
    if ! docker info &> /dev/null; then
        echo -e "${RED}[✗] Docker daemon not running!${NC}"
        echo -e "${YELLOW}[*] Run: sudo systemctl start docker${NC}"
        exit 1
    fi
}

check_kernel_modules() {
    echo -e "${BLUE}[*] Checking kernel modules...${NC}"
    
    if grep -q "binder" /proc/filesystems 2>/dev/null; then
        echo -e "${GREEN}[✓] binder${NC}"
    else
        echo -e "${RED}[✗] binder not available - run setup-kernel.sh${NC}"
    fi
}

start_emulator() {
    local profile="${1:-anti-detect}"
    check_docker
    check_kernel_modules
    
    echo -e "${BLUE}[*] Starting emulator: $profile${NC}"
    docker compose up -d "$profile"
    echo -e "${GREEN}[✓] Emulator started!${NC}"
    echo -e "${YELLOW}[*] Connect with: adb connect localhost:5555${NC}"
}

stop_emulator() {
    local profile="${1:-redroid-antidetect}"
    echo -e "${BLUE}[*] Stopping emulator: $profile${NC}"
    docker stop "$profile" 2>/dev/null || docker compose stop "$profile"
    echo -e "${GREEN}[✓] Emulator stopped${NC}"
}

restart_emulator() {
    local profile="${1:-redroid-antidetect}"
    stop_emulator "$profile"
    sleep 2
    start_emulator "${profile#redroid-}"
}

show_status() {
    check_docker
    echo -e "${BLUE}╔══════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║                  Emulator Status                         ║${NC}"
    echo -e "${BLUE}╚══════════════════════════════════════════════════════════╝${NC}"
    echo ""
    
    docker compose ps
    
    echo ""
    echo -e "${BLUE}ADB Connection:${NC}"
    if adb devices 2>/dev/null | grep -q "localhost:5555"; then
        echo -e "${GREEN}[✓] Connected to localhost:5555${NC}"
    else
        echo -e "${YELLOW}[-] Not connected${NC}"
    fi
}

connect_adb() {
    echo -e "${BLUE}[*] Connecting to emulator via ADB...${NC}"
    adb connect localhost:5555
    sleep 2
    adb devices
}

disconnect_adb() {
    echo -e "${BLUE}[*] Disconnecting ADB...${NC}"
    adb disconnect localhost:5555 2>/dev/null || true
}

open_shell() {
    echo -e "${BLUE}[*] Opening shell in emulator...${NC}"
    adb shell
}

show_logs() {
    local follow=""
    if [[ "$1" == "-f" ]]; then
        follow="-f"
        shift
    fi
    
    local container="${1:-redroid-antidetect}"
    docker logs $follow "$container"
}

backup_data() {
    local backup_dir="./backups/$(date +%Y%m%d_%H%M%S)"
    mkdir -p "$backup_dir"
    
    echo -e "${BLUE}[*] Backing up emulator data to: $backup_dir${NC}"
    docker cp redroid-antidetect:/data/. "$backup_dir/data/"
    tar -czf "$backup_dir.tar.gz" -C "$backup_dir" .
    rm -rf "$backup_dir"
    
    echo -e "${GREEN}[✓] Backup saved: $backup_dir.tar.gz${NC}"
}

restore_data() {
    local backup_file="${1}"
    
    if [ -z "$backup_file" ]; then
        echo -e "${RED}[✗] Please specify backup file${NC}"
        exit 1
    fi
    
    echo -e "${BLUE}[*] Restoring from: $backup_file${NC}"
    
    local temp_dir="/tmp/redroid-restore-$$"
    mkdir -p "$temp_dir"
    tar -xzf "$backup_file" -C "$temp_dir"
    
    docker cp "$temp_dir/data/." redroid-antidetect:/data/
    rm -rf "$temp_dir"
    
    echo -e "${GREEN}[✓] Data restored! Restart emulator to apply.${NC}"
}

cleanup() {
    echo -e "${RED}[!] WARNING: This will remove ALL redroid containers and images!${NC}"
    read -p "Continue? (y/N): " confirm
    
    if [ "$confirm" = "y" ] || [ "$confirm" = "Y" ]; then
        echo -e "${BLUE}[*] Cleaning up...${NC}"
        docker compose down --rmi all
        docker system prune -f
        echo -e "${GREEN}[✓] Cleanup complete${NC}"
    else
        echo -e "${YELLOW}[*] Cancelled${NC}"
    fi
}

# Main
case "${1:-}" in
    start)
        start_emulator "$2"
        ;;
    stop)
        stop_emulator "$2"
        ;;
    restart)
        restart_emulator "$2"
        ;;
    status)
        show_status
        ;;
    connect)
        connect_adb
        ;;
    disconnect)
        disconnect_adb
        ;;
    shell)
        open_shell
        ;;
    logs)
        show_logs "$2" "$3"
        ;;
    backup)
        backup_data
        ;;
    restore)
        restore_data "$2"
        ;;
    cleanup)
        cleanup
        ;;
    help|--help|-h)
        show_help
        ;;
    *)
        show_help
        exit 1
        ;;
esac
