# ESP32 FireTV Remote Bridge 📺🔋

Dieses Projekt ermöglicht es, eine **Amazon Fire TV Fernbedienung (Modell BE59CV)** per Bluetooth mit einem **ESP32** zu verbinden. Der ESP32 fängt die Tastensignale ab, damit man damit andere Geräte (wie den **WiiM Mini**) steuern kann.

## ⚠️ Wichtiger Hinweis (Disclaimer)
**Dies ist mein allererstes Projekt.** Ich lerne gerade erst, wie das alles funktioniert. Ich habe mir vieles zusammengesucht und bin froh, dass es läuft – aber ich habe oft selbst keine Ahnung, warum der Code funktioniert (oder warum nicht).

* **Kein Support:** Ich kann absolut **keinen technischen Support** bieten. Wenn bei dir etwas nicht funktioniert, weiß ich wahrscheinlich leider auch nicht, woran es liegt. Ich nutze viel KI für technische Fragen.
* **Nutzung auf eigene Gefahr:** Ich übernehme keine Garantie für die Funktion oder Sicherheit.
* **Helfende Hände gesucht:** Wenn du Ahnung von ESP-IDF oder Bluetooth hast und siehst, dass ich Fehler gemacht habe – bitte erstelle einen Pull Request! Ich freue mich über jede Hilfe, um zu lernen.

🔍 Hinweis: Keine HID‑Schicht, keine HID‑Parser – reine L2CAP‑Rohdaten
Dieses Projekt verwendet keinen HID‑Stack und keinen HID‑Parser des ESP32.
Die Fire‑TV‑Fernbedienung wird nicht als HID‑Gerät verarbeitet. Stattdessen liest der ESP32 die Tastensignale direkt als rohe L2CAP‑Pakete vom HID‑Interrupt‑Channel (PSM 0x13) und wertet die relevanten Bytes manuell aus.

Das bedeutet:
-keine HIDH‑API
-kein HID‑Report‑Descriptor
-keine automatische HID‑Interpretation
-die Tasten werden ausschließlich anhand der empfangenen Bytes (A1 XX YY ZZ 3B) interpretiert

---

## 🚀 Aktueller Stand
- [x] Bluetooth Classic Pairing
- [x] Auslesen der Tastendaten über L2CAP Channels
- [x] Mapping der Tasten (Home, Back, OK, Navigation, etc.)
- [x] Integration der WiiM Mini HTTPS API  (Play,Next,Previous,Volume)
- [ ] MQTT Unterstützung für Home Assistant (geplant)

## 🛠 Voraussetzungen
* **Hardware:** ESP32 Development Board.
* **Software:** ESP-IDF 5.5 Framework.
* **Fernbedienung:** Amazon Fire TV Remote (Modell BE59CV).
* **Wissen:** Du musst die MAC-Adresse deiner Fernbedienung kennen.

## 📦 Installation

1.  **Code anpassen:**
    Öffne die `main.c` und trage deine MAC-Adresse ein:
    ```c
    esp_bd_addr_t remote_bda = {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX};
    ```

2.  **Kompilieren und Flashen:**
    ```bash
    idf.py build
    idf.py flash monitor
    ```

3.  **Verbinden:**
    Drücke eine Taste auf der Fernbedienung, während der ESP32 läuft. Im Terminal-Monitor sollten die gedrückten Tasten erscheinen.

## 🗺 Key Mapping
Der Code erkennt folgende Tasten:
* **Steuerkreuz:** Hoch, Runter, Links, Rechts, OK
* **Media:** Play/Pause, Vorspulen, Zurückspulen
* **System:** Home, Zurück, Menü,
  
* Mikrofon wird nicht unterstützt!

## 📜 Lizenz
Dieses Projekt steht unter der **MIT-Lizenz**. Das bedeutet: Du kannst damit machen, was du willst, aber es gibt keinerlei Garantie oder Haftung meinerseits.
