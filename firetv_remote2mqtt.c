#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_bt_device.h"
#include "osi/allocator.h"
#include "esp_timer.h"

// WiFi
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"

// MQTT
#include "mqtt_client.h"

// L2CAP Zugriff
#include "stack/l2c_api.h"
#include "stack/bt_types.h"
#include "stack/btm_api.h"

// ========== KONFIGURATION ==========
#define WIFI_SSID      "DEIN WIFI"      // <-- HIER ÄNDERN!
#define WIFI_PASSWORD  "DEIN PASSWORT"  // <-- HIER ÄNDERN!
#define MQTT_BROKER    "mqtt://192.XXX.XXX.XX" // <-- HIER ÄNDERN! (IP Home Assistant)
#define MQTT_USERNAME  "DEIN BENUTZERNAME"      // <-- MQTT Username (falls aktiviert)
#define MQTT_PASSWORD  "DEIN PASSWORT"  // <-- MQTT Passwort (falls aktiviert)
#define MQTT_TOPIC     "firetv/remote"        // <-- Optional anpassen
// ===================================

static const char *TAG = "FIRE_TV_L2CAP";

// Fire TV Remote Adresse muss angepasst werden
esp_bd_addr_t remote_bda = {0x00, 0x00, 0x00, 0x8d, 0x00, 0x00};  // <-- Hier ÄNDER!

// L2CAP PSM
#define HID_PSM_CONTROL    0x0011
#define HID_PSM_INTERRUPT  0x0013

// L2CAP Connection IDs
static uint16_t l2cap_handle_ctrl = 0;
static uint16_t l2cap_handle_intr = 0;
static bool connected = false;

// Debouncing
static uint8_t last_key_byte1 = 0;
static uint8_t last_key_byte2 = 0;
static uint32_t last_key_time = 0;
#define KEY_DEBOUNCE_MS 200

// WiFi & MQTT
static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool mqtt_connected = false;

// Fire TV Tastencode Mapping
const char* get_key_name_byte1(uint8_t key_code) {
    switch(key_code) {
        case 0x04: return "LEFT";
        case 0x08: return "SKIP_REWIND";
        case 0x10: return "MICROPHONE";
        case 0x20: return "BACK";
        case 0x40: return "OK";
        case 0x80: return "PLAY_PAUSE";
        default: return NULL;
    }
}

const char* get_key_name_byte2(uint8_t key_code) {
    switch(key_code) {
        case 0x02: return "UP";
        case 0x04: return "RIGHT";
        case 0x08: return "SKIP_FORWARD";
        case 0x20: return "HOME";
        case 0x40: return "DOWN";
        case 0x80: return "MENU";
        default: return NULL;
    }
}

// MQTT Nachricht senden
void send_mqtt_key(const char* key_name) {
    if (!mqtt_connected || mqtt_client == NULL) {
        ESP_LOGW(TAG, "MQTT nicht verbunden - Key nicht gesendet: %s", key_name);
        return;
    }
    
    // JSON Payload erstellen
    char payload[128];
    snprintf(payload, sizeof(payload), 
             "{\"button\":\"%s\",\"timestamp\":%lld}", 
             key_name, 
             (long long)(esp_timer_get_time() / 1000000));
    
    int msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC, payload, 0, 1, 0);
    
    if (msg_id >= 0) {
        ESP_LOGI(TAG, "📤 MQTT gesendet: %s (msg_id=%d)", key_name, msg_id);
    } else {
        ESP_LOGE(TAG, "❌ MQTT Fehler beim Senden: %s", key_name);
    }
}

void process_hid_data(uint8_t *data, uint16_t len) {
    ESP_LOG_BUFFER_HEX(TAG, data, len);
    
    if (len >= 5) {
        uint8_t key_byte1 = data[1];
        uint8_t key_byte2 = data[2];
        
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        // Debouncing
        if (key_byte1 == last_key_byte1 && key_byte2 == last_key_byte2) {
            if ((now - last_key_time) < KEY_DEBOUNCE_MS) {
                return;
            }
        }
        
        last_key_byte1 = key_byte1;
        last_key_byte2 = key_byte2;
        last_key_time = now;
        
        const char* key_name = NULL;
        
        if (key_byte1 != 0x00) {
            key_name = get_key_name_byte1(key_byte1);
        } else if (key_byte2 != 0x00) {
            key_name = get_key_name_byte2(key_byte2);
        }
        
        if (key_name != NULL) {
            ESP_LOGI(TAG, "");
            ESP_LOGI(TAG, "╔═══════════════════════════════╗");
            ESP_LOGI(TAG, "║  🎮 %s", key_name);
            ESP_LOGI(TAG, "╚═══════════════════════════════╝");
            ESP_LOGI(TAG, "");
            
            // MQTT senden
            send_mqtt_key(key_name);
        }
    }
}

