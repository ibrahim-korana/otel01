/*
 * room.h
 *
 *  Created on: 10 Nis 2023
 *      Author: korana
 */

#ifndef _KLIMA_H_
#define _KLIMA_H_

#include <stdio.h>
#include <string.h>
#include "core.h"
#include "storage.h"
#include "classes.h"

class Klima : public Base_Function {
    public:
      Klima(int id, function_callback_t cb, Storage dsk) {
        genel.device_id = id;
        strcpy(genel.name,"klima");
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
              write_status();
        }
      };
      ~Klima() {};
        void set_status(home_status_t stat);
      	void remote_set_status(home_status_t stat, bool callback_call=true);

		void get_status_json(cJSON* obj) override;
		void room_event(room_event_t ev) override;
		//bool get_port_json(cJSON* obj) override;
		void init(void);

		void fire(bool stat);
		void ConvertStatus(home_status_t stt, cJSON* obj);
		void senaryo(char *par);

    private:


	protected:
	  static void func_callback(void *arg, port_action_t action);

    };


#endif
