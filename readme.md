## 🔄 Update: Migration to MQTT (v2.0)
The previous version, which sent direct HTTPS commands to WiiM devices, has been moved to the `legacy_https_version` folder. This new version uses **MQTT**, making the setup significantly more flexible for modern smart homes.

---

## ⚠️ Important Notice / Disclaimer
> **This is my first-ever ESP32 project.**
* I am still learning ESP-IDF, Bluetooth Classic, and L2CAP.
* The code works in my environment, but there is **no guarantee** it will work for yours.
* **No Support:** I cannot provide technical assistance for troubleshooting.
* **Use at Your Own Risk:** No liability for damages or security issues.

---

## 🔍 Technical Approach & Debouncing
This project bypasses the standard HID abstraction layer for direct control:
* **L2CAP Direct Access:** Manually registers PSMs `0x11` (Control) and `0x13` (Interrupt).
* **Raw Data Parsing:** Button bytes are extracted directly from the `L2CA_DataInd_Cb`.
* **Software Debouncing:** The code includes a check (`KEY_DEBOUNCE_MS = 200`) to prevent a single physical press from triggering multiple MQTT messages.



---

## 🚀 Features
* ✅ **Bluetooth Classic Pairing:** Handles SSP (Secure Simple Pairing) with auto-acceptance.
* ✅ **Efficient Debouncing:** Filters signal noise using a 200ms threshold.
* ✅ **MQTT Integration:** Sends JSON data including the button name and a timestamp.
* ✅ **WiFi Management:** Features automatic reconnection logic.

---

## 🛠 Configuration & Installation

### 1. Adjust the Code
Update these definitions in `firetv_remote2mqtt.c` to match your network:

```c
#define WIFI_SSID      "YOUR_SSID"
#define WIFI_PASSWORD  "YOUR_PASSWORD"
#define MQTT_BROKER    "mqtt://192.168.xxx.xxx"

// Enter the MAC address of your specific remote
esp_bd_addr_t remote_bda = {0x00, 0x00, 0x00, 0x8D, 0x00, 0x00}; 
2. Build Requirements
ESP-IDF: v5.x (Tested with 5.5.2).


Dependencies: bt, nvs_flash, and mqtt must be in your CMakeLists.txt. 

sdkconfig: Ensure Bluetooth Classic and Bluedroid are enabled (use the provided sdkconfig).

📡 MQTT Payload Example
Messages are published to the topic firetv/remote:

JSON

{
  "button": "OK",
  "timestamp": 1245
}
🗺 Button Mapping
Navigation: UP, DOWN, LEFT, RIGHT, OK

Media: PLAY_PAUSE, SKIP_FORWARD, SKIP_REWIND

System: HOME, BACK, MENU

🚫 Microphone: Not supported.

📜 License
MIT License.


---

### 🇩🇪 Deutsche Version

```markdown
# ESP32 Fire TV Remote → MQTT Bridge 📺📡

Dieses Projekt verbindet eine **Amazon Fire TV Fernbedienung (Modell BE59CV)** per **Bluetooth Classic** mit einem **ESP32**. 
Der ESP32 liest die Tastensignale **direkt aus rohen L2CAP-Paketen** und sendet diese als **JSON-Payload an einen MQTT-Broker**.

👉 **Ziel:** Die Fire-TV-Fernbedienung als universelle Steuerung für Home Assistant oder andere MQTT-basierte Smart-Home-Systeme nutzen.

---

## 🔄 Update: Umstellung auf MQTT (v2.0)
Die alte Version (direkte HTTPS-Befehle an WiiM-Geräte) wurde in den Ordner `legacy_https_version` verschoben. Die aktuelle Version nutzt **MQTT**, was das Projekt deutlich flexibler macht.

---

## ⚠️ Wichtiger Hinweis / Disclaimer
> **Dies ist mein allererstes ESP32-Projekt.**
* Ich lerne ESP-IDF, Bluetooth Classic und L2CAP noch.
* Der Code funktioniert in meinem Setup, aber es gibt **keinerlei Garantie**.
* **Kein Support:** Ich kann keine technische Hilfe anbieten.
* **Nutzung auf eigene Gefahr:** Keine Haftung für Schäden oder Sicherheitslücken.

---

## 🔍 Technischer Ansatz & Debouncing
Das Projekt nutzt keinen Standard-HID-Stack, sondern arbeitet direkt mit dem Bluedroid-Stack:
* **L2CAP Direktzugriff:** Manuelle Registrierung der PSMs `0x11` (Control) und `0x13` (Interrupt).
* **Raw Data Parsing:** Tasten-Bytes werden direkt aus `L2CA_DataInd_Cb` extrahiert.
* **Software Debouncing:** Ein Zeitstempel-Check (`KEY_DEBOUNCE_MS = 200`) verhindert Mehrfach-Auslösungen bei einem Tastendruck.

---

## 🚀 Features
* ✅ **Bluetooth Classic Pairing:** Nutzt SSP (Secure Simple Pairing) mit Auto-Bestätigung.
* ✅ **Software Debouncing:** Filtert Prellen mit einem 200ms Schwellenwert.
* ✅ **MQTT Integration:** Versand von JSON-Daten inkl. Button-Name und Zeitstempel.
* ✅ **WiFi Management:** Automatischer Reconnect bei Verbindungsabbruch.

---

## 🛠 Konfiguration & Installation

### 1. Code anpassen
Passe die Werte in der `firetv_remote2mqtt.c` an:

```c
#define WIFI_SSID      "DEIN_WLAN"
#define WIFI_PASSWORD  "DEIN_PASSWORT"
#define MQTT_BROKER    "mqtt://192.168.xxx.xxx"

// Die MAC-Adresse DEINER Fernbedienung
esp_bd_addr_t remote_bda = {0x00, 0x00, 0x00, 0x8D, 0x00, 0x00}; 
2. Build-Anforderungen
ESP-IDF: v5.x (getestet mit 5.5.2).


Komponenten: bt, nvs_flash und mqtt müssen in der CMakeLists.txt stehen. 

sdkconfig: Bluetooth Classic muss aktiviert sein (beiliegende sdkconfig nutzen).

📡 MQTT Datenformat
Nachrichten werden an das Topic firetv/remote gesendet:

JSON

{
  "button": "PLAY_PAUSE",
  "timestamp": 1245
}
🗺 Tastenbelegung
Navigation: UP, DOWN, LEFT, RIGHT, OK

Media: PLAY_PAUSE, SKIP_FORWARD, SKIP_REWIND

System: HOME, BACK, MENU

🚫 Mikrofon: Nicht unterstützt.

📜 Lizenz
MIT License.
