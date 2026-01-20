# ESP32 Fire TV Remote → WiiM Bridge 📺🎵

This project connects an **Amazon Fire TV Remote (model BE59CV)** to an **ESP32** using **Bluetooth Classic**.
The ESP32 reads the button events **directly from raw L2CAP packets** and translates them into **HTTP commands for a WiiM audio streamer** (e.g. WiiM Mini).

👉 Goal: Use the Fire TV remote as an **alternative / universal remote** for other devices.

---

## ⚠️ Important Notice / Disclaimer

> **This is my very first larger ESP32 project.**

* I am still learning ESP-IDF, Bluetooth Classic, and L2CAP.
* A lot of this project was developed experimentally and with external help.
* The code works for me – **but there is absolutely no guarantee** that it will work for you.

### ❌ No Support

I cannot provide **any technical support**.
If something does not work, I probably do not know the solution either.

### ⚠️ Use at Your Own Risk

Use this project **entirely at your own risk**.
No liability for damage, malfunctions, or security issues.

### 🤝 Help Is Very Welcome

If you have experience with **ESP-IDF**, **Bluetooth Classic**, **HID**, or **L2CAP** and notice mistakes or room for improvement:

👉 **Pull Requests are highly welcome!**
I am happy about any feedback and learning opportunity.

---

## 🤖 Use of AI / Author’s Role

Large parts of this project were created **with the help of Artificial Intelligence (AI)**.

* AI was heavily used for:

  * Code examples and code generation
  * Explanations of ESP-IDF, Bluetooth Classic, and L2CAP
  * Debugging ideas and architectural suggestions

* **My role was primarily that of a project manager**:

  * Defining the idea and overall goal
  * Combining the individual building blocks
  * Testing, tweaking, and iterating
  * Understanding things on a conceptual level (not always every technical detail 😄)

This is therefore **not a traditionally hand-written embedded project**, but the result of:

> *Curiosity, learning, AI assistance, and a lot of trial & error.*

This also explains:

* unconventional solutions
* missing elegance in some places
* why some things work without being perfectly understood

If that bothers you or you know better: **feel free to open a Pull Request 😉**

---

## 🔍 Technical Approach (Very Important!)

**No HID stack. No HID parser. No HID API.**

This project does **not** use the ESP-IDF HID abstraction layer.

The Fire TV remote is **not** handled as a HID device. Instead:

* Bluetooth **Classic** (not BLE)
* Direct access to **L2CAP**
* Manual opening of the HID channels

  * Control Channel → PSM `0x11`
  * Interrupt Channel → PSM `0x13`
* Manual parsing of **raw HID interrupt packets**

Example (simplified):

```
A1 XX YY ZZ 3B
```

Button events are interpreted **solely based on the received bytes**.

### Consequences

* ❌ no HIDH API
* ❌ no HID report descriptor
* ❌ no automatic HID interpretation
* ✅ full control over the raw data

---

## 🚀 Current Features

* ✅ Bluetooth Classic pairing
* ✅ Manual opening of L2CAP channels (control + interrupt)
* ✅ Reading Fire TV remote button data
* ✅ Debouncing & press/release detection
* ✅ Button mapping (navigation & media)
* ✅ Control of a WiiM streamer via **HTTPS API**

  * Play / Pause
  * Next / Previous
  * Volume control (relative, cached)
* 🟡 MQTT / Home Assistant (planned)

---

## 🛠 Requirements

### Hardware

* ESP32 development board
* Amazon Fire TV Remote **BE59CV**
* WiiM Mini / WiiM streamer

### Software

* **ESP-IDF 5.5**
* Bluetooth Classic enabled

### Knowledge

* MAC address of your Fire TV remote
* Wi-Fi credentials
* IP address of your WiiM device

---

## ⚙️ Important `menuconfig` Notes

⚠️ **Very important!**

This project requires a **non-default ESP-IDF configuration**, especially for:

* Bluetooth Classic
* Bluedroid stack
* L2CAP access
* HTTPS / TLS support

👉 A working **`sdkconfig` is included in the repository**.

Please use **this exact configuration**, otherwise you will most likely run into build or link errors.
Default ESP-IDF projects are **not configured for Bluetooth Classic**, and HTTPS/TLS is also often incomplete by default.

> **Note about HTTPS certificates:**
> This project intentionally disables strict certificate validation (e.g. `skip_cert_common_name_check = true`) to keep things simple.
> This is **not secure**, but sufficient for local network control of a WiiM device.

