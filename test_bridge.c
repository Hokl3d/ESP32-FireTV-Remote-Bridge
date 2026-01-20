/******************************************************************************
 * @file           test_bridge.c
 * @brief          ESP32 Bridge: Fire TV Remote (L2CAP) to WiiM Audio (HTTP API)
 * @author         Dein Name Hokl3d
 * @date           2024-05-22
 * @version        3.0
 * @requirements   ESP-IDF Version 5.5
 * * @copyright      Copyright (c) 2026 Hokl3d
 * * @license        MIT License
 * * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * * @details        Diese Software ermöglicht die Steuerung eines WiiM Streamers
 * über eine Amazon Fire TV Fernbedienung mittels Bluetooth 
 * Classic L2CAP Channels. Optimiert für ESP-IDF 5.5.
 ******************************************************************************/

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

// WLAN
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"

// HTTP Client
#include "esp_http_client.h"
#include "esp_tls.h"

// L2CAP Zugriff
#include "stack/l2c_api.h"
#include "stack/bt_types.h"
#include "stack/btm_api.h"

static const char *TAG = "FIRE_TV_L2CAP";

// ==================== KONFIGURATION (PLATZHALTER) ====================

// WLAN Einstellungen
#define WIFI_SSID      "DEIN_WLAN_NAME"
#define WIFI_PASSWORD  "DEIN_WLAN_PASSWORT"

// WiiM Einstellungen
#define WIIM_IP        "192.168.xxx.xxx"

// Fire TV Remote Adresse (MAC-Adresse)
esp_bd_addr_t remote_bda = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};



// ... ( restlichen Codes folgt hier unverändert)


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

// WLAN
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"

// HTTP Client
#include "esp_http_client.h"
#include "esp_tls.h"

// L2CAP Zugriff
#include "stack/l2c_api.h"
#include "stack/bt_types.h"
#include "stack/btm_api.h"

static const char *TAG = "FIRE_TV_L2CAP";

// ==================== KONFIGURATION ====================
// WLAN Einstellungen - HIER ANPASSEN!
#define WIFI_SSID      "Dein Wlan"
#define WIFI_PASSWORD  "Dein Passwort"

// WiiM Einstellungen - HIER ANPASSEN!
#define WIIM_IP        "10.10.10.254"

// Fire TV Remote Adresse - HIER ANPASSEN!
esp_bd_addr_t remote_bda = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// L2CAP PSM
#define HID_PSM_CONTROL    0x0011
#define HID_PSM_INTERRUPT  0x0013

// L2CAP Connection IDs
static uint16_t l2cap_handle_ctrl = 0;
static uint16_t l2cap_handle_intr = 0;
static bool bt_connected = false;

// WLAN Status
static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
static bool wifi_connected = false;

// Debouncing Einstellungen
static uint8_t last_key_byte1 = 0;
static uint8_t last_key_byte2 = 0;
static uint32_t last_key_time = 0;
#define KEY_DEBOUNCE_MS 500  // Erhöht von 200ms auf 500ms

// Optional: Auch Release-Events filtern
static bool key_is_pressed = false;

// ==================== WIIM VOLUME CONTROL ====================

// Buffer für HTTP Response
#define HTTP_RESPONSE_BUFFER_SIZE 512
static char http_response_buffer[HTTP_RESPONSE_BUFFER_SIZE];
static int http_response_len = 0;

// Aktueller Volume-Wert Cache (verhindert verzögerung durch Handshake)
static int current_volume = -1;
static uint32_t last_volume_query_time = 0;
#define VOLUME_CACHE_TIMEOUT_MS 3000  // Cache 2 Sekunden gültig

// HTTP Event Handler (einfach, ohne Response-Capture)
esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_DATA:
            if (evt->data_len > 0) {
                ESP_LOGD(TAG, "HTTP Response: %.*s", evt->data_len, (char*)evt->data);
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}