// L2CAP Data Callback
void l2cap_data_callback(uint16_t lcid, BT_HDR *p_buf) {
    if (p_buf == NULL) return;
    
    uint8_t *data = (uint8_t *)(p_buf + 1) + p_buf->offset;
    uint16_t len = p_buf->len;
    
    if (lcid == l2cap_handle_intr) {
        process_hid_data(data, len);
    } else if (lcid == l2cap_handle_ctrl) {
        ESP_LOGD(TAG, "Control Channel Data:");
        ESP_LOG_BUFFER_HEX(TAG, data, len);
    }
    
    osi_free(p_buf);
}

// L2CAP Connect Indication
void l2cap_connect_ind_callback(BD_ADDR bd_addr, uint16_t lcid, uint16_t psm, uint8_t id) {
    ESP_LOGI(TAG, "L2CAP Connect Request: PSM=0x%04X, LCID=0x%04X", psm, lcid);
    
    tL2CAP_CFG_INFO cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.mtu_present = true;
    cfg.mtu = 672;
    
    L2CA_ConnectRsp(bd_addr, id, lcid, L2CAP_CONN_OK, 0);
    L2CA_ConfigReq(lcid, &cfg);
    
    if (psm == HID_PSM_CONTROL) {
        l2cap_handle_ctrl = lcid;
        ESP_LOGI(TAG, "✅ Control Channel verbunden");
    } else if (psm == HID_PSM_INTERRUPT) {
        l2cap_handle_intr = lcid;
        ESP_LOGI(TAG, "✅ Interrupt Channel verbunden");
    }
}

// L2CAP Connect Confirmation
void l2cap_connect_cfm_callback(uint16_t lcid, uint16_t result) {
    ESP_LOGI(TAG, "L2CAP Connect Confirm: LCID=0x%04X, Result=%d", lcid, result);
    
    if (result == L2CAP_CONN_OK) {
        tL2CAP_CFG_INFO cfg;
        memset(&cfg, 0, sizeof(cfg));
        cfg.mtu_present = true;
        cfg.mtu = 672;
        
        L2CA_ConfigReq(lcid, &cfg);
    }
}

// L2CAP Config Indication
void l2cap_config_ind_callback(uint16_t lcid, tL2CAP_CFG_INFO *p_cfg) {
    ESP_LOGD(TAG, "L2CAP Config Ind: LCID=0x%04X", lcid);
    
    tL2CAP_CFG_INFO cfg;
    memset(&cfg, 0, sizeof(cfg));
    L2CA_ConfigRsp(lcid, &cfg);
}

// L2CAP Config Confirmation
void l2cap_config_cfm_callback(uint16_t lcid, tL2CAP_CFG_INFO *p_cfg) {
    ESP_LOGI(TAG, "L2CAP Config Complete: LCID=0x%04X", lcid);
    
    if (lcid == l2cap_handle_ctrl || lcid == l2cap_handle_intr) {
        if (l2cap_handle_ctrl != 0 && l2cap_handle_intr != 0) {
            connected = true;
            ESP_LOGI(TAG, "");
            ESP_LOGI(TAG, "╔═════════════════════════════════════╗");
            ESP_LOGI(TAG, "║  🔥 FIRE TV REMOTE EINSATZBEREIT!  ║");
            ESP_LOGI(TAG, "╚═════════════════════════════════════╝");
            ESP_LOGI(TAG, "");
        }
    }
}

