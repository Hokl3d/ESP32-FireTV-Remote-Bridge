
ESP32 Fire TV Remote → MQTT Bridge 📺📡

Version v0.2
Bluetooth Classic (L2CAP) → WiFi → MQTT


---

🇬🇧 English Version

📌 Overview

This project connects an Amazon Fire TV Remote (Model BE59CV- Bluetooth Classic) directly to an ESP32.
The ESP32 listens to raw L2CAP HID traffic, extracts button presses, and publishes them as JSON messages via MQTT.

👉 Goal: Use a Fire TV remote as a physical input device for Home Assistant or any other MQTT-based smart home system.


---

🔄 Version History

v0.2 (current)
✅ MQTT-based architecture
✅ Improved flexibility for smart home setups

v0.1 (legacy)
🔁 Direct HTTPS commands to WiiM devices
📁 Available in the folder: legacy_https_version



---

⚠️ Very Important Notice (ESP-IDF Version)

🚨 This project was developed and tested exclusively with:

ESP-IDF v5.5.2


❗ Other ESP-IDF versions will very likely NOT work.
Bluetooth Classic, Bluedroid, and internal L2CAP APIs are highly version-sensitive.

✅ You MUST use:

The provided CMakeLists.txt

The provided sdkconfig / menuconfig


Many required Bluetooth Classic and Bluedroid options were manually configured in menuconfig.
Trying to recreate these settings manually is strongly discouraged.


---

⚠️ Disclaimer

> This is my first ESP32 project.



I am still learning ESP-IDF, Bluetooth Classic, and L2CAP internals.

The code works in my environment, but there is no guarantee it will work in yours.

❌ No support is provided.

⚠️ Use at your own risk — no liability for damage or security issues.



---

🤖 AI-Assisted Development Notice

This project was created with the assistance of AI tools.

Parts of the code may not follow best practices or be optimally structured.

The focus was on functionality and learning, not on perfect architecture or elegance.

I do not fully understand every low-level technical detail, especially within:

Bluetooth Classic internals

Bluedroid / L2CAP APIs



The project is shared as-is, mainly for experimentation, learning, and inspiration.


---

🔍 Technical Details

Bluetooth Classic (Bluedroid)

Direct L2CAP access

Control PSM: 0x11

Interrupt PSM: 0x13


Raw HID parsing

Button bytes extracted directly from L2CA_DataInd_Cb


Software debouncing

KEY_DEBOUNCE_MS = 200


WiFi + MQTT

JSON messages with button name + timestamp




---

🚀 Features

✅ Bluetooth Classic pairing (SSP auto-accept)

✅ Direct L2CAP HID handling (no HID abstraction layer)

✅ Software debouncing (200 ms)

✅ MQTT publishing (JSON)

✅ Automatic WiFi reconnection



---

🛠 Configuration & Installation

1️⃣ Requirements

ESP32 with Bluetooth Classic

ESP-IDF v5.5.2 (mandatory)

MQTT Broker (e.g. Mosquitto, Home Assistant)



---

2️⃣ Use the Provided Configuration

⚠️ Do NOT skip this step

Use the included:

CMakeLists.txt

sdkconfig


Do NOT modify menuconfig unless you fully understand the Bluetooth options



---

3️⃣ Adjust Network & Device Settings

Edit firetv_remote2mqtt.c:

#define WIFI_SSID      "YOUR_WIFI"
#define WIFI_PASSWORD  "YOUR_PASSWORD"
#define MQTT_BROKER    "mqtt://192.168.xxx.xxx"

// MAC address of your Fire TV remote
esp_bd_addr_t remote_bda = {0x00, 0x00, 0x00, 0x8D, 0x00, 0x00};


---

📡 MQTT Payload

Topic: firetv/remote

{
  "button": "OK",
  "timestamp": 1710000000
}


---

🗺 Button Mapping

Navigation: UP, DOWN, LEFT, RIGHT, OK
Media: PLAY_PAUSE, SKIP_FORWARD, SKIP_REWIND
System: HOME, BACK, MENU

🚫 Microphone button is not supported


---

📜 License

MIT License


---


---

🇩🇪 Deutsche Version

📌 Übersicht

Dieses Projekt verbindet eine Amazon Fire TV Fernbedienung (Model BE59CV- Bluetooth Classic) direkt mit einem ESP32.
Der ESP32 liest rohe L2CAP-HID-Daten, erkennt Tastendrücke und sendet diese als JSON per MQTT.

👉 Ziel: Die Fire-TV-Fernbedienung als Eingabegerät für Home Assistant oder andere MQTT-basierte Systeme nutzen.


---

🔄 Versionshistorie

v0.2 (aktuell)
✅ MQTT-basierte Architektur
✅ Flexible Integration in Smart-Home-Systeme