// HTTP Event Handler mit Response-Capture
esp_err_t http_event_handler_with_response(esp_http_client_event_t *evt) {
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            http_response_len = 0;  // Reset buffer
            break;
        case HTTP_EVENT_ON_DATA:
            if (evt->data_len > 0) {
                // Response in Buffer speichern
                int copy_len = evt->data_len;
                if (http_response_len + copy_len >= HTTP_RESPONSE_BUFFER_SIZE) {
                    copy_len = HTTP_RESPONSE_BUFFER_SIZE - http_response_len - 1;
                }
                if (copy_len > 0) {
                    memcpy(http_response_buffer + http_response_len, evt->data, copy_len);
                    http_response_len += copy_len;
                    http_response_buffer[http_response_len] = '\0';
                }
                ESP_LOGD(TAG, "HTTP Response: %.*s", evt->data_len, (char*)evt->data);
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}

//Volume Hilfsfunktion: Finde Wert in JSON-ähnlicher Response // WiiM Format: "vol":"35" 

int parse_volume_from_response(const char* response) {
    if (response == NULL) return -1;
    
    ESP_LOGD(TAG, "Parsing response: %s", response);
    
    // Suche nach "vol":"
    const char* vol_start = strstr(response, "\"vol\":\"");
    if (vol_start == NULL) {
        ESP_LOGW(TAG, "Pattern \"vol\":\" nicht gefunden in Response");
        return -1;
    }
    
    // Überspringe "vol":"
    vol_start += 7;
    
    // Lese Zahl bis zum nächsten "
    int volume = atoi(vol_start);
    
    // Validierung: Volume sollte 0-100 sein
    if (volume < 0 || volume > 100) {
        ESP_LOGW(TAG, "Ungültiger Volume-Wert: %d", volume);
        return -1;
    }
    
    ESP_LOGD(TAG, "✅ Parsed volume: %d from response", volume);
    return volume;
}

// WiiM Volume Status abfragen
int wiim_get_current_volume(void) {
    if (!wifi_connected) {
        ESP_LOGW(TAG, "⚠️ WLAN nicht verbunden");
        return -1;
    }
    
    // Cache prüfen
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if (current_volume >= 0 && (now - last_volume_query_time) < VOLUME_CACHE_TIMEOUT_MS) {
        ESP_LOGD(TAG, "📦 Volume aus Cache: %d", current_volume);
        return current_volume;
    }
    
    char url[256];
    snprintf(url, sizeof(url), "https://%s/httpapi.asp?command=getPlayerStatus", WIIM_IP);
    
    ESP_LOGD(TAG, "📊 Frage Volume-Status ab...");
    
    http_response_len = 0;
    memset(http_response_buffer, 0, sizeof(http_response_buffer));
    
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler_with_response,
        .timeout_ms = 5000,
        .method = HTTP_METHOD_GET,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .skip_cert_common_name_check = true,
        .crt_bundle_attach = NULL,
        .buffer_size = 1024,
        .buffer_size_tx = 1024,
        .use_global_ca_store = false,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "❌ HTTP Client Init fehlgeschlagen!");
        return -1;
    }
    
    esp_err_t err = esp_http_client_perform(client);
    int status_code = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    
    if (err != ESP_OK || status_code != 200) {
        ESP_LOGE(TAG, "❌ Status-Abfrage fehlgeschlagen: %s (Status: %d)", 
                 esp_err_to_name(err), status_code);
        return -1;
    }
    
    // Parse Volume aus Response
    int volume = parse_volume_from_response(http_response_buffer);
    
    if (volume >= 0 && volume <= 100) {
        current_volume = volume;
        last_volume_query_time = now;
        ESP_LOGI(TAG, "📊 Aktuelles Volume: %d%%", volume);
        return volume;
    }
    
    ESP_LOGW(TAG, "⚠️ Konnte Volume nicht parsen aus: %s", http_response_buffer);
    return -1;
}

// Volume setzen (absoluter Wert)
bool wiim_set_volume(int volume) {
    if (!wifi_connected) {
        ESP_LOGW(TAG, "⚠️ WLAN nicht verbunden");
        return false;
    }
    
    // Begrenze auf 0-100
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    
    char command[64];
    snprintf(command, sizeof(command), "setPlayerCmd:vol:%d", volume);
    
    char url[256];
    snprintf(url, sizeof(url), "https://%s/httpapi.asp?command=%s", WIIM_IP, command);
    
    ESP_LOGI(TAG, "🔊 Setze Volume auf %d%%", volume);
    
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .timeout_ms = 5000,
        .method = HTTP_METHOD_GET,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .skip_cert_common_name_check = true,
        .crt_bundle_attach = NULL,
        .buffer_size = 1024,
        .buffer_size_tx = 1024,
        .use_global_ca_store = false,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "❌ HTTP Client Init fehlgeschlagen!");
        return false;
    }
    
    esp_err_t err = esp_http_client_perform(client);
    int status_code = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    
    if (err == ESP_OK && status_code == 200) {
        // Update Cache
        current_volume = volume;
        last_volume_query_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        ESP_LOGI(TAG, "✅ Volume erfolgreich geändert");
        return true;
    } else {
        ESP_LOGE(TAG, "❌ Volume-Änderung fehlgeschlagen: %s (Status: %d)", 
                 esp_err_to_name(err), status_code);
        return false;
    }
}

