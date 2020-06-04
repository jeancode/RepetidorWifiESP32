#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "lwip/opt.h"


#include "lwip/lwip_napt.h"


#include "lwip/err.h"
#include "lwip/sys.h"

#define MY_DNS_IP_ADDR 0x08080808 // 8.8.8.8

// WIFI CONFIGURATION
#define ESP_AP_SSID "Soaps"
#define ESP_AP_PASS "123456789"

#define EXAMPLE_ESP_WIFI_SSID      "upita"
#define EXAMPLE_ESP_WIFI_PASS      "123456789"

#define EXAMPLE_ESP_MAXIMUM_RETRY  3


static EventGroupHandle_t s_wifi_event_group;


const int WIFI_CONNECTED_BIT = BIT0;

static const char *TAG = "wifi apsta";

static int s_retry_num = 0;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
  switch(event->event_id) {
  case SYSTEM_EVENT_STA_START:
    esp_wifi_connect();
    break;
  case SYSTEM_EVENT_STA_GOT_IP:
    ESP_LOGI(TAG, "ip:" IPSTR, IP2STR(&event->event_info.got_ip.ip_info.ip));
    s_retry_num = 0;
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    break;
  case SYSTEM_EVENT_STA_DISCONNECTED:
    {
      if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
	  esp_wifi_connect();
	  xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
	  s_retry_num++;
	  ESP_LOGI(TAG,"Connectar AP");
      }
      ESP_LOGI(TAG,"Conexion fail");

      break;
    }
  case SYSTEM_EVENT_AP_STACONNECTED:
    ESP_LOGI(TAG,"Estacion Conectada");
    break;
  case SYSTEM_EVENT_AP_STADISCONNECTED:
    ESP_LOGI(TAG,"Ap Desconectada");
    break;  
  default:
    break;
  }
  return ESP_OK;
}

void wifi_init_sta()
{
    ip_addr_t dnsserver;


    s_wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));


    wifi_config_t wifi_config = {
	.sta = {
	    .ssid = EXAMPLE_ESP_WIFI_SSID,
	    .password = EXAMPLE_ESP_WIFI_PASS
	},
    };


    wifi_config_t ap_config = {
	.ap = {
	    .ssid = ESP_AP_SSID,
	    .channel = 0,
	    .authmode = WIFI_AUTH_WPA2_PSK,
	    .password = ESP_AP_PASS,
	    .ssid_hidden = 0,
	    .max_connection = 8,
	    .beacon_interval = 100
	}
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config) );


    dhcps_offer_t dhcps_dns_value = OFFER_DNS;
    dhcps_set_option_info(6, &dhcps_dns_value, sizeof(dhcps_dns_value));


    dnsserver.u_addr.ip4.addr = htonl(MY_DNS_IP_ADDR);
    dnsserver.type = IPADDR_TYPE_V4;
    dhcps_dns_setserver(&dnsserver);


    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "AP Final");
    ESP_LOGI(TAG, "Conectado a SSID: %s ", EXAMPLE_ESP_WIFI_SSID);
}

void app_main()
{
  // Iniciar NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // Configurar WIFI
  wifi_init_sta();


  u32_t napt_netif_ip = 0xC0A80401; //192.168.4.1
  ip_napt_enable(htonl(napt_netif_ip), 1);
  ESP_LOGI(TAG, "NAT is enabled");


}