---

## 📦 Installation

### 1️⃣ Adjust the Code

Edit the following values in `main.c`:

```c
#define WIFI_SSID     "YOUR_WIFI_NAME"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define WIIM_IP       "192.168.xxx.xxx"

esp_bd_addr_t remote_bda = {
    0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX
};
```

---

### 2️⃣ Build & Flash

```bash
idf.py build
idf.py flash monitor
```

---

### 3️⃣ Connect

* Power up the ESP32
* Press any button on the Fire TV remote
* Detected button presses should appear in the serial monitor

---

## 🗺 Button Mapping

### Navigation

* ⬆️ UP
* ⬇️ DOWN
* ⬅️ LEFT
* ➡️ RIGHT
* ✅ OK

### Media

* ▶️ Play / Pause
* ⏭ Next
* ⏮ Previous

### System

* 🏠 Home
* 🔙 Back
* ☰ Menu

🚫 **Microphone is not supported**

---

## 📜 License

MIT License

You are free to use, modify, and redistribute this code.
**No warranty. No liability. No promises.**





# ESP32 Fire TV Remote → WiiM Bridge 📺🎵

Dieses Projekt verbindet eine **Amazon Fire TV Fernbedienung (Modell BE59CV)** per **Bluetooth Classic** mit einem **ESP32**.
Der ESP32 liest die Tastensignale **direkt aus rohen L2CAP-Paketen** und übersetzt sie in **HTTP-Befehle für einen WiiM Audio Streamer** (z. B. WiiM Mini).

👉 Ziel: Die Fire-TV-Fernbedienung als **alternative / universelle Fernbedienung** für andere Geräte nutzen.

---

## ⚠️ Wichtiger Hinweis / Disclaimer

> **Dies ist mein allererstes größeres ESP32-Projekt.**

* Ich lerne ESP-IDF, Bluetooth Classic und L2CAP noch.
* Vieles wurde experimentell entwickelt und mit externer Hilfe zusammengetragen.
* Der Code funktioniert bei mir – **aber es gibt keinerlei Garantie**, dass er bei dir genauso funktioniert.

### ❌ Kein Support

Ich kann **keinen technischen Support** leisten.
Wenn etwas nicht funktioniert, weiß ich es vermutlich selbst nicht besser.

### ⚠️ Nutzung auf eigene Gefahr

Die Nutzung erfolgt vollständig **auf eigene Verantwortung**.
Keine Haftung für Schäden, Fehlfunktionen oder Sicherheitsprobleme.

### 🤝 Hilfe ausdrücklich erwünscht

Wenn du dich mit **ESP-IDF**, **Bluetooth Classic**, **HID** oder **L2CAP** auskennst und Fehler oder Verbesserungspotenzial siehst:

👉 **Pull Requests sind sehr willkommen!**
Ich freue mich über jedes Feedback und jede Lernmöglichkeit.

---

## 🤖 Einsatz von KI / Rolle des Autors

Große Teile dieses Projekts wurden **mit Hilfe von Künstlicher Intelligenz (KI)** erstellt.

* KI wurde intensiv genutzt für:

  * Code-Beispiele und Code-Generierung
  * Erklärungen zu ESP-IDF, Bluetooth Classic und L2CAP
  * Debugging-Ideen und Architektur-Vorschläge

* **Meine Rolle war primär die eines Projektmanagers**:

  * Definition der Idee und des Ziels
  * Zusammenführen der einzelnen Bausteine
  * Testen, Anpassen und Iterieren
  * Verständnis auf konzeptioneller Ebene (nicht immer jedes technische Detail 😄)

Dies ist daher **kein klassisch von Hand geschriebenes Embedded-Projekt**, sondern das Ergebnis von:

> *Neugier, Lernen, KI-Unterstützung und sehr viel Trial & Error.*

Das erklärt auch:

* unkonventionelle Lösungsansätze
* fehlende Eleganz an manchen Stellen
* warum manche Dinge funktionieren, ohne vollständig verstanden zu sein

Wenn dich das stört oder du es besser weißt: **Pull Request öffnen 😉**

---

## 🔍 Technischer Ansatz (sehr wichtig!)

**Kein HID-Stack. Kein HID-Parser. Keine HID-API.**

Dieses Projekt nutzt **keine** HID-Abstraktion des ESP-IDF.