v0.1 (Legacy)
🔁 Direkte HTTPS-Kommandos an WiiM-Geräte
📁 Im Ordner legacy_https_version verfügbar



---

⚠️ Sehr wichtiger Hinweis (ESP-IDF Version)

🚨 Dieses Projekt wurde ausschließlich mit folgender Version entwickelt und getestet:

ESP-IDF v5.5.2


❗ Andere ESP-IDF-Versionen funktionieren sehr wahrscheinlich NICHT.
Bluetooth Classic, Bluedroid und interne L2CAP-APIs sind extrem versionsabhängig.

✅ Zwingend erforderlich:

Die mitgelieferte CMakeLists.txt

Die mitgelieferte sdkconfig / menuconfig


Viele notwendige Bluetooth-Optionen wurden manuell gesetzt.
Eine manuelle Nachkonfiguration wird nicht empfohlen.


---

⚠️ Haftungsausschluss

> Dies ist mein erstes ESP32-Projekt.



Ich lerne ESP-IDF, Bluetooth Classic und L2CAP noch.

Der Code funktioniert in meinem Setup, aber ohne Garantie.

❌ Kein Support

⚠️ Nutzung auf eigene Gefahr



---

🤖 Hinweis zur KI-gestützten Entwicklung

Dieses Projekt wurde mit Unterstützung von KI-Tools erstellt.

Teile des Codes sind möglicherweise nicht elegant oder optimal strukturiert.

Der Fokus lag auf Funktionalität und Lernprozess, nicht auf perfektem Code-Design.

Ich verstehe nicht jedes Low-Level-Detail vollständig, insbesondere im Bereich:

Bluetooth Classic Internals

Bluedroid / L2CAP APIs



Das Projekt wird ohne Anspruch auf Perfektion und ausschließlich zu Lern- und Experimentierzwecken veröffentlicht.


---

🔍 Technischer Ansatz

Bluetooth Classic (Bluedroid)

Direkter L2CAP-Zugriff

Control PSM: 0x11

Interrupt PSM: 0x13


Raw HID Parsing

Software-Debouncing (200 ms)

WiFi + MQTT



---

🚀 Features

✅ Bluetooth Classic Pairing (SSP Auto-Accept)

✅ Direkter L2CAP HID Zugriff

✅ MQTT (JSON)

✅ Automatischer WiFi-Reconnect



---

📜 Lizenz

MIT License


---



















ESP32 Fire TV Remote → MQTT Bridge 📺📡
Version v0.2
Bluetooth Classic (L2CAP) → WiFi → MQTT
🇬🇧 English Version
📌 Overview
This project connects an Amazon Fire TV Remote (Bluetooth Classic) directly to an ESP32.
The ESP32 listens to raw L2CAP HID traffic, extracts button presses, and publishes them as JSON messages via MQTT.
👉 Goal: Use a Fire TV remote as a physical input device for Home Assistant or any other MQTT-based smart home system.
🔄 Version History
v0.2 (current)
✅ MQTT-based architecture
✅ Improved flexibility for smart home setups
v0.1 (legacy)
🔁 Direct HTTPS commands to WiiM devices
📁 Available in the folder: legacy_https_version
⚠️ Very Important Notice (ESP-IDF Version)
🚨 This project was developed and tested exclusively with:
ESP-IDF v5.5.2
❗ Other ESP-IDF versions will very likely NOT work.
Bluetooth Classic, Bluedroid, and internal L2CAP APIs are highly version-sensitive.
✅ You MUST use:
The provided CMakeLists.txt
The provided sdkconfig / menuconfig
Many required Bluetooth Classic and Bluedroid options were manually configured in menuconfig.
Trying to recreate these settings manually is strongly discouraged.
⚠️ Disclaimer
This is my first ESP32 project.
I am still learning ESP-IDF, Bluetooth Classic, and L2CAP internals.
The code works in my environment, but there is no guarantee it will work in yours.
❌ No support is provided.
⚠️ Use at your own risk — no liability for damage or security issues.
🤖 AI-Assisted Development Notice
This project was created with the assistance of AI tools.
Parts of the code may not follow best practices or be optimally structured.
The focus was on functionality and learning, not on perfect architecture or elegance.
I do not fully understand every low-level technical detail, especially within:
Bluetooth Classic internals
Bluedroid / L2CAP APIs
The project is shared as-is, mainly for experimentation, learning, and inspiration.
🔍 Technical Details
Bluetooth Classic (Bluedroid)
Direct L2CAP access
Control PSM: 0x11
Interrupt PSM: 0x13
Raw HID parsing
Button bytes extracted directly from L2CA_DataInd_Cb
Software debouncing
KEY_DEBOUNCE_MS = 200
WiFi + MQTT
JSON messages with button name + timestamp
🚀 Features
✅ Bluetooth Classic pairing (SSP auto-accept)
✅ Direct L2CAP HID handling (no HID abstraction layer)
✅ Software debouncing (200 ms)
✅ MQTT publishing (JSON)
✅ Automatic WiFi reconnection
🛠 Configuration & Installation
1️⃣ Requirements
ESP32 with Bluetooth Classic
ESP-IDF v5.5.2 (mandatory)
MQTT Broker (e.g. Mosquitto, Home Assistant)
2️⃣ Use the Provided Configuration
⚠️ Do NOT skip this step
Use the included:
CMakeLists.txt
sdkconfig
Do NOT modify menuconfig unless you fully understand the Bluetooth options
3️⃣ Adjust Network & Device Settings
Edit firetv_remote2mqtt.c:
Code kopieren
C
#define WIFI_SSID      "YOUR_WIFI"
#define WIFI_PASSWORD  "YOUR_PASSWORD"
#define MQTT_BROKER    "mqtt://192.168.xxx.xxx"