// L2CAP Disconnect Indication
void l2cap_disconnect_ind_callback(uint16_t lcid, bool ack_needed) {
    ESP_LOGW(TAG, "L2CAP Disconnect: LCID=0x%04X", lcid);
    
    if (lcid == l2cap_handle_ctrl) {
        l2cap_handle_ctrl = 0;
    } else if (lcid == l2cap_handle_intr) {
        l2cap_handle_intr = 0;
    }
    
    connected = false;
}

// L2CAP Disconnect Confirmation
void l2cap_disconnect_cfm_callback(uint16_t lcid, uint16_t result) {
    ESP_LOGW(TAG, "L2CAP Disconnect Confirm: LCID=0x%04X", lcid);
}

// Registriere L2CAP Callbacks
void register_l2cap_callbacks(uint16_t psm) {
    tL2CAP_APPL_INFO appl_info;
    memset(&appl_info, 0, sizeof(appl_info));
    
    appl_info.pL2CA_ConnectInd_Cb = l2cap_connect_ind_callback;
    appl_info.pL2CA_ConnectCfm_Cb = l2cap_connect_cfm_callback;
    appl_info.pL2CA_ConfigInd_Cb = l2cap_config_ind_callback;
    appl_info.pL2CA_ConfigCfm_Cb = l2cap_config_cfm_callback;
    appl_info.pL2CA_DisconnectInd_Cb = l2cap_disconnect_ind_callback;
    appl_info.pL2CA_DisconnectCfm_Cb = l2cap_disconnect_cfm_callback;
    appl_info.pL2CA_DataInd_Cb = l2cap_data_callback;
    
    uint16_t result = L2CA_Register(psm, &appl_info);
    ESP_LOGI(TAG, "L2CA_Register(PSM=0x%04X) = 0x%04X", psm, result);
}

// GAP Callback für Pairing
void gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
    switch (event) {
        case ESP_BT_GAP_AUTH_CMPL_EVT:
            if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
                ESP_LOGI(TAG, "✅ Pairing erfolgreich!");
                ESP_LOGI(TAG, "🔐 Bonding Key im NVS gespeichert");
                
                // Link Keys loggen (Debug)
                ESP_LOG_BUFFER_HEX_LEVEL(TAG, param->auth_cmpl.bda, ESP_BD_ADDR_LEN, ESP_LOG_INFO);
                
                ESP_LOGI(TAG, "Öffne L2CAP Channels...");
                
                l2cap_handle_ctrl = L2CA_ConnectReq(HID_PSM_CONTROL, remote_bda);
                vTaskDelay(pdMS_TO_TICKS(100));
                l2cap_handle_intr = L2CA_ConnectReq(HID_PSM_INTERRUPT, remote_bda);
                
            } else {
                ESP_LOGE(TAG, "❌ Pairing fehlgeschlagen: %d", param->auth_cmpl.stat);
            }
            break;

        case ESP_BT_GAP_CFM_REQ_EVT:
            ESP_LOGI(TAG, "SSP Bestätigung - Auto-Accept");
            esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
            break;

        case ESP_BT_GAP_PIN_REQ_EVT: {
            ESP_LOGI(TAG, "PIN angefordert - sende 0000");
            esp_bt_pin_code_t pin = {0, 0, 0, 0};
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin);
            break;
        }

        default:
            break;
    }
}

// Discovery Callback - findet die Remote
void gap_discovery_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
    switch (event) {
        case ESP_BT_GAP_DISC_RES_EVT: {
            if (memcmp(param->disc_res.bda, remote_bda, ESP_BD_ADDR_LEN) == 0) {
                ESP_LOGI(TAG, "✅ Fire TV Remote gefunden!");
                ESP_LOGI(TAG, "🔗 Starte Pairing-Vorbereitung...");
                
                // Discovery stoppen für stabilere Verbindung
                esp_bt_gap_cancel_discovery(); 
                
                // Pairing via SSP bestätigen
                esp_bt_gap_ssp_confirm_reply(remote_bda, true);
                
                // Verbindung triggern
                l2cap_handle_ctrl = L2CA_ConnectReq(HID_PSM_CONTROL, remote_bda);
            }
            break;
        }
        
        case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
            if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED) {
                ESP_LOGI(TAG, "Discovery beendet");
                if (!connected) {
                    ESP_LOGI(TAG, "Noch nicht verbunden, warte kurz und suche erneut...");
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
                }
            } else if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STARTED) {
                ESP_LOGI(TAG, "🔍 Suche nach Geräten...");
            }
            break;
            
        default:
            break;
    }
}

