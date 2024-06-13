/*
 * room.h
 *
 *  Created on: 10 Nis 2023
 *      Author: korana
 */

#ifndef _ROOM_H_
#define _ROOM_H_

#include <stdio.h>
#include <string.h>
#include "core.h"
#include "storage.h"
#include "classes.h"
#include "esp_timer.h"

//Lamba Version 1.1

class Room : public Base_Function {
    public:
      Room(int id, function_callback_t cb, Storage dsk) {
        genel.device_id = id;
        strcpy(genel.name,"room");
        function_callback = cb;
        disk = dsk;
        disk.read_status(&status,genel.device_id);
        local_port_callback = &func_callback;
        if (!status.first) {
              memset(&status,0,sizeof(status));
              status.stat     = false;
              status.active   = true;
              status.first    = true;
              status.status   = 0;
              GenelAktif = false;
              write_status();
        }
      };
      ~Room() {};
        void set_status(home_status_t stat);
      	void remote_set_status(home_status_t stat, bool callback_call=true);

		void get_status_json(cJSON* obj) override;
		void room_event(room_event_t ev) override;
		//bool get_port_json(cJSON* obj) override;
		void init(void);

		void fire(bool stat);
		void ConvertStatus(home_status_t stt, cJSON* obj);
		void senaryo(char *par) override;

		bool GenelAktif = false;



    private:
		esp_timer_handle_t xtimer = NULL;
		esp_timer_handle_t ytimer = NULL;

	protected:
	  static void func_callback(void *arg, port_action_t action);
	  static void xtim_callback(void* arg);
	  void xtim_stop(void);
	  void xtim_start(void);
	  void xtim_restart(void);
	  void ytim_stop(void);
	  void ytim_start(void);
	  bool ytim_is_active(void) {return esp_timer_is_active(ytimer);}
	  void room_active();



    };


#endif