// MAC address of your Fire TV remote
esp_bd_addr_t remote_bda = {0x00, 0x00, 0x00, 0x8D, 0x00, 0x00};
📡 MQTT Payload
Topic: firetv/remote
Code kopieren
Json
{
  "button": "OK",
  "timestamp": 1710000000
}
🗺 Button Mapping
Navigation: UP, DOWN, LEFT, RIGHT, OK
Media: PLAY_PAUSE, SKIP_FORWARD, SKIP_REWIND
System: HOME, BACK, MENU
🚫 Microphone button is not supported
📜 License
MIT License
🇩🇪 Deutsche Version
📌 Übersicht
Dieses Projekt verbindet eine Amazon Fire TV Fernbedienung (Bluetooth Classic) direkt mit einem ESP32.
Der ESP32 liest rohe L2CAP-HID-Daten, erkennt Tastendrücke und sendet diese als JSON per MQTT.
👉 Ziel: Die Fire-TV-Fernbedienung als Eingabegerät für Home Assistant oder andere MQTT-basierte Systeme nutzen.
🔄 Versionshistorie
v0.2 (aktuell)
✅ MQTT-basierte Architektur
✅ Flexible Integration in Smart-Home-Systeme
v0.1 (Legacy)
🔁 Direkte HTTPS-Kommandos an WiiM-Geräte
📁 Im Ordner legacy_https_version verfügbar
⚠️ Sehr wichtiger Hinweis (ESP-IDF Version)
🚨 Dieses Projekt wurde ausschließlich mit folgender Version entwickelt und getestet:
ESP-IDF v5.5.2
❗ Andere ESP-IDF-Versionen funktionieren sehr wahrscheinlich NICHT.
Bluetooth Classic, Bluedroid und interne L2CAP-APIs sind extrem versionsabhängig.
✅ Zwingend erforderlich:
Die mitgelieferte CMakeLists.txt
Die mitgelieferte sdkconfig / menuconfig
Viele notwendige Bluetooth-Optionen wurden manuell gesetzt.
Eine manuelle Nachkonfiguration wird nicht empfohlen.
⚠️ Haftungsausschluss
Dies ist mein erstes ESP32-Projekt.
Ich lerne ESP-IDF, Bluetooth Classic und L2CAP noch.
Der Code funktioniert in meinem Setup, aber ohne Garantie.
❌ Kein Support
⚠️ Nutzung auf eigene Gefahr
🤖 Hinweis zur KI-gestützten Entwicklung
Dieses Projekt wurde mit Unterstützung von KI-Tools erstellt.
Teile des Codes sind möglicherweise nicht elegant oder optimal strukturiert.
Der Fokus lag auf Funktionalität und Lernprozess, nicht auf perfektem Code-Design.
Ich verstehe nicht jedes Low-Level-Detail vollständig, insbesondere im Bereich:
Bluetooth Classic Internals
Bluedroid / L2CAP APIs
Das Projekt wird ohne Anspruch auf Perfektion und ausschließlich zu Lern- und Experimentierzwecken veröffentlicht.
🔍 Technischer Ansatz
Bluetooth Classic (Bluedroid)
Direkter L2CAP-Zugriff
Control PSM: 0x11
Interrupt PSM: 0x13
Raw HID Parsing
Software-Debouncing (200 ms)
WiFi + MQTT
🚀 Features
✅ Bluetooth Classic Pairing (SSP Auto-Accept)
✅ Direkter L2CAP HID Zugriff
✅ MQTT (JSON)
✅ Automatischer WiFi-Reconnect
📜 Lizenz
MIT License

