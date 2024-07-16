
#include "esp_log.h"
#include "assert.h"
#include "esp_system.h"
#include "esp_event.h"
#include "storage.h"
#include "network.h"
#include "ethernet.h"
#include "IPTool.h"
#include "mqtt.h"
#include "tcpserver.h"
#include "uart.h"
#include "jsontool.h"
#include "esp32Time.h"
#include "home_i2c.h"
#include "ssd1306.h"
#include "home_8563.h"

ESP_EVENT_DEFINE_BASE(LED_EVENTS);
ESP_EVENT_DEFINE_BASE(ROOM_EVENTS);
ESP_EVENT_DEFINE_BASE(TCP_SERVER_EVENTS);
ESP_EVENT_DEFINE_BASE(SYSTEM_EVENTS);
ESP_EVENT_DEFINE_BASE(MQTT_LOCAL_EVENTS);

#define GLOBAL_FILE "/config/global.bin"
#define NETWORK_FILE "/config/network.bin"

#define LED0 (gpio_num_t)0
#define LED1 (gpio_num_t)12
#define LED2 (gpio_num_t)14
#define BTN0 (gpio_num_t)34

#define DEVICE_ID 253
#define CPU2_ID 254
#define RS485_MASTER DEVICE_ID

#define I2C (i2c_port_t) 0
#define SDA      GPIO_NUM_21
#define SCL      GPIO_NUM_22

const char *TAG="MAIN";

typedef enum {
  SEND_ALL = 0,
  SEND_MQTT = 1,
  SEND_TCP = 2,
} sender_t;

IPAddr Addr = IPAddr();
Storage disk = Storage();
Ethernet Eth = Ethernet();
Network wifi = Network();
Mqtt mqtt= Mqtt();
TcpServer tcpserver = TcpServer();
UART_config_t uart_cfg={};
UART uart = UART();
ESP32Time rtc(0);
Home_i2c bus = Home_i2c();
SSD1306_t ekran;
Home_8563 h8563 = Home_8563();

OdaNo *oda;
home_network_config_t NetworkConfig = {};
home_global_config_t GlobalConfig = {};
bool ComStatus=false;
sender_t sender_type=SEND_ALL;


#define FATAL_MSG(a, str)                          \
    if (!(a))                                                     \
    {                                                             \
        ESP_LOGE(TAG, "%s(%d): %s", __FUNCTION__, __LINE__, str); \
        abort();                                         \
    }


#include "tool/http.c"

void global_default_config(void)
{
     GlobalConfig.home_default = 1;
     strcpy((char*)GlobalConfig.device_name, "RoomBox");
     strcpy((char*)GlobalConfig.mqtt_server,"icemqtt.com.tr");
     GlobalConfig.mqtt_keepalive = 60;
     GlobalConfig.start_value = 0;
     GlobalConfig.odano._room.altodano = 0;
     GlobalConfig.odano._room.binano = 1;
     GlobalConfig.odano._room.katno = 1;
     GlobalConfig.odano._room.odano = 1;
     GlobalConfig.device_id = 1;
     GlobalConfig.comminication = 2;
     GlobalConfig.http_start = 1;
     GlobalConfig.tcpserver_start = 1;
     GlobalConfig.project_number = 1;
     disk.file_control(GLOBAL_FILE);
     disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);
}

void network_default_config(void)
{
     NetworkConfig.home_default = 1;
     NetworkConfig.wifi_type = HOME_WIFI_AP;
     NetworkConfig.wan_type = WAN_ETHERNET;
     NetworkConfig.ipstat = DYNAMIC_IP;
     //strcpy((char*)NetworkConfig.wifi_ssid, "Lords Palace");
     strcpy((char*)NetworkConfig.wifi_pass, "");
     strcpy((char*)NetworkConfig.ip,"192.168.1.11");
     strcpy((char*)NetworkConfig.netmask,"255.255.0.0");
     strcpy((char*)NetworkConfig.gateway,"192.168.0.1");
     strcpy((char*)NetworkConfig.dns,"4.4.4.4");
     strcpy((char*)NetworkConfig.backup_dns,"8.8.8.8");
     NetworkConfig.channel = 1;

/*
    NetworkConfig.wifi_type = HOME_WIFI_STA;
    strcpy((char *)NetworkConfig.wifi_ssid,(char *)"Akdogan_2.4G");
    strcpy((char *)NetworkConfig.wifi_pass,(char *)"651434_2.4");

    //NetworkConfig.wifi_type = HOME_WIFI_STA;
    strcpy((char *)NetworkConfig.wifi_ssid,(char *)"IMS_YAZILIM");
    strcpy((char *)NetworkConfig.wifi_pass,(char *)"mer6514a4c");
    */

    disk.write_file(NETWORK_FILE,&NetworkConfig,sizeof(NetworkConfig),0);
}


