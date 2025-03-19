#!/bin/bash
 
# Configuration du hotspot Wi-Fi
SSID="RaspberryPiHotspot"
PASSWORD="raspberry123"
INTERFACE="wlan0"
 
# Arrêter le gestionnaire de réseau par défaut
sudo systemctl stop NetworkManager
 
# Configurer l'interface Wi-Fi
sudo ifconfig $INTERFACE down
sudo ifconfig $INTERFACE 192.168.4.1 netmask 255.255.255.0 up
 
# Configurer hostapd
echo "interface=$INTERFACE
driver=nl80211
ssid=$SSID
hw_mode=g
channel=7
wmm_enabled=0
macaddr_acl=0
auth_algs=1
ignore_broadcast_ssid=0
wpa=2
wpa_passphrase=$PASSWORD
wpa_key_mgmt=WPA-PSK
rsn_pairwise=CCMP" | sudo tee /etc/hostapd/hostapd.conf > /dev/null
 
# Configurer dnsmasq
echo "interface=$INTERFACE
dhcp-range=192.168.4.2,192.168.4.20,255.255.255.0,24h" | sudo tee /etc/dnsmasq.conf > /dev/null
 
# Démarrer dnsmasq et hostapd
sudo systemctl start dnsmasq
sudo hostapd /etc/hostapd/hostapd.conf &
 
# Démarrer le serveur web Python
#python3 /chemin/vers/votre/script_python.py &
