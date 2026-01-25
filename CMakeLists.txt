idf_component_register(
    SRCS "firetv_remote2mqtt.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES 
        bt 
        nvs_flash
        mqtt
)

# Füge Bluedroid Stack Include-Pfade hinzu (für L2CAP direktzugriff)
target_include_directories(${COMPONENT_LIB} PRIVATE
    "${IDF_PATH}/components/bt/host/bluedroid/stack/include"
    "${IDF_PATH}/components/bt/host/bluedroid/common/include"
    "${IDF_PATH}/components/bt/common/osi/include"
    "${IDF_PATH}/components/bt/common/include"
    "${IDF_PATH}/components/bt/common/osi/include"
)