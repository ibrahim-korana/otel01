

void command_transfer(cJSON *rcv_json, sender_t snd);

void led_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    //ESP_LOGW(TAG, "LED %ld %ld", id , base, id);
    led_events_data_t* event = (led_events_data_t*) event_data;
    bool et=false, mq=false;
    
    if (id==LED_EVENTS_ETH) {gpio_set_level(LED2, event->state);}
    if (id==LED_EVENTS_MQTT) {gpio_set_level(LED1, event->state);}
    vTaskDelay(10/portTICK_PERIOD_MS);
    mq = gpio_get_level(LED1);
    et = gpio_get_level(LED2);

    if (!mq && !et) display_write(2,"          ");
    if (et) display_write(2,"ETH ERROR");
    if (!et && mq) display_write(2,"MQTT ERROR"); 
/*
    if (id==LED_EVENTS_ETH && event->state) {
        display_write(2,"ETH ERROR");
        return;
    }
    if (id==LED_EVENTS_MQTT && event->state) {        
        display_write(2,"MQTT ERROR");
        return;
    }
*/    
    
}

void ip_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    //ESP_LOGW(TAG, "IP %ld %ld", id , base, id);
    if (id==IP_EVENT_STA_GOT_IP || id==IP_EVENT_ETH_GOT_IP)
              {
                // Net.wifi_update_clients();
                ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
                NetworkConfig.home_ip = event->ip_info.ip.addr;
                NetworkConfig.home_netmask = event->ip_info.netmask.addr;
                NetworkConfig.home_gateway = event->ip_info.gw.addr;
                NetworkConfig.home_broadcast = (uint32_t)(NetworkConfig.home_ip) | (uint32_t)0xFF000000UL;
                //tcpclient.wait = false;
                if (id==IP_EVENT_ETH_GOT_IP) Eth.set_connect_bit();
                if (id==IP_EVENT_STA_GOT_IP) wifi.set_connection_bit();
                ESP_LOGI(TAG, "IP Received");
                sntp_set_time_sync_notification_cb(time_sync);
                Set_SystemTime_SNTP();             
              }
}

void eth_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    //ESP_LOGW(TAG, "ETH %ld %ld", id , base, id);
    if (id==ETHERNET_EVENT_CONNECTED)
        	{
        		esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;
        		esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, NetworkConfig.mac);
        		ESP_LOGI(TAG, "Ethernet Link Up");
        	}
    if(id==ETHERNET_EVENT_DISCONNECTED)
    {
        ESP_LOGI(TAG, "Ethernet Link Down");
        Eth.set_fail_bit();
    }
}

void wifi_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    //ESP_LOGW(TAG, "WIFI %ld %ld", id , base, id);
    if (id==WIFI_EVENT_STA_DISCONNECTED)
              {
                 // tcpclient.wait = true;
                  if (wifi.retry < WIFI_MAXIMUM_RETRY) {
                	  wifi.Station_connect();;
                      wifi.retry++;
                      ESP_LOGW(TAG, "Tekrar Baglanıyor %d",WIFI_MAXIMUM_RETRY-wifi.retry);
                                                      } else {
                      FATAL_MSG(0,"Wifi Başlatılamadı..");
                                                             }
              }

    if (id==WIFI_EVENT_STA_START && GlobalConfig.comminication!=TR_ESPNOW)
        {
        wifi.retry=0;
        ESP_LOGW(TAG, "Wifi Network Connecting..");
        wifi.Station_connect();
        }

}


void tcp_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    tcp_events_data_t *ev = (tcp_events_data_t *)event_data;
    
    char *data = (char *)calloc(1,ev->data_len+1);
    strcpy(data,ev->data);
    //data convert edilip system event olarak yayınlanacak 
    cJSON *rcv_json = cJSON_Parse(data);
    char *command = (char *)calloc(1,15);
    JSON_getstring(rcv_json,"com", command,15);
    if (strcmp(command,"ping")!=0) ESP_LOGI(TAG,"TCP << %s",data);
    bool trn = true;

    if (strcmp(command,"ping")==0) {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "com", "pong");
        char *dat = cJSON_PrintUnformatted(root);
        tcpserver.Send(dat);
        cJSON_free(dat);
        cJSON_Delete(root);
        trn = false;
     }

    if (strcmp(command,"T_REQ")==0) {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "com", "T_ACK");
        unsigned long abc = rtc.getLocalEpoch();
        cJSON_AddNumberToObject(root, "time", abc);
        char *dat = cJSON_PrintUnformatted(root);
       
        while (tcpserver.sender_busy) vTaskDelay(50/portTICK_PERIOD_MS);
        tcpserver.Send(dat,true);
        vTaskDelay(100/portTICK_PERIOD_MS); 
        cJSON_free(dat);
        cJSON_Delete(root);
        trn = false;
    } 
/*
    if ( strcmp(command,"intro")==0 || 
         strcmp(command,"sintro")==0 || 
         strcmp(command,"loc")==0 ||
         strcmp(command,"loc")==0 ||
         strcmp(command,"sevent")==0 ||
         strcmp(command,"spevent")==0 ||
         strcmp(command,"status")==0 ||
         if (strncmp(command,"COM",3)==0) 
       ) {
            sender_type=SEND_TCP;
            uart.Sender(data,CPU2_ID);
         }
    if ( strcmp(command,"event")==0 ) {
            sender_type=SEND_ALL;
            uart.Sender(data,CPU2_ID);
    }     
*/
    if (trn) command_transfer(rcv_json, SEND_TCP); 

    free(data); 
    free(command);
    cJSON_Delete(rcv_json); 
}