void default_start(void)
{
	disk.read_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig), 0);
	if (GlobalConfig.home_default==0 ) {
		//Global ayarlar diskte kayıtlı değil. Kaydet.
		 global_default_config();
		 disk.read_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);
		 FATAL_MSG(GlobalConfig.home_default,"Global Initilalize File ERROR !...");
	}

	disk.read_file(NETWORK_FILE,&NetworkConfig,sizeof(NetworkConfig), 0);
	if (NetworkConfig.home_default==0 ) {
		//Network ayarları diskte kayıtlı değil. Kaydet.
		 network_default_config();
		 disk.file_control(NETWORK_FILE);
		 disk.read_file(NETWORK_FILE,&NetworkConfig,sizeof(NetworkConfig),0);
		 FATAL_MSG(NetworkConfig.home_default, "Network Initilalize File ERROR !...");
	}
}

void display_write(int satir, const char *txt)
{
    ssd1306_clear_line(&ekran,satir,false);
    ssd1306_display_text(&ekran, satir, (char*)txt, strlen(txt), false);
}

void time_sync(struct timeval *tv)
{
    ESP_LOGI(TAG,"Time Sync");
    time_t now;
	struct tm tt1;
	time(&now);
	localtime_r(&now, &tt1);
	setenv("TZ", "UTC-03:00", 1);
	tzset();
	localtime_r(&now, &tt1);
    h8563.esp_to_device();
}

#include "tool/events.c"

void webwrite(home_network_config_t net, home_global_config_t glob)
{
    disk.write_file(NETWORK_FILE,&net,sizeof(net),0);
    disk.write_file(GLOBAL_FILE,&glob,sizeof(glob),0);
}

void defreset(void *)
{
    network_default_config();
    global_default_config();
}

#include "tool/uartcallback.cpp"

void CPU2_Ping_Reset(uint8_t counter)
{
    ESP_LOGE(TAG,"CPU2 Cevap vermiyor %d",counter);
	/*
  if (GlobalConfig.reset_servisi==1)
  {  
    ESP_LOGW(TAG,"CPU2 Cevap vermiyor %d",counter);
    if (counter>9) ESP_LOGE(TAG,"CPU2 RESETLENECEK");
    if (counter>10 && GlobalConfig.reset_servisi==1) {
        ESP_LOGE(TAG,"CPU2 RESETLIYORUM");
        CPU2_Reset();
    }
  }
  */
}

#include "lwip/dns.h"
//#include "lwip/ip4_addr.h

void query_dns(const char *name)
{
    struct addrinfo *address_info;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int res = getaddrinfo(name, NULL, &hints, &address_info);
    if (res != 0 || address_info == NULL) {
        ESP_LOGE(TAG, "couldn't get hostname for :%s: "
                      "getaddrinfo() returns %d, addrinfo=%p", name, res, address_info);
    } else {
        if (address_info->ai_family == AF_INET) {
            struct sockaddr_in *p = (struct sockaddr_in *)address_info->ai_addr;
            ESP_LOGI(TAG, "Resolved %s IPv4 address: %s", name,ipaddr_ntoa((const ip_addr_t*)&p->sin_addr.s_addr));
        }
    }    
}