// Volume relativ ändern
bool wiim_change_volume_relative(int delta) {
    ESP_LOGI(TAG, "🎚️ Volume %s um %d", delta > 0 ? "erhöhen" : "verringern", abs(delta));
    
    // Aktuelles Volume abfragen
    int current = wiim_get_current_volume();
    
    if (current < 0) {
        ESP_LOGW(TAG, "⚠️ Konnte aktuelles Volume nicht ermitteln");
        // Fallback: Nimm 50% als Startwert an
        current = 50;
        ESP_LOGI(TAG, "📌 Verwende Fallback-Wert: %d%%", current);
    }
    
    // Neues Volume berechnen
    int new_volume = current + delta;
    
    // Begrenze auf 0-100
    if (new_volume < 0) new_volume = 0;
    if (new_volume > 100) new_volume = 100;
    
    ESP_LOGI(TAG, "📊 Volume: %d%% → %d%% (%+d)", current, new_volume, delta);
    
    // Setze neues Volume
    return wiim_set_volume(new_volume);
}

// ==================== TASTENBELEGUNG ====================

typedef struct {
    const char* key_name;
    const char* http_command;  // NULL = spezielle Funktion
    bool is_volume_command;
    int volume_delta;
} wiim_key_mapping_t;

// Tastenbelegung mit Volume-Steuerung
static const wiim_key_mapping_t wiim_mappings[] = {
    {"UP",           NULL, true,  +5},              // Volume +5
    {"DOWN",         NULL, true,  -5},              // Volume -5
    {"PLAY_PAUSE",   "setPlayerCmd:onepause", false, 0},
    {"SKIP_FORWARD", "setPlayerCmd:next", false, 0},
    {"SKIP_REWIND",  "setPlayerCmd:prev", false, 0},
    {NULL, NULL, false, 0}
};

// ==================== ABSTRAKTIONS-LAYER ====================

void send_wiim_command(const char* key_name) {
    // Finde das Mapping für diese Taste
    for (int i = 0; wiim_mappings[i].key_name != NULL; i++) {
        if (strcmp(wiim_mappings[i].key_name, key_name) == 0) {
            
            // Volume-Befehle
            if (wiim_mappings[i].is_volume_command) {
                ESP_LOGI(TAG, "🎵 WiiM: %s → Volume %+d", 
                         key_name, wiim_mappings[i].volume_delta);
                wiim_change_volume_relative(wiim_mappings[i].volume_delta);
                return;
            }
            
            // Normale HTTP-Befehle
            if (wiim_mappings[i].http_command != NULL) {
                ESP_LOGI(TAG, "🎵 WiiM: %s → %s", 
                         key_name, wiim_mappings[i].http_command);
                         
                // Verwende die einfache Version für normale Befehle
                char url[256];
                snprintf(url, sizeof(url), "https://%s/httpapi.asp?command=%s", 
                         WIIM_IP, wiim_mappings[i].http_command);
                
                esp_http_client_config_t config = {
                    .url = url,
                    .event_handler = http_event_handler,
                    .timeout_ms = 5000,
                    .method = HTTP_METHOD_GET,
                    .transport_type = HTTP_TRANSPORT_OVER_SSL,
                    .skip_cert_common_name_check = true,
                    .crt_bundle_attach = NULL,
                    .use_global_ca_store = false,
                };
                
                esp_http_client_handle_t client = esp_http_client_init(&config);
                if (client != NULL) {
                    esp_http_client_perform(client);
                    esp_http_client_cleanup(client);
                }
                return;
            }
            
            ESP_LOGD(TAG, "Taste '%s' nicht gemappt", key_name);
            return;
        }
    }
    
    ESP_LOGD(TAG, "Keine Aktion für Taste '%s'", key_name);
}