void mqtt_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    mqtt_events_data_t *ev = (mqtt_events_data_t *)event_data;

    //printf("%d %d %s %s\n",ev->data_len,ev->topic_len,ev->data,ev->topic);

    char *data = (char *)calloc(1,ev->data_len+1);
    char *topic = (char *)calloc(1,ev->topic_len+1);
    strncpy(data,ev->data,ev->data_len);
    strncpy(topic,ev->topic,ev->topic_len);
    //data convert edilip system event olarak yayınlanacak 

    ESP_LOGI(TAG,"MQTT << %s",data);
    cJSON *rcv_json = cJSON_Parse(data);
    char *command = (char *)calloc(1,15);

    JSON_getstring(rcv_json,"com", command,15); 
    if (strcmp(command,"message")==0) {
        while (tcpserver.sender_busy) vTaskDelay(50/portTICK_PERIOD_MS);
        tcpserver.Send(data);
    }
    
    command_transfer(rcv_json, SEND_MQTT);
    free(command);       
    free(topic);
    free(data); 
    cJSON_Delete(rcv_json); 
   

}

void system_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    if (id==SYSTEM_DEFAULT_RESET) {
        network_default_config();
        global_default_config();
        esp_restart();
    }
    if (id==SYSTEM_RESET) {
        //Self reset
        //esp_restart();
    }
}



//-----------------------------------------
bool send_action(cJSON *rcv)
{
    char *dat = cJSON_PrintUnformatted(rcv);
            while(uart.is_busy()) vTaskDelay(50/portTICK_PERIOD_MS);
            uart.Sender(dat,RS485_MASTER);
            vTaskDelay(50/portTICK_PERIOD_MS); 
    cJSON_free(dat);
    return true;
}
//-----------------------------------------
void command_transfer(cJSON *rcv_json, sender_t snd)
{
    char *command = (char *)calloc(1,15);

    JSON_getstring(rcv_json,"com", command,15); 

    ESP_LOGI("command_transfer","COMMAND : %s",command);

    if (strcmp(command,"time_sync")==0) 
    {
        sntp_set_time_sync_notification_cb(time_sync);
        Set_SystemTime_SNTP_diff(); 
    }  

    if (strncmp(command,"COM",3)==0) send_action(rcv_json);

    if (strcmp(command,"start_senaryo")==0) {
        uint8_t *aa = (uint8_t *) malloc(1);
        *aa = 1;
       // start_sen(aa);
        free(aa);
    }

    if (strcmp(command,"stop_senaryo")==0) {
        uint8_t *aa = (uint8_t *) malloc(1);
        *aa = 1;
       // stop_sen(aa);
        free(aa);
    }

    if (strcmp(command,"message")==0) {
        sender_type = snd;
        send_action(rcv_json);

    }

    if (strcmp(command,"intro")==0) {
        sender_type = snd;
        send_action(rcv_json);
    }
    if (strcmp(command,"sintro")==0) {
        //Cihaz Alt özelliklerini iste
        sender_type = snd;
        send_action(rcv_json);
    }
    if (strcmp(command,"status")==0) {
        //tüm cihazların veya 1 cihazın durum/larını iste
        sender_type = snd;
        send_action(rcv_json);
    }
    if (strcmp(command,"event")==0) {
        //mqttden gelen mesajı cpu2 ye aktararak fonksiyonların
        //durum degiştirmesini sağlar
        sender_type = SEND_ALL;
        send_action(rcv_json);
    }
    if (strcmp(command,"sevent")==0) {
        //durum degiştir
        sender_type = SEND_ALL;
        send_action(rcv_json);
    }
    if (strcmp(command,"spevent")==0) {
        //durum degiştir
        sender_type = SEND_ALL;
        send_action(rcv_json);
    }
    if (strcmp(command,"loc")==0) {
        //durum degiştir
        sender_type = SEND_ALL;
        send_action(rcv_json);
    }

    if (strcmp(command,"all")==0) {
        sender_type = snd;
        send_action(rcv_json);
    }

    if (strcmp(command,"sen")==0) {
        uint8_t snn = 0;
        bool change = false;
        if (JSON_getint(rcv_json,"no", &snn)) {
            GlobalConfig.aktif_senaryo=snn;
            change=true;
        }
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "com", "sen");
        cJSON_AddNumberToObject(root, "no", GlobalConfig.aktif_senaryo);
        char *dat = cJSON_PrintUnformatted(root);
        while (tcpserver.sender_busy) vTaskDelay(50/portTICK_PERIOD_MS);
        tcpserver.Send(dat);
        mqtt.publish(dat,strlen(dat));
        cJSON_free(dat);
        cJSON_Delete(root);
        if (change)
          {
            disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);
            ESP_LOGI(TAG,"Senaryo No : %d",GlobalConfig.aktif_senaryo);
          }
    }
    free(command);   
}
