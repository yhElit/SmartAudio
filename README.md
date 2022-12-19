# Smart Audio

Smart Audio Projekt für Communication Systems

## Konfiguration des Raspberry Pi Zero W für WiFi
Getestet wurde mit Raspbian Buster Lite, Version September 2019 (2019-09-26), Kernel 4.19
und ist auf der Seite www.raspberrypi.org/downloads/raspbian zu finden.

Anzulegen ist eine wpa_supplicant.conf im "boot"-Folder der SD-Karte.
Diese Datei beinhaltet folgendes:

```
country=DE
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
update_config=1

network={
    ssid="HierSSIDDesWLAN"
    scan_ssid=1
    psk="HierWLANPassphrase"
    key_mgmt=WPA-PSK
}
```
und ist anschließend unter */etc/wpa_supplicant/wpa_supplicant.conf* zu finden.

Desweiteren ist einfach eine Datei namens 'ssh' anzulegen um den Zugriff per SSH
zu gewährleisten.

## MQTT C Library
Als MQTT Library wird libmosquitto verwendet. Diese lässt sich im Gegensatz zu den Paho-Bibliotheken einfach aus den Repositories auf den Raspberry Pi installieren.
