

#include "room.h"
#include "esp_log.h"

static const char *ROOM_TAG = "ROOM";

ESP_EVENT_DEFINE_BASE(ROOM_EVENTS);

void Room::room_active()
{
	Base_Port *target = port_head_handle;
	home_status_t stat = get_status();
	while (target)
	{
		if (target->type == PORT_OUTPORT)
		{
			stat.stat = target->set_status(true); //roleyi cektir
			local_set_status(stat,true);  //statusu true yap
			if (function_callback!=NULL) function_callback(this, get_status());
			ESP_ERROR_CHECK(esp_event_post(ROOM_EVENTS, ROOM_ON, NULL, 0, portMAX_DELAY));
			GenelAktif=true;
		}
		target = target->next;
	}
}

void Room::func_callback(void *arg, port_action_t action)
{

printf("GELDI \n");
   if (action.action_type==ACTION_INPUT)
     {
        button_handle_t *btn = (button_handle_t *) action.port;
        Base_Port *prt = (Base_Port *) iot_button_get_user_data(btn);
        button_event_t evt = iot_button_get_event(btn);
        Room *oda = (Room *) arg;
        //home_status_t stat = oda->get_status();
        //bool change = false;

        	printf("GELDI %s %d %d\n",prt->name, prt->type, evt);

        	if (evt==BUTTON_PRESS_DOWN || evt==BUTTON_PRESS_UP)
			{
               /*
        		if (prt->type==PORT_SENSOR && strcmp(prt->name,"DOOR")==0)
				{
					if (evt==BUTTON_PRESS_UP) {
			            //Kapı acildi
						ESP_ERROR_CHECK(esp_event_post(ROOM_EVENTS, MAIN_DOOR_ON, NULL, 0, portMAX_DELAY));
					}
					if (evt==BUTTON_PRESS_DOWN) {
						//Kapi kapandi
						ESP_ERROR_CHECK(esp_event_post(ROOM_EVENTS, MAIN_DOOR_OFF, NULL, 0, portMAX_DELAY));
						oda->ytim_start();
					}
				}
               */
        		if (prt->type==PORT_INPORT)
				{
                    printf("GELDI PORT_INPORT\n");
        			//EQU
					if (evt==BUTTON_PRESS_UP) {
						//Kart Çıkartıldı
						//oda->ytim_start();
						printf("EQU SENSOR UP \n");
					}
					if (evt==BUTTON_PRESS_DOWN) {
						//Kart Takıldı
						//if (oda->ytim_is_active()) oda->ytim_stop();
						//oda->room_active();
						printf("EQU SENSOR DOWN \n");
					}
				}

        		/*
				//portları tarayıp çıkış portlarını bul
				Base_Port *target = oda->port_head_handle;
				while (target) {
					if (target->type == PORT_OUTPORT)
					{
						if (prt->type==PORT_INPORT)
						{
							if (evt==BUTTON_PRESS_DOWN) {

								if (oda->GenelAktif)
									{
									  //Oda aktif hareket algılandı. İzleme timerını resetle
									  //Eger giriş varsa timerı durdur
									  oda->xtim_restart();
									  oda->ytim_stop();
									} else {
										if (oda->ytim_active())
										{
											//Kapı açılıp kapandı ve hareket algılandı
											//Oda aktif degil. Bu nedenle yeni giriş var.
											oda->ytim_stop();
											stat.stat = target->set_status(true);
											change = true;
											oda->xtim_restart();
											printf("Odada insan var\n");
										}
									}
                                //printf("DOWN %d\n",stat.stat);

							}
						}



					}

					target = target->next;
					}
			}
        	if (change) {

        		            if (stat.stat==1 && !oda->GenelAktif)
        					{
        						((Room *)oda)->local_set_status(stat,true);
        						if (oda->function_callback!=NULL) oda->function_callback(oda, oda->get_status());
        						ESP_ERROR_CHECK(esp_event_post(ROOM_EVENTS, ROOM_ON, NULL, 0, portMAX_DELAY));
        						oda->GenelAktif=true;
        					}

        				}
    */
		} //evt
     } //ACTION_INPUT
}


