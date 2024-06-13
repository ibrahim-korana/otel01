



char **paketler;
uint8_t paket_sayisi = 0;

void _send_data(char *data)
{
if (sender_type==SEND_ALL)
    {
        ESP_LOGI("UART_CALLBACK","Tcp ve mqtt ye gidiyor");
        mqtt.publish(data,strlen(data));
        while (tcpserver.sender_busy) vTaskDelay(50/portTICK_PERIOD_MS);
        tcpserver.Send(data,true);
        vTaskDelay(100/portTICK_PERIOD_MS); 
    }
if (sender_type==SEND_MQTT)
    {
        ESP_LOGI("UART_CALLBACK","mqtt ye gidiyor");
        cJSON *ss = cJSON_CreateObject();
        cJSON_AddNumberToObject(ss,"room",GlobalConfig.odano.room);
        
        mqtt.publish(data,strlen(data));
    } 
if (sender_type==SEND_TCP)
    {
        ESP_LOGI("UART_CALLBACK","Tcp ye gidiyor");
        while (tcpserver.sender_busy) vTaskDelay(50/portTICK_PERIOD_MS);
        tcpserver.Send(data,true);
        vTaskDelay(100/portTICK_PERIOD_MS); 
    }       
}

void intro_olustur()
{
    cJSON *introJSON=NULL;
    cJSON *func=NULL;

    introJSON = cJSON_CreateObject();
    func = cJSON_AddArrayToObject(introJSON, "func");

    for (uint8_t p=0;p<paket_sayisi;p++)
    {
        cJSON *rcv_json = cJSON_Parse(paketler[p]);
        if (rcv_json==NULL) return; 
        cJSON *oku = NULL;
        cJSON *element = NULL;   
        oku = cJSON_GetObjectItem(rcv_json, "func");
        cJSON_ArrayForEach(element,oku)
            {
                uint8_t id=0,loc=0,ico=0;
                char *nm = (char *)malloc(20);
                cJSON *obje = cJSON_CreateObject();
                JSON_getstring(element,"name",nm,15);
                JSON_getint(element,"id",&id);
                JSON_getint(element,"loc",&loc);
                JSON_getint(element,"icon",&ico);
                cJSON_AddStringToObject(obje, "name", nm);
                cJSON_AddNumberToObject(obje,"id",id);
                cJSON_AddNumberToObject(obje,"loc",loc);
                cJSON_AddNumberToObject(obje,"icon",ico);
                cJSON_AddItemToArray(func, obje); 
            }
        if (p==paket_sayisi-1) {
            cJSON_AddStringToObject(introJSON, "com", "intro");
            char *ad = (char *)malloc(20);
            JSON_getstring(rcv_json,"dev",ad,15);
            cJSON_AddStringToObject(introJSON, "dev", ad);
            char *dat = cJSON_PrintUnformatted(introJSON);
            _send_data(dat);
            cJSON_free(dat); 
            cJSON_Delete(introJSON); 
        } 
        cJSON_Delete(rcv_json);
        free(paketler[p]);
        paketler[p] = NULL;
   }
   free(paketler);
   paketler=NULL;
}



void uart_callback(char *data)
{ 
	bool n_mqtt = true;
	uint8_t sender = 254;

	ESP_LOGI(TAG,"CPU2 << %s",data);
    vTaskDelay(50/portTICK_PERIOD_MS);
	cJSON *rcv_json = cJSON_Parse(data);
    if (rcv_json==NULL) return; 
    char *command = (char *)calloc(1,15);
    assert(command);
    JSON_getstring(rcv_json,"com", command,10); 

    if (strcmp(command,"intro")==0)
    {
        uint8_t dpak=0, cpak=0;
        JSON_getint(rcv_json,"tpak", &dpak);
        JSON_getint(rcv_json,"cpak", &cpak);
        paket_sayisi = dpak;
        //printf("dpak %d cpak%d paketler %p\n",dpak,cpak,paketler);
        if (cpak==1) paketler = (char**)malloc(paket_sayisi * sizeof(char*));
        paketler[cpak-1] = (char*)malloc(strlen(data)+2);
        strcpy(paketler[cpak-1],data);
        if (cpak==dpak)
          {
            intro_olustur();
          }
        n_mqtt = false;  
    }

	if (strcmp(command,"T_REQ")==0)
    {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "com", "T_ACK");
        //unsigned long abc = rtc.getLocalEpoch();
        unsigned long abc = rtc.getEpoch();
        cJSON_AddNumberToObject(root, "time", abc);
        cJSON_AddNumberToObject(root, "res_sys", GlobalConfig.reset_servisi);
		cJSON_AddNumberToObject(root, "room", GlobalConfig.odano.room);
        char *dat = cJSON_PrintUnformatted(root);
        while(uart.is_busy()) vTaskDelay(50/portTICK_PERIOD_MS);
        uart.Sender(dat,sender);    
        cJSON_free(dat);
        cJSON_Delete(root);
        n_mqtt = false;
    }

    cJSON_Delete(rcv_json);
    
    if (n_mqtt) {
        bool mm=true;

        if (sender_type!=SEND_ALL && strcmp(command,"status")==0) mm=false;
        if (sender_type==SEND_ALL)
          {
             _send_data(data);
          }
        if (sender_type==SEND_MQTT)
          {
             _send_data(data);
             if (mm) sender_type = SEND_ALL;
          } 
        if (sender_type==SEND_TCP)
          {
             _send_data(data);
             if (mm) sender_type = SEND_ALL;
          }       
    }

    free(command);

}