idf_component_register(
    SRCS "scan.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES 
        bt 
        nvs_flash
        esp_wifi
        esp_netif
        esp_event
        esp_http_client
        esp-tls
)

# Füge Bluedroid Stack Include-Pfade hinzu (für L2CAP direktzugriff)
target_include_directories(${COMPONENT_LIB} PRIVATE
    "${IDF_PATH}/components/bt/host/bluedroid/stack/include"
    "${IDF_PATH}/components/bt/host/bluedroid/common/include"
    "${IDF_PATH}/components/bt/common/osi/include"
    "${IDF_PATH}/components/bt/common/include"
    "${IDF_PATH}/components/bt/common/osi/include"
)