// ========== WiFi Event Handler ==========
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "WiFi getrennt - versuche Reconnect...");
        mqtt_connected = false;
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "✅ WiFi verbunden! IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// WiFi initialisieren
void wifi_init(void) {
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

// ========== MQTT Event Handler ==========
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "✅ MQTT verbunden!");
            mqtt_connected = true;
            esp_mqtt_client_publish(mqtt_client, "firetv/status", "online", 0, 1, 1);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT getrennt");
            mqtt_connected = false;
            break;
        default: break;
    }
}

void mqtt_init(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER,
        .credentials = {
            .username = MQTT_USERNAME,
            .authentication.password = MQTT_PASSWORD,
        },
    };
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

// ========== MAIN ======================================================================================
void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "╔═════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  ESP32 Fire TV → MQTT Bridge v2.4  ║");
    ESP_LOGI(TAG, "╚═════════════════════════════════════╝");

    wifi_init();
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
    mqtt_init();
    vTaskDelay(pdMS_TO_TICKS(2000));

    // BT Controller & Bluedroid
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    // GAP Register
    esp_bt_gap_register_callback(gap_callback);
    esp_bt_gap_register_callback(gap_discovery_callback);

    // ========== BONDING KONFIGURATION ==========
    ESP_LOGI(TAG, "🔐 Konfiguriere Bluetooth Bonding...");
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_NONE;
    esp_bt_gap_set_security_param(ESP_BT_SP_IOCAP_MODE, &iocap, sizeof(uint8_t));
    
    // Auth Mode: BONDING (Index 0 in aktuellen ESP-IDF Versionen oft als SP_AUTH_MODE genutzt)
    uint8_t auth_req = 0x03; 
    esp_bt_gap_set_security_param(0, &auth_req, sizeof(uint8_t)); 
    esp_bt_gap_set_pin(ESP_BT_PIN_TYPE_FIXED, 0, NULL);
    // ===========================================

    esp_bt_gap_set_device_name("ESP32_FireTV_MQTT");
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);

    // L2CAP Security & Callbacks
    BTM_SetSecurityLevel(false, "", BTM_SEC_SERVICE_HIDD_INTR, BTM_SEC_NONE, HID_PSM_INTERRUPT, 0, 0);
    BTM_SetSecurityLevel(false, "", BTM_SEC_SERVICE_HCRP_CTRL, BTM_SEC_NONE, HID_PSM_CONTROL, 0, 0);
    register_l2cap_callbacks(HID_PSM_CONTROL);
    register_l2cap_callbacks(HID_PSM_INTERRUPT);

    ESP_LOGI(TAG, "🔍 Starte aktive Suche nach Remote...");
    esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);

    while (1) {
        if (!connected) {
            // Watchdog: Falls nicht verbunden, alle 10 Sekunden Suche sicherstellen
            esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 5, 0);
        } else {
            static int heartbeat = 0;
            if (++heartbeat % 10 == 0) ESP_LOGI(TAG, "💚 Aktiv & Verbunden");
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
    // ... am Ende von app_main ...
    ESP_LOGI(TAG, "🔍 Starte aktive Suche und Reconnect-Loop...");
    
    while (1) {
        if (!connected) {
            // Wenn nicht verbunden, wird versucht aktiv die Kanäle zu öffnen
            // Das triggert bei Classic BT den Page Scan
            L2CA_ConnectReq(HID_PSM_CONTROL, remote_bda);
            
            // Parallel suchen, falls die Remote im Discovery-Mode ist
            esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 5, 0);
            
            ESP_LOGD(TAG, "Versuche Reconnect zu %02x:%02x:%02x...", 
                     remote_bda[0], remote_bda[1], remote_bda[2]);
        } else {
            static int heartbeat = 0;
            if (++heartbeat % 12 == 0) { // Alle 60 Sek
                ESP_LOGI(TAG, "💚 Verbindung steht. Warte auf Tastendruck...");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5000)); // Alle 5 Sekunden prüfen
    }
}
