

#include "esp_log.h"
#include "klima.h"

static const char *KLIMA_TAG = "KLIMA";

void Klima::func_callback(void *arg, port_action_t action)
{

   if (action.action_type==ACTION_INPUT)
     {
        button_handle_t *btn = (button_handle_t *) action.port;
        Base_Port *prt = (Base_Port *) iot_button_get_user_data(btn);
        button_event_t evt = iot_button_get_event(btn);
        Base_Function *oda = (Base_Function *) arg;
        home_status_t stat = oda->get_status();
        bool change = false;
        if (oda->genel.active)
		{
        	if (evt==BUTTON_PRESS_DOWN || evt==BUTTON_PRESS_UP)
			{
				//portları tarayıp çıkış portlarını bul
				Base_Port *target = oda->port_head_handle;
				while (target) {
					if (target->type == PORT_OUTPORT)
					{
						if (prt->type==PORT_INPORT) {
							if (evt==BUTTON_PRESS_DOWN && oda->room_on) {stat.stat = target->set_status(true);change = true;}
							if (evt==BUTTON_PRESS_UP  && oda->room_on) {stat.stat = target->set_status(false);change = true;}
						                            }
					}
					target = target->next;
								}
			}
        	if (change) {
        					((Klima *)oda)->local_set_status(stat,true);
        					if (oda->function_callback!=NULL) oda->function_callback(oda, oda->get_status());
        				}

		}
     }
}


//bu fonksiyon lambayı yazılım ile tetiklemek için kullanılır.
void Klima::set_status(home_status_t stat)
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
void Klima::remote_set_status(home_status_t stat, bool callback_call)
{
	//
	ESP_LOGI(KLIMA_TAG,"%d Status Changed",genel.device_id);
}

void Klima::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
}

void Klima::get_status_json(cJSON* obj)
{
    return ConvertStatus(status , obj);
}

void Klima::room_event(room_event_t ev)
{
	switch (ev)
		{
		case ROOM_ON: {
			room_on = true;
			break;
		              }
		case ROOM_OFF:{
			room_on = false;
	        break;
	                  }
		case MAIN_DOOR_ON:{
			if (room_on) ;
			printf("door on\n");
			break;
	                  }
		case MAIN_DOOR_OFF:{
			if (room_on) ; 
			printf("door off\n");
			break;
			               }

		}
	//printf("KLIMA Room state %d\n",room_on);
}

void Klima::fire(bool stat)
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

void Klima::senaryo(char *par)
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

void Klima::init(void)
{
    if (!genel.virtual_device)
    {
        disk.read_status(&status,genel.device_id);
        Base_Port *target = port_head_handle;
        while (target) {
            if (target->type==PORT_OUTPORT)
                {
                status.stat = target->get_hardware_status();
                break;
                }
            target=target->next;
        }
    }
}