// ==================== FIRE TV TASTENCODE MAPPING ====================
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

// ==================== WLAN EVENT HANDLER ====================
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "WLAN getrennt - reconnect...");
        wifi_connected = false;
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "╔════════════════════════════════════════╗");
        ESP_LOGI(TAG, "║  ✅ WLAN VERBUNDEN!                    ║");
        ESP_LOGI(TAG, "║  IP: " IPSTR "                   ║", IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "╚════════════════════════════════════════╝");
        wifi_connected = true;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init(void)
{
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
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WLAN Init. Verbinde mit '%s'...", WIFI_SSID);
}

// ==================== HID DATEN VERARBEITUNG (VERBESSERT) ====================
void process_hid_data(uint8_t *data, uint16_t len) {
    ESP_LOG_BUFFER_HEX(TAG, data, len);
    
    if (len >= 5) {
        uint8_t key_byte1 = data[1];
        uint8_t key_byte2 = data[2];
        
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        // Prüfe ob Taste losgelassen wurde (beide Bytes 0x00)
        if (key_byte1 == 0x00 && key_byte2 == 0x00) {
            key_is_pressed = false;
            last_key_byte1 = 0;
            last_key_byte2 = 0;
            ESP_LOGD(TAG, "Taste losgelassen");
            return;  // Ignoriere Release-Events
        }
        
        // Debouncing: Gleiche Taste innerhalb der Debounce-Zeit?
        if (key_byte1 == last_key_byte1 && key_byte2 == last_key_byte2) {
            if ((now - last_key_time) < KEY_DEBOUNCE_MS) {
                ESP_LOGD(TAG, "Debounce: Taste ignoriert (Delta: %lu ms)", now - last_key_time);
                return;
            }
        }
        
        // Nur Press-Events verarbeiten, keine Repeats
        if (key_is_pressed) {
            ESP_LOGD(TAG, "Taste bereits gedrückt, ignoriere Repeat");
            return;
        }
        
        last_key_byte1 = key_byte1;
        last_key_byte2 = key_byte2;
        last_key_time = now;
        key_is_pressed = true;
        
        const char* key_name = NULL;
        
        if (key_byte1 != 0x00) {
            key_name = get_key_name_byte1(key_byte1);
        } else if (key_byte2 != 0x00) {
            key_name = get_key_name_byte2(key_byte2);
        }
        
        if (key_name != NULL) {
            ESP_LOGI(TAG, "");
            ESP_LOGI(TAG, "╔════════════════════════════════╗");
            ESP_LOGI(TAG, "║  🎮 %s", key_name);
            ESP_LOGI(TAG, "╚════════════════════════════════╝");
            ESP_LOGI(TAG, "");
            
            send_wiim_command(key_name);
        }
    }
}

// ==================== L2CAP CALLBACKS ====================
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

void l2cap_config_ind_callback(uint16_t lcid, tL2CAP_CFG_INFO *p_cfg) {
    ESP_LOGD(TAG, "L2CAP Config Ind: LCID=0x%04X", lcid);
    
    tL2CAP_CFG_INFO cfg;
    memset(&cfg, 0, sizeof(cfg));
    L2CA_ConfigRsp(lcid, &cfg);
}

void l2cap_config_cfm_callback(uint16_t lcid, tL2CAP_CFG_INFO *p_cfg) {
    ESP_LOGI(TAG, "L2CAP Config Complete: LCID=0x%04X", lcid);
    
    if (lcid == l2cap_handle_ctrl || lcid == l2cap_handle_intr) {
        if (l2cap_handle_ctrl != 0 && l2cap_handle_intr != 0) {
            bt_connected = true;
            ESP_LOGI(TAG, "");
            ESP_LOGI(TAG, "╔════════════════════════════════════════╗");
            ESP_LOGI(TAG, "║  🔥 FIRE TV REMOTE BEREIT!            ║");
            ESP_LOGI(TAG, "╚════════════════════════════════════════╝");
            ESP_LOGI(TAG, "");
        }
    }
}

void l2cap_disconnect_ind_callback(uint16_t lcid, bool ack_needed) {
    ESP_LOGW(TAG, "L2CAP Disconnect: LCID=0x%04X", lcid);
    
    if (lcid == l2cap_handle_ctrl) {
        l2cap_handle_ctrl = 0;
    } else if (lcid == l2cap_handle_intr) {
        l2cap_handle_intr = 0;
    }
    
    bt_connected = false;
}

