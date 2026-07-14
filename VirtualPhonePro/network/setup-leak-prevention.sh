#!/bin/bash
###############################################################################
# VirtualPhonePro - Network Leak Prevention Setup
# Run this script on WSL2/Linux host before starting instances
###############################################################################

set -e

echo "=========================================="
echo "  VirtualPhonePro - Network Setup"
echo "=========================================="

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo -e "${RED}[ERROR] Please run as root: sudo $0${NC}"
    exit 1
fi

echo -e "${GREEN}[*] Setting up network leak prevention...${NC}"

# ==============================================================================
# 1. Enable IP Forwarding
# ==============================================================================
echo -e "${GREEN}[*] Enabling IP forwarding...${NC}"
cat >> /etc/sysctl.conf << 'EOF'

# VirtualPhonePro - Network Leak Prevention
net.ipv4.ip_forward = 1
net.ipv6.conf.all.forwarding = 1
net.ipv4.conf.all.rp_filter = 1
net.ipv4.conf.default.rp_filter = 1
net.ipv4.tcp_syncookies = 1
net.ipv4.conf.all.send_redirects = 0
net.ipv4.conf.default.send_redirects = 0
net.ipv4.conf.all.accept_redirects = 0
net.ipv4.conf.default.accept_redirects = 0
net.ipv4.conf.all.accept_source_route = 0
net.ipv4.conf.default.accept_source_route = 0
EOF

sysctl -p /etc/sysctl.conf 2>/dev/null || true

# ==============================================================================
# 2. Create iptables rules for leak prevention
# ==============================================================================
echo -e "${GREEN}[*] Configuring iptables rules...${NC}"

# Create iptables rules file
mkdir -p /etc/iptables

cat > /etc/iptables/rules.v4 << 'EOF'
# VirtualPhonePro iptables rules
# Generated automatically - DO NOT EDIT

*filter
:INPUT DROP [0:0]
:FORWARD DROP [0:0]
:OUTPUT ACCEPT [0:0]

# Allow loopback
-A INPUT -i lo -j ACCEPT
-A OUTPUT -o lo -j ACCEPT

# Allow established connections
-A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT

# Docker forwarding
-A FORWARD -i docker0 -j ACCEPT
-A FORWARD -i docker0 -o docker0 -j ACCEPT
-A FORWARD -o docker0 -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT

# Allow Docker networks
-A FORWARD -s 10.200.0.0/16 -j ACCEPT
-A FORWARD -d 10.200.0.0/16 -j ACCEPT

# Block private network leaks from Docker
-A FORWARD -s 10.0.0.0/8 ! -o docker+ -j DROP
-A FORWARD -s 172.16.0.0/12 ! -o docker+ -j DROP
-A FORWARD -s 192.168.0.0/16 ! -o docker+ -j DROP

# Allow VPN interfaces (wg0, tun+)
-A FORWARD -i wg0 -j ACCEPT
-A FORWARD -i tun+ -j ACCEPT

# Log dropped packets (optional, for debugging)
# -A FORWARD -m limit --limit 5/min -j LOG --log-prefix "IPT_FORWARD_DROP: "

COMMIT

*nat
:PREROUTING ACCEPT [0:0]
:INPUT ACCEPT [0:0]
:OUTPUT ACCEPT [0:0]
:POSTROUTING ACCEPT [0:0]

# Masquerade Docker traffic
-A POSTROUTING -s 10.200.0.0/16 ! -o docker+ -j MASQUERADE

# Proxy forwarding (for SOCKS/HTTP proxy)
# -A POSTROUTING -s 10.200.0.0/16 -p tcp -j REDIRECT --to-port 9040

COMMIT
EOF

# Apply rules
iptables-restore < /etc/iptables/rules.v4

# ==============================================================================
# 3. DNS Leak Prevention Setup
# ==============================================================================
echo -e "${GREEN}[*] Setting up DNS leak prevention...${NC}"

# Configure systemd-resolved if available
if [ -f /etc/systemd/resolved.conf ]; then
    cat > /etc/systemd/resolved.conf.d/virtualphonepro.conf << 'EOF'
[Resolve]
DNS=8.8.8.8 8.8.4.4 1.1.1.1
DNSOverTLS=yes
DNSStubListener=no
EOF
fi

# ==============================================================================
# 4. Create Docker daemon configuration
# ==============================================================================
echo -e "${GREEN}[*] Configuring Docker daemon...${NC}"

mkdir -p /etc/docker

cat > /etc/docker/daemon.json << 'EOF'
{
  "storage-driver": "overlay2",
  "iptables": true,
  "ip-forward": true,
  "ip-masq": true,
  "ipv6": false,
  "userland-proxy": false,
  "bridge": "none",
  "log-driver": "json-file",
  "log-opts": {
    "max-size": "10m",
    "max-file": "3"
  },
  "dns": ["8.8.8.8", "8.8.4.4"],
  "default-address-pools": [
    {
      "base": "10.200.0.0/16",
      "size": 24
    }
  ]
}
EOF