//bu fonksiyon odayı yazılım ile tetiklemek için kullanılır.
void Room::set_status(home_status_t stat)
{
    if (!genel.virtual_device)
    {
        local_set_status(stat);
        if (!genel.active) status.stat = false;

        Base_Port *target = port_head_handle;
        while (target) {
            if (target->type == PORT_OUTPORT)
                {
                    status.stat = target->set_status(status.stat);
                }
            target = target->next;
        }
        write_status();
        if (function_callback!=NULL) function_callback((void *)this, get_status());
    } else {
        if (command_callback!=NULL) command_callback((void *)this, stat);
    }
}

//Eger mevcut durumdan fark var ise statusu ayarlar ve/veya callback çağrılır
//durum degişimi portları etkilemez. bu fonksiyon daha çok remote cihaz
//durum değişimleri için kullanılır.
void Room::remote_set_status(home_status_t stat, bool callback_call)
{
	//
	ESP_LOGI(ROOM_TAG,"%d Status Changed",genel.device_id);
}

void Room::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
}

void Room::get_status_json(cJSON* obj)
{
    return ConvertStatus(status , obj);
}

void Room::xtim_stop(void){
    if (xtimer!=NULL)
      if (esp_timer_is_active(xtimer)) {
    	  esp_timer_stop(xtimer);
      }

}
void Room::xtim_start(void){
    if (xtimer!=NULL)
     if (!esp_timer_is_active(xtimer))
     {
       ESP_ERROR_CHECK(esp_timer_start_once(xtimer, duration * 1000000));
     }
}

void Room::ytim_stop(void){
    if (ytimer!=NULL)
      if (esp_timer_is_active(ytimer)) {
    	  esp_timer_stop(ytimer);
    	  printf("Kapatma zamanlamasi DURDU\n");
      }

}
void Room::ytim_start(void){
    if (ytimer!=NULL)
     if (!esp_timer_is_active(ytimer))
     {
        ESP_ERROR_CHECK(esp_timer_start_once(ytimer, 20 * 1000000));
    	printf("Kapatma zamanlamasi basladi\n");
     }
}

void Room::xtim_restart(void){
    xtim_stop();
    xtim_start();
}

void Room::xtim_callback(void* arg)
{
    //lamba/ları kapat
    Room *oda = (Room *) arg;
    home_status_t stat = oda->get_status();
    stat.stat=false;
    oda->set_status(stat);
    printf("ODA STOP %d\n",stat.stat);
    ESP_ERROR_CHECK(esp_event_post(ROOM_EVENTS, ROOM_OFF, NULL, 0, portMAX_DELAY));
    oda->GenelAktif=false;
}




void Room::room_event(room_event_t ev)
{
    //
}

void Room::fire(bool stat)
{
    if (stat) {
        main_temp_status = status;
        status.stat = false;
        set_status(status);
    } else {
       status = main_temp_status;
       set_status(status);
    }
}

void Room::senaryo(char *par)
{
    if(strcmp(par,"on")==0)
      {
        status.stat = true;
        set_status(status);
      }
    if(strcmp(par,"off")==0)
      {
        status.stat = false;
        set_status(status);
      }
}

void Room::init(void)
{
    if (!genel.virtual_device)
    {
        //Kayıtlı son durumu oku
        disk.read_status(&status,genel.device_id);
        //xTimer ı oluştur
        esp_timer_create_args_t arg1 = {};
        arg1.callback = &xtim_callback;
        arg1.name = "Ltim1";
        arg1.arg = (void *) this;
        ESP_ERROR_CHECK(esp_timer_create(&arg1, &xtimer));
        //yTimer ı oluştur 
        esp_timer_create_args_t arg2 = {};
        arg2.callback = &xtim_callback;
        arg2.name = "Ltim2";
        arg2.arg = (void *) this;
        ESP_ERROR_CHECK(esp_timer_create(&arg2, &ytimer));
        //Hardware den ilk outport un durumunu oku
        //  
        Base_Port *target = port_head_handle;
        if (duration==0) duration=5;
        /*
        while (target) {
            if (target->type==PORT_OUTPORT)
                {
                status.stat = !target->get_hardware_status();
                break;
                }
            target=target->next;
        }
        if (status.stat) {
        	GenelAktif=true;
        	//xtim_start();
        	printf("ODA BASLATILMIS GORUNUYOR\n");
        }
        */
    }
}