Die Fire-TV-Fernbedienung wird **nicht** als HID-Gerät verarbeitet. Stattdessen:

* Bluetooth **Classic** (nicht BLE)
* Direkter Zugriff auf **L2CAP**
* Manuelles Öffnen der HID-Channels

  * Control Channel → PSM `0x11`
  * Interrupt Channel → PSM `0x13`
* Manuelle Auswertung der **rohen HID-Interrupt-Pakete**

Beispiel (vereinfacht):

```
A1 XX YY ZZ 3B
```

Die Tasten werden **ausschließlich anhand der empfangenen Bytes** interpretiert.

### Konsequenzen

* ❌ keine HIDH-API
* ❌ kein HID-Report-Descriptor
* ❌ keine automatische HID-Interpretation
* ✅ maximale Kontrolle über die Rohdaten

---

## 🚀 Aktueller Funktionsumfang

* ✅ Bluetooth Classic Pairing
* ✅ Manuelles Öffnen der L2CAP-Channels (Control + Interrupt)
* ✅ Auslesen der Fire-TV-Tastensignale
* ✅ Debouncing & Press/Release-Erkennung
* ✅ Tasten-Mapping (Navigation & Media)
* ✅ Steuerung eines WiiM Streamers über **HTTPS API**

  * Play / Pause
  * Next / Previous
  * Lautstärkeregelung (relativ, gecacht)
* 🟡 MQTT / Home Assistant (geplant)

---

## 🛠 Voraussetzungen

### Hardware

* ESP32 Development Board
* Amazon Fire TV Fernbedienung **BE59CV**
* WiiM Mini / WiiM Streamer

### Software

* **ESP-IDF 5.5**
* Bluetooth Classic aktiviert

### Wissen

* MAC-Adresse deiner Fire-TV-Fernbedienung
* WLAN-Zugangsdaten
* IP-Adresse deines WiiM Geräts

---

## ⚙️ Wichtige Hinweise zu `menuconfig`

⚠️ **Sehr wichtig!**

Dieses Projekt benötigt eine **nicht-Standard ESP-IDF-Konfiguration**, insbesondere für:

* Bluetooth Classic
* Bluedroid Stack
* L2CAP-Zugriff
* HTTPS / TLS-Unterstützung

👉 Eine funktionierende **`sdkconfig` liegt im Repository bei**.

Bitte verwende **genau diese Konfiguration**, da es sonst sehr wahrscheinlich zu Build- oder Link-Fehlern kommt.
Standard-ESP-IDF-Projekte sind **nicht für Bluetooth Classic vorkonfiguriert**, und HTTPS/TLS ist oft ebenfalls nicht vollständig aktiviert.

> **Hinweis zu HTTPS-Zertifikaten:**
> Dieses Projekt deaktiviert bewusst die strikte Zertifikatsprüfung (z. B. `skip_cert_common_name_check = true`), um die Einrichtung einfach zu halten.
> Das ist **nicht sicher**, für die lokale Steuerung eines WiiM-Geräts im Heimnetz aber ausreichend.

---

## 📦 Installation

### 1️⃣ Code anpassen

Passe in `main.c` folgende Werte an:

```c
#define WIFI_SSID     "DEIN_WLAN_NAME"
#define WIFI_PASSWORD "DEIN_WLAN_PASSWORT"
#define WIIM_IP       "192.168.xxx.xxx"

esp_bd_addr_t remote_bda = {
    0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX
};
```

---

### 2️⃣ Kompilieren & Flashen

```bash
idf.py build
idf.py flash monitor
```

---

### 3️⃣ Verbinden

* ESP32 starten
* Eine Taste auf der Fire-TV-Fernbedienung drücken
* Die erkannten Tastendrücke sollten im seriellen Monitor erscheinen

---

## 🗺 Tastenbelegung

### Navigation

* ⬆️ UP
* ⬇️ DOWN
* ⬅️ LEFT
* ➡️ RIGHT
* ✅ OK

### Media

* ▶️ Play / Pause
* ⏭ Next
* ⏮ Previous

### System

* 🏠 Home
* 🔙 Back
* ☰ Menu

🚫 **Mikrofon wird nicht unterstützt**

---

## 📜 Lizenz

MIT License

Du darfst den Code frei nutzen, verändern und weiterverbreiten.
**Ohne Garantie. Ohne Haftung. Ohne Versprechen.**