void l2cap_disconnect_cfm_callback(uint16_t lcid, uint16_t result) {
    ESP_LOGW(TAG, "L2CAP Disconnect Confirm: LCID=0x%04X", lcid);
}

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

// ==================== GAP CALLBACK ====================
void gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
    switch (event) {
        case ESP_BT_GAP_AUTH_CMPL_EVT:
            if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
                ESP_LOGI(TAG, "✅ Pairing erfolgreich!");
                
                ESP_LOGI(TAG, "Öffne L2CAP Channels...");
                l2cap_handle_ctrl = L2CA_ConnectReq(HID_PSM_CONTROL, remote_bda);
                ESP_LOGI(TAG, "Control Channel LCID: 0x%04X", l2cap_handle_ctrl);
                
                vTaskDelay(pdMS_TO_TICKS(100));
                
                l2cap_handle_intr = L2CA_ConnectReq(HID_PSM_INTERRUPT, remote_bda);
                ESP_LOGI(TAG, "Interrupt Channel LCID: 0x%04X", l2cap_handle_intr);
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


    // ==================== MAIN ====================
void app_main(void) {
    esp_err_t ret;

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔═══════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  ESP32 Fire TV → WiiM Bridge v3.0     ║");
    ESP_LOGI(TAG, "║  Smart Volume Control                 ║");
    ESP_LOGI(TAG, "╚═══════════════════════════════════════╝");
    ESP_LOGI(TAG, "");

    // NVS Init
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // WLAN Init
    ESP_LOGI(TAG, "📡 Starte WLAN...");
    wifi_init();

    // BT Controller
    ESP_LOGI(TAG, "📶 Starte Bluetooth...");
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT));

    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());
    ESP_ERROR_CHECK(esp_bt_gap_register_callback(gap_callback));

    // Security
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_NONE;
    esp_bt_gap_set_security_param(ESP_BT_SP_IOCAP_MODE, &iocap, sizeof(uint8_t));
    uint8_t auth_req = 0x03;
    esp_bt_gap_set_security_param(0, &auth_req, sizeof(uint8_t));
    esp_bt_gap_set_pin(ESP_BT_PIN_TYPE_FIXED, 0, NULL);
    esp_bt_gap_set_device_name("ESP32_FireTV_WiiM");
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);

    // L2CAP
    ESP_LOGI(TAG, "🔧 Registriere L2CAP...");
    BTM_SetSecurityLevel(false, "", BTM_SEC_SERVICE_HIDD_INTR, 
                         BTM_SEC_NONE, HID_PSM_INTERRUPT, 0, 0);
    BTM_SetSecurityLevel(false, "", BTM_SEC_SERVICE_HCRP_CTRL, 
                         BTM_SEC_NONE, HID_PSM_CONTROL, 0, 0);
    
    register_l2cap_callbacks(HID_PSM_CONTROL);
    register_l2cap_callbacks(HID_PSM_INTERRUPT);

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "✅ Init abgeschlossen!");
    ESP_LOGI(TAG, "🎮 Tastenbelegung:");
    ESP_LOGI(TAG, "   UP          → Volume +5%%");
    ESP_LOGI(TAG, "   DOWN        → Volume -5%%");
    ESP_LOGI(TAG, "   PLAY/PAUSE  → Play/Pause Toggle");
    ESP_LOGI(TAG, "   SKIP_FWD    → Nächster Track");
    ESP_LOGI(TAG, "   SKIP_REW    → Vorheriger Track");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "💡 Volume wird dynamisch abgefragt!");
    ESP_LOGI(TAG, "");

    // Status Loop
    int counter = 0;
    while (1) {
        counter++;
        if (counter % 30 == 0) {
            ESP_LOGI(TAG, "📊 Status: WLAN=%s | BT=%s | WiiM=%s", 
                     wifi_connected ? "✅" : "❌",
                     bt_connected ? "✅" : "❌",
                     WIIM_IP);
            
            // Zeige aktuelles Volume alle 30 Sekunden an
            if (wifi_connected && current_volume >= 0) {
                ESP_LOGI(TAG, "   🔊 Volume: %d%%", current_volume);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}