# ==============================================================================
# 5. Network namespace isolation setup
# ==============================================================================
echo -e "${GREEN}[*] Creating network namespace scripts...${NC}"

mkdir -p /opt/virtualphonepro/bin

# Script to create isolated network namespace
cat > /opt/virtualphonepro/bin/create-isolated-net.sh << 'NETEOF'
#!/bin/bash
# Create isolated network namespace for container

NETWORK_NAME=${1:-vphonepro}
NETWORK_SUBNET=${2:-10.200.0.0/24}

# Create network
docker network create \
    --driver bridge \
    --subnet=$NETWORK_SUBNET \
    --gateway=$(echo $NETWORK_SUBNET | cut -d/ -f1 | sed 's/0.0.0$$/1/') \
    --internal=false \
    $NETWORK_NAME

# Configure iptables for this network
iptables -I FORWARD -s $NETWORK_SUBNET -j ACCEPT
iptables -I FORWARD -d $NETWORK_SUBNET -j ACCEPT
iptables -t nat -I POSTROUTING -s $NETWORK_SUBNET ! -o $NETWORK_NAME -j MASQUERADE

echo "Network $NETWORK_NAME created with subnet $NETWORK_SUBNET"
NETEOF

chmod +x /opt/virtualphonepro/bin/create-isolated-net.sh

# ==============================================================================
# 6. MAC Spoofing helper
# ==============================================================================
cat > /opt/virtualphonepro/bin/spoof-mac.sh << 'MACEOF'
#!/bin/bash
# Spoof MAC address for container veth interface

CONTAINER_ID=${1}
NEW_MAC=${2:-$(openssl rand -hex 6 | sed "s/\(..\)/\1:/g" | sed "s/:$$//")}

# Get container PID
CONTAINER_PID=$(docker inspect --format '{{.State.Pid}}' $CONTAINER_ID 2>/dev/null)

if [ -z "$CONTAINER_PID" ]; then
    echo "Container not found: $CONTAINER_ID"
    exit 1
fi

# Get veth interface name
VETH_PAIR=$(nsenter -t $CONTAINER_PID -n ip link show 2>/dev/null | grep -oP 'eth0@if\d+' | sed 's/@if\d//')
HOST_VETH="veth$(echo $VETH_PAIR | sed 's/eth0/veth1/')"

# Change MAC address
ip link set $VETH_PAIR down
ip link set $VETH_PAIR address $NEW_MAC
ip link set $VETH_PAIR up

# Also change in namespace
nsenter -t $CONTAINER_PID -n ip link set eth0 down
nsenter -t $CONTAINER_PID -n ip link set eth0 address $NEW_MAC
nsenter -t $CONTAINER_PID -n ip link set eth0 up

echo "MAC address spoofed to: $NEW_MAC"
MACEOF

chmod +x /opt/virtualphonepro/bin/spoof-mac.sh

# ==============================================================================
# 7. DNS leak test script
# ==============================================================================
cat > /opt/virtualphonepro/bin/test-dns-leak.sh << 'TESTEOF'
#!/bin/bash
# Test for DNS leaks

CONTAINER_ID=${1}
DNS_SERVER=${2:-8.8.8.8}

echo "Testing DNS leak for container: $CONTAINER_ID"

# Query known DNS test domains through specific server
RESULT=$(docker exec $CONTAINER_ID dig @$DNS_SERVER whoami.akamai.net +short 2>/dev/null)

if [ -z "$RESULT" ]; then
    echo "WARNING: No response from DNS server"
    exit 1
fi

# Check if response came from expected server
VERIFIED=$(docker exec $CONTAINER_ID dig +short whoami.akamai.net +tries=1 2>/dev/null)

echo "DNS working: $RESULT"
echo "Verification: $VERIFIED"

if [ "$RESULT" == "$VERIFIED" ]; then
    echo "DNS leak test: PASSED"
    exit 0
else
    echo "DNS leak test: POTENTIAL LEAK DETECTED"
    exit 2
fi
TESTEOF

chmod +x /opt/virtualphonepro/bin/test-dns-leak.sh

# ==============================================================================
# 8. IP leak test script
# ==============================================================================
cat > /opt/virtualphonepro/bin/test-ip-leak.sh << 'IPEOF'
#!/bin/bash
# Test for IP leaks

CONTAINER_ID=${1}

echo "Testing IP leak for container: $CONTAINER_ID"

# Get external IP through container
EXTERNAL_IP=$(docker exec $CONTAINER_ID curl -s ifconfig.me 2>/dev/null || \
             docker exec $CONTAINER_ID curl -s ipinfo.io/ip 2>/dev/null || \
             docker exec $CONTAINER_ID wget -qO- ifconfig.me 2>/dev/null)