extern "C" void app_main()
{

    esp_log_level_set("esp-tls", ESP_LOG_NONE);  
    esp_log_level_set("transport_base", ESP_LOG_NONE); 
    esp_log_level_set("mqtt_client", ESP_LOG_NONE); 
    esp_log_level_set("esp_netif_handlers", ESP_LOG_NONE);
 //   esp_log_level_set("MQTT", ESP_LOG_NONE); 

    gpio_config_t io_conf = {};
	io_conf.mode = GPIO_MODE_INPUT_OUTPUT;
	io_conf.pin_bit_mask = (1ULL<<LED0) | (1ULL<<LED1) | (1ULL<<LED2);
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	gpio_config(&io_conf);

    gpio_config_t io_conf1 = {};
	io_conf1.mode = GPIO_MODE_INPUT;
	io_conf1.pin_bit_mask = (1ULL<<BTN0);
	io_conf1.pull_down_en = GPIO_PULLDOWN_DISABLE;
	io_conf1.pull_up_en = GPIO_PULLUP_ENABLE;
	gpio_config(&io_conf1);

    if(esp_event_loop_create_default()!=ESP_OK) {ESP_LOGE(TAG,"esp_event_loop_create_default ERROR "); }
    ESP_ERROR_CHECK(esp_event_handler_instance_register(LED_EVENTS, ESP_EVENT_ANY_ID, led_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, ip_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(ETH_EVENT, ESP_EVENT_ANY_ID, eth_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(TCP_SERVER_EVENTS, ESP_EVENT_ANY_ID, tcp_handler, NULL, NULL)); 
	ESP_ERROR_CHECK(esp_event_handler_instance_register(SYSTEM_EVENTS, ESP_EVENT_ANY_ID, system_handler, NULL, NULL)); 
	ESP_ERROR_CHECK(esp_event_handler_instance_register(MQTT_LOCAL_EVENTS, ESP_EVENT_ANY_ID, mqtt_handler, NULL, NULL)); 
    ESP_ERROR_CHECK(disk.init());

    default_start();
    oda = new OdaNo(GlobalConfig.odano);
	strcpy((char*)GlobalConfig.license,oda->get_oda_string());

    ESP_LOGI(TAG,"I2C bus oluşturuluyor");
    ESP_ERROR_CHECK(bus.init_bus(SDA,SCL,I2C));
    ESP_LOGI(TAG, "Panel is 128x32");
    i2c_master_init(&ekran,&bus,-1);
	ssd1306_init(&ekran, 128, 32);
    ssd1306_clear_screen(&ekran, false);
	ssd1306_contrast(&ekran, 0xff);
    display_write(0,"Initializing..");

    ESP_ERROR_CHECK_WITHOUT_ABORT(h8563.init_device(&bus,0x00));
    ESP_LOGI(TAG,"Saat Entegresi bulundu");
    struct tm rtcinfo;
    bool Valid=false;

    sntp_set_time_sync_notification_cb(time_sync);

    if (h8563.get_time(&rtcinfo, &Valid)==ESP_OK) {
        ESP_LOGI(TAG,"Saat okundu. Degerler Stabil %s",(!Valid)?"Degil":"");
        if (!Valid)
        {
            ESP_LOGW(TAG, "Saat default degere ayarlaniyor");
            struct tm tt0 = {};
            tt0.tm_sec = 0;
            tt0.tm_min = 59;
            tt0.tm_hour = 13;
            tt0.tm_mday = 1;
            tt0.tm_mon = 3;
            tt0.tm_year = 2024;
            h8563.set_time(&tt0,true);
            h8563.get_time(&rtcinfo, &Valid); 
            //pcf8563_set_sync(true);
            if (Valid) ESP_LOGE(TAG,"Saat AYARLANAMADI");
        } else {
            //rtc degerlerini espye yazıyor
            h8563.device_to_esp();
            char *rr = (char *)malloc(50);
            struct tm tt0 = {};
            h8563.Get_current_date_time(rr,&tt0);
            ESP_LOGW(TAG, "Tarih/Saat %s", rr);
            free(rr);
        }
    }
	
	uart_cfg.uart_num = 2;
    uart_cfg.dev_num  = DEVICE_ID;
    uart_cfg.rx_pin   = 27;
    uart_cfg.tx_pin   = 26;
    uart_cfg.atx_pin  = 32;
    uart_cfg.arx_pin  = 25;
    uart_cfg.int_pin  = 33;
    uart_cfg.baud   = 921600;

    uart.initialize(&uart_cfg, &uart_callback);
    uart.pong_start(CPU2_ID,&CPU2_Ping_Reset,LED0);

    display_write(2,"Ethernet/Wifi");

    if (GlobalConfig.comminication==1)
	{
		//Lokal Haberleşme WIFI üzerinden, Wan haberleşmesi ethernet üzerinden olacak
		//espnow'ı aktif hale getir. Etherneti aktif hale getir.
		if (Eth.start(NetworkConfig, &GlobalConfig)!=ESP_OK) FATAL_MSG(0,"Ethernet Başlatılamadı..");
		ComStatus=true;       
	} else {
			if (NetworkConfig.wan_type==WAN_ETHERNET)
			{
				//Wan haberleşmesi ethernettir. Wifi kapalı
				if (Eth.start(NetworkConfig, &GlobalConfig)!=ESP_OK) FATAL_MSG(0,"Ethernet Baslatilamadi..");
				ComStatus=true;
			}
            
			if (NetworkConfig.wan_type==WAN_WIFI)
			{
				//Wan haberleşmesi Wifi. Etherneti kapat. Wifi STA olarak start et.
				wifi.init(NetworkConfig);
				ComStatus=true;
				if (wifi.Station_Start()!=ESP_OK) {ESP_LOGE(TAG,"WIFI Station Baslatilamadi..");ComStatus=false;}
			}           
	}

    if (ComStatus)
	{
        display_write(2,"Services");
        //obtain_time();
		if (GlobalConfig.http_start==1)
		{
			ESP_LOGI(TAG, "WEB START");
			ESP_ERROR_CHECK(start_rest_server("/config",NetworkConfig, GlobalConfig, &webwrite));
		}

		if (GlobalConfig.tcpserver_start==1)
		{
			ESP_LOGI(TAG, "TCP SOCKET SERVER START");
			tcpserver.start(5717);
		}

		ESP_LOGI(TAG, "MQTT START");
		mqtt.init(GlobalConfig);
		mqtt.start();
	}

    ssd1306_clear_screen(&ekran,false);
    char *ss = (char*)calloc(1,100);
    sprintf(ss,"%s",Addr.to_string(NetworkConfig.home_ip));
    display_write(0,ss);

    sprintf(ss,"%s/%d",oda->get_oda_string(),GlobalConfig.odano.room);
    display_write(3,ss);
    free(ss); 
    
    char *pp = (char *)calloc(1,64);
    rtc.getTimeDate(pp);
    ESP_LOGI(TAG,"Zaman : %s",pp);
    free(pp);
    

    	ESP_LOGI(TAG,"         ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
	    ESP_LOGI(TAG,"         |         HOTEL ROOM BOX      |");
	    ESP_LOGI(TAG,"         ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");       
	    ESP_LOGI(TAG,"         RS485 DEVICE ID : %d", GlobalConfig.device_id);
	    ESP_LOGI(TAG,"               Cihaz ADI : %s", GlobalConfig.device_name);
	    ESP_LOGI(TAG,"             Mqtt Server : %s", GlobalConfig.mqtt_server);
	    ESP_LOGI(TAG,"          Mqtt Keepalive : %d", GlobalConfig.mqtt_keepalive);
	    ESP_LOGI(TAG,"                 Bina No : %d", oda->get_bina());
	    ESP_LOGI(TAG,"                  Kat No : %d", oda->get_kat());
	    ESP_LOGI(TAG,"                  Oda No : %d", oda->get_oda());
	    ESP_LOGI(TAG,"       Connection Oda No : %d", oda->get_connection());
	    ESP_LOGI(TAG,"                  Oda Id : %d", oda->get_oda_id());
	    ESP_LOGI(TAG,"                  ODA NO : %s", oda->get_oda_string());
        ESP_LOGI(TAG,"                Proje NO : %d", GlobalConfig.project_number);
	    ESP_LOGI(TAG,"          Lokal Iletisim : %s", (GlobalConfig.comminication==1)?"Wifi":"Rs485");
	    ESP_LOGI(TAG,"                     Wan : %s", (NetworkConfig.wan_type==WAN_ETHERNET)?"Ethernet":"Wifi");
	    ESP_LOGI(TAG,"        Wan Comminication: %s", (ComStatus)?"OK":"Wan iletisimi YOK");
	    ESP_LOGI(TAG,"             Web Started : %s", (GlobalConfig.http_start==1)?"OK":"Closed Web");
	    ESP_LOGI(TAG," Tcp Socket Serv Started : %s", (GlobalConfig.tcpserver_start==1)?"OK":"Closed Tcp Server");
	    ESP_LOGI(TAG,"                      IP : %s", Addr.to_string(NetworkConfig.home_ip));
	    ESP_LOGI(TAG,"                 NETMASK : %s", Addr.to_string(NetworkConfig.home_netmask));
	    ESP_LOGI(TAG,"                 GATEWAY : %s", Addr.to_string(NetworkConfig.home_gateway));
	    ESP_LOGI(TAG,"               BROADCAST : %s", Addr.to_string(NetworkConfig.home_broadcast));
	    ESP_LOGI(TAG,"                     MAC : %s", Addr.mac_to_string(NetworkConfig.mac));
		ESP_LOGI(TAG,"                     DNS : %s", NetworkConfig.dns);
		ESP_LOGI(TAG,"              Backup DNS : %s", NetworkConfig.backup_dns);
	    ESP_LOGI(TAG,"               Free heap : %u", esp_get_free_heap_size());


    query_dns("icemqtt.com.tr");


    char *rr = (char *)malloc(50);
    struct tm tt0 = {};
    h8563.Get_current_date_time(rr,&tt0);
    ESP_LOGW(TAG, "Tarih/Saat %s", rr);
    free(rr);
	
    ESP_LOGI(TAG,"STARTED..");
    while(true)
    {
        if (gpio_get_level(BTN0)==0)
        {
            printf("BUTON\n");
            uint64_t startMicros = esp_timer_get_time();
            uint64_t currentMicros = startMicros;
            while(gpio_get_level(BTN0)==0)
            {
                currentMicros = esp_timer_get_time()-startMicros;
                
            }
        }
        vTaskDelay(10/portTICK_PERIOD_MS);
    }

}