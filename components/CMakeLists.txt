
set(LIBRARY_SRCS

  library/pcf/i2cdev.c
  library/pcf/ice_pcf8574.c
  library/pcf/pcf8563.c
  library/core/core.cpp
  library/storage/storage.cpp
  library/network/network.cpp
  library/rs485/rs485.cpp
  library/wifi_now/wifi_now.cpp  
  library/enc28j60/esp_eth_mac_enc28j60.c
  library/enc28j60/esp_eth_phy_enc28j60.c
  library/enc28j60/ethernet.cpp
  library/cihazlar/cihazlar.cpp
  library/classes/classes.cpp
  library/jsontool/jsontool.cpp
  library/out/iot_out.c
  library/out/out_8574.c
  library/out/out_gpio.c
  library/out/out_pwm.c
  library/button/button_8574.c
  library/button/button_adc.c
  library/button/button_gpio.c
  library/button/iot_button.c
  library/tcpserver/tcpserver.cpp
  library/mqtt/mqtt.cpp
  library/uart/uart.cpp

  library/lamp/lamp.cpp
  library/room/room.cpp
  library/klima/klima.cpp
  library/pwmled/pled.cpp
  library/rgbled/rgbled.cpp

  )
set(includedirs
  library/pcf
  library/core
  library/storage
  library/network
  library/enc28j60
  library/iptool
  library/classes
  library/out
  library/button
  library/jsontool
  library/cihazlar
  library/rs485
  library/wifi_now 
  library/tcpserver
  library/mqtt
  library/uart

  library/lamp
  library/room
  library/klima
  library/pwmled
  library/rgbled
  )

set(srcs ${LIBRARY_SRCS})
set(requires mqtt esp_adc esp_timer nvs_flash esp_wifi esp_eth esp_event vfs driver spiffs json)

idf_component_register(INCLUDE_DIRS ${includedirs} SRCS ${srcs} REQUIRES ${requires})