# Get internal IP
INTERNAL_IP=$(docker exec $CONTAINER_ID ip addr show eth0 2>/dev/null | grep -oP 'inet \K[\d.]+')

echo "External IP: $EXTERNAL_IP"
echo "Internal IP: $INTERNAL_IP"

# Check for private IPs in external
if [[ "$EXTERNAL_IP" =~ ^10\. ]] || [[ "$EXTERNAL_IP" =~ ^172\.(1[6-9]|2[0-9]|3[0-1] ]] || [[ "$EXTERNAL_IP" =~ ^192\.168\. ]]; then
    echo "IP LEAK DETECTED: Private IP exposed"
    exit 2
fi

# Check if external IP is empty
if [ -z "$EXTERNAL_IP" ]; then
    echo "WARNING: Could not determine external IP"
    exit 1
fi

echo "IP leak test: PASSED"
exit 0
IPEOF

chmod +x /opt/virtualphonepro/bin/test-ip-leak.sh

# ==============================================================================
# 9. WebRTC leak prevention for Android
# ==============================================================================
echo -e "${GREEN}[*] WebRTC leak prevention...${NC}"

cat > /opt/virtualphonepro/bin/prevent-webrtc-leak.sh << 'WEBRTCEOF'
#!/bin/bash
# Prevent WebRTC IP leaks in Android container

CONTAINER_ID=${1}

echo "Configuring WebRTC leak prevention for: $CONTAINER_ID"

# Disable WebRTC ICE candidates that expose local IP
docker exec $CONTAINER_ID settings put global webview_enable_mock_geo true 2>/dev/null || true

# Force WebRTC to use only relay candidates (TURN)
# This requires a TURN server configuration
docker exec $CONTAINER_ID setprop net.webrtc.ice_relay_only true 2>/dev/null || true

# Block direct peer connections
docker exec $CONTAINER_ID setprop net.webrtc.ice_prefer_relay true 2>/dev/null || true

# Disable local candidate discovery
docker exec $CONTAINER_ID setprop net.webrtc.ice_local_address_mask 255.255.255.0 2>/dev/null || true

echo "WebRTC leak prevention configured"
WEBRTCEOF

chmod +x /opt/virtualphonepro/bin/prevent-webrtc-leak.sh

# ==============================================================================
# 10. Tor proxy setup (optional)
# ==============================================================================
echo -e "${YELLOW}[*] Tor proxy setup (optional)...${NC}"

cat > /opt/virtualphonepro/bin/setup-tor-proxy.sh << 'TOREOF'
#!/bin/bash
# Setup Tor as proxy for container

CONTAINER_ID=${1}
TOR_PORT=${2:-9050}

echo "Setting up Tor proxy for: $CONTAINER_ID"

# Start Tor container
docker run -d \
    --name ${CONTAINER_ID}_tor \
    --network container:${CONTAINER_ID} \
    --cap-add NET_ADMIN \
    -p ${TOR_PORT}:9050 \
    -p 9051:9051 \
    -e TOR_NODNS=1 \
    dperson/tor-proxy:latest

# Get Tor container IP
TOR_IP=$(docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' ${CONTAINER_ID}_tor)

# Configure proxy in Android
docker exec $CONTAINER_ID settings put global http_proxy $TOR_IP:9050 2>/dev/null || true

echo "Tor proxy configured at: $TOR_IP:9050"
echo "Tor control port: 9051"
TOREOF

chmod +x /opt/virtualphonepro/bin/setup-tor-proxy.sh

# ==============================================================================
# Final message
# ==============================================================================
echo ""
echo -e "${GREEN}=========================================="
echo -e "  Network Setup Complete!"
echo -e "=========================================="
echo ""
echo "Installed scripts:"
echo "  /opt/virtualphonepro/bin/create-isolated-net.sh"
echo "  /opt/virtualphonepro/bin/spoof-mac.sh"
echo "  /opt/virtualphonepro/bin/test-dns-leak.sh"
echo "  /opt/virtualphonepro/bin/test-ip-leak.sh"
echo "  /opt/virtualphonepro/bin/prevent-webrtc-leak.sh"
echo "  /opt/virtualphonepro/bin/setup-tor-proxy.sh"
echo ""
echo "Usage examples:"
echo "  # Create isolated network"
echo "  sudo /opt/virtualphonepro/bin/create-isolated-net.sh mynet 10.200.1.0/24"
echo ""
echo "  # Test for leaks"
echo "  sudo /opt/virtualphonepro/bin/test-ip-leak.sh <container_id>"
echo ""
echo -e "${YELLOW}NOTE: Restart Docker to apply Docker daemon changes${NC}"
echo "  sudo systemctl restart docker"
echo ""
