
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "lwip/api.h"

#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "string.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "websocket_server.h"
#include "cJSON.h"

#define EXAMPLE_ESP_WIFI_SSID      "Elon"    //add my wifi credentials
#define EXAMPLE_ESP_WIFI_PASS      "12345678"
#define EXAMPLE_ESP_MAXIMUM_RETRY  3
//-----------------------------------
#define DIRB 14
#define PWMB 27
#define DIRA 12
#define PWMA 13
#define left_motor_PWM 0  // PWM channels used to control motors
#define right_motor_PWM 1
#define LED_OUT 25 // head lights output
//------------------------------------
/* FreeRTOS event group to signal when we are connected*/
static QueueHandle_t client_queue;
const static int client_queue_size = 10;
static EventGroupHandle_t s_wifi_event_group;
/* The event group allows multiple bits for each event, but we only care about one event
 * - are we connected to the AP with an IP? */
struct Car_state {
	int is_updated;
	int left_motor_speed;
	int right_motor_speed;
	int cam_horiz; //220
	int cam_vertical; //150
	int lights;
};
struct Car_state tank = {0, 0, 0, 220, 150, 0};

const int WIFI_CONNECTED_BIT = BIT0;

static const char *TAG = "wifi station";

static int s_retry_num = 0;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "got ip:%s",
                 ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        {
            if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
                esp_wifi_connect();
                xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
                s_retry_num++;
                ESP_LOGI(TAG,"retry to connect to the AP");
            }
            ESP_LOGI(TAG,"connect to the AP fail\n");
            break;
        }
    default:
        break;
    }
    return ESP_OK;
}

void wifi_init_sta()
{
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

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
}

static void PWM_init(void)
{
    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t left_motor; // create an object with PWM channel parameters
    left_motor.speed_mode = LEDC_HIGH_SPEED_MODE;
    left_motor.channel    = left_motor_PWM;
    left_motor.timer_sel  = LEDC_TIMER_1;
    left_motor.intr_type  = LEDC_INTR_DISABLE;
    left_motor.gpio_num   = PWMA;
    left_motor.duty       = 0; // set duty at 0
    //ledc_channel_config(&left_motor);// pass it to config function

    ledc_channel_config_t right_motor;
    right_motor.speed_mode = LEDC_HIGH_SPEED_MODE;
    right_motor.channel    = right_motor_PWM;
    right_motor.timer_sel  = LEDC_TIMER_1;
    right_motor.intr_type  = LEDC_INTR_DISABLE;
    right_motor.gpio_num   = PWMB;
    right_motor.duty       = 0; // set duty at 0

    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t PWM_timer; // create an object with PWM timer parameters
    PWM_timer.speed_mode       = LEDC_HIGH_SPEED_MODE;
    PWM_timer.timer_num        = LEDC_TIMER_1;
    PWM_timer.duty_resolution  = LEDC_TIMER_8_BIT;
    PWM_timer.freq_hz          = 100;  // set output frequency at 100 Hz
    //ledc_timer_config(&PWM_timer); // pass it to config function

    ESP_ERROR_CHECK( ledc_channel_config(&right_motor) );
    ESP_ERROR_CHECK( ledc_channel_config(&left_motor) );
    ESP_ERROR_CHECK( ledc_timer_config(&PWM_timer) );
}
void gpio_init()
{
	gpio_pad_select_gpio(DIRA);
	gpio_set_direction(DIRA, GPIO_MODE_OUTPUT);
	gpio_pad_select_gpio(DIRB);
	gpio_set_direction(DIRB, GPIO_MODE_OUTPUT);
}
void setMotorState(int leftMotorSpeed, int rightMotorSpeed)
{
	if (leftMotorSpeed >= 0){
		ledc_set_duty(LEDC_HIGH_SPEED_MODE, left_motor_PWM, leftMotorSpeed);
  	    ledc_update_duty(LEDC_HIGH_SPEED_MODE, left_motor_PWM);
  	    gpio_set_level(DIRA, 1);
  	    printf("Left = forward %d\n", leftMotorSpeed);
	} else if (leftMotorSpeed < 0){
		leftMotorSpeed = -1*leftMotorSpeed;
		ledc_set_duty(LEDC_HIGH_SPEED_MODE, left_motor_PWM, leftMotorSpeed);
		ledc_update_duty(LEDC_HIGH_SPEED_MODE, left_motor_PWM);
		gpio_set_level(DIRA, 0);
		printf("Left = back\n");
	}
	if (rightMotorSpeed >= 0){
		ledc_set_duty(LEDC_HIGH_SPEED_MODE, right_motor_PWM, rightMotorSpeed);
  	    ledc_update_duty(LEDC_HIGH_SPEED_MODE, right_motor_PWM);
  	    gpio_set_level(DIRB, 1);
  	    printf("Right = forward %d\n", rightMotorSpeed);
	} else if (rightMotorSpeed < 0){
		rightMotorSpeed = -1*rightMotorSpeed;
		ledc_set_duty(LEDC_HIGH_SPEED_MODE, right_motor_PWM, rightMotorSpeed);
		ledc_update_duty(LEDC_HIGH_SPEED_MODE, right_motor_PWM);
		gpio_set_level(DIRB, 0);
		printf("Right = back\n");
		}
}
// handles websocket events
void websocket_callback(uint8_t num,WEBSOCKET_TYPE_t type,char* msg,uint64_t len){
  const static char* TAG = "websocket_callback";
  int value;
  switch(type) {
    case WEBSOCKET_CONNECT:
      ESP_LOGI(TAG,"client %i connected!",num);
      break;
    case WEBSOCKET_DISCONNECT_EXTERNAL:
    {
    	ESP_LOGI(TAG,"client %i sent a disconnect message",num);
    	tank.left_motor_speed = 0;
    	tank.right_motor_speed = 0;
    	tank.is_updated = false;
    }
      break;
    case WEBSOCKET_DISCONNECT_INTERNAL:
      ESP_LOGI(TAG,"client %i was disconnected",num);
      break;
    case WEBSOCKET_DISCONNECT_ERROR:
    {
    	ESP_LOGI(TAG,"client %i was disconnected due to an error",num);
    	tank.left_motor_speed = 0;
    	tank.right_motor_speed = 0;
    	tank.is_updated = false;
    }
      break;
    case WEBSOCKET_TEXT:
    {
      if(len) {
    	  ESP_LOGI(TAG,"client sent a message:");
    	  cJSON *root = cJSON_Parse(msg);
    	  int dataType = cJSON_GetObjectItem(root,"dataType")->valueint;
    	  switch(dataType) {
    	       case 0:
    	       {
    	    	   printf("Motor data received!\n");
    	    	   tank.left_motor_speed = cJSON_GetObjectItem(root,"leftSpeed")->valueint;
    	    	   tank.right_motor_speed = cJSON_GetObjectItem(root,"rightSpeed")->valueint;
    	    	   printf(" %d\n",tank.left_motor_speed);
    	    	   printf(" %d\n",tank.right_motor_speed);
    	       }
    	       break;
    	       case 1:
    	       {
    	    	   tank.cam_horiz = cJSON_GetObjectItem(root,"horizontal")->valueint;
    	    	   tank.cam_vertical = cJSON_GetObjectItem(root,"vertical")->valueint;
    	    	   printf("Camera data received!\n");
    	       }
    	       break;
    	       case 2:
    	       {
    	    	   tank.lights = cJSON_GetObjectItem(root,"whiteLights")->valueint;
    	    	   printf("Lights data received!\n");
    	       }
    	   }
    	  tank.is_updated = false;
    	  cJSON_Delete(root);
       }
    }
      break;
    case WEBSOCKET_BIN:
      ESP_LOGI(TAG,"client %i sent binary message of size %i:\n%s",num,(uint32_t)len,msg);
      break;
    case WEBSOCKET_PING:
      ESP_LOGI(TAG,"client %i pinged us with message of size %i:\n%s",num,(uint32_t)len,msg);
      break;
    case WEBSOCKET_PONG:
      ESP_LOGI(TAG,"client %i responded to the ping",num);
      break;
  }
}

// serves any clients (provides real services)
static void http_serve(struct netconn *conn) {
  const static char* TAG = "http_server";
  const static char HTML_HEADER[] = "HTTP/1.1 200 OK\nContent-type: text/html\n\n";
  const static char ERROR_HEADER[] = "HTTP/1.1 404 Not Found\nContent-type: text/html\n\n";
  const static char JS_HEADER[] = "HTTP/1.1 200 OK\nContent-type: text/javascript\n\n";
  const static char CSS_HEADER[] = "HTTP/1.1 200 OK\nContent-type: text/css\n\n";
  //const static char PNG_HEADER[] = "HTTP/1.1 200 OK\nContent-type: image/png\n\n";
  const static char ICO_HEADER[] = "HTTP/1.1 200 OK\nContent-type: image/x-icon\n\n";
  //const static char PDF_HEADER[] = "HTTP/1.1 200 OK\nContent-type: application/pdf\n\n";
  //const static char EVENT_HEADER[] = "HTTP/1.1 200 OK\nContent-Type: text/event-stream\nCache-Control: no-cache\nretry: 3000\n\n";
  struct netbuf* inbuf;
  static char* buf;
  static uint16_t buflen;
  static err_t err;

  // web pages served by server_____________-

  //---------------
  netconn_set_recvtimeout(conn,1000); // allow a connection timeout of 1 second
  ESP_LOGI(TAG,"reading from client...");
  err = netconn_recv(conn, &inbuf); //receiving data from "conn" connection and put it to inbuf
  ESP_LOGI(TAG,"read from client");
  if(err==ERR_OK) {
    netbuf_data(inbuf, (void**)&buf, &buflen);
    if(buf) {

      // default page (if client requests just basic http page)
      if     (strstr(buf,"GET / ")
          && !strstr(buf,"Upgrade: websocket")) { // if request type is get and doesn't want to upgrade to websocket
        ESP_LOGI(TAG,"Sending /"); // just send client basic web page
        netconn_write(conn, HTML_HEADER, sizeof(HTML_HEADER)-1,NETCONN_NOCOPY);
        netconn_close(conn);
        netconn_delete(conn);
        netbuf_delete(inbuf);
      }

      // default page websocket (if client wants to open websocket connection)
      else if(strstr(buf,"GET / ")
           && strstr(buf,"Upgrade: websocket")) {
        ESP_LOGI(TAG,"Requesting websocket on /");
        ws_server_add_client(conn,buf,buflen,"/",websocket_callback);// establish web socket connection
        netbuf_delete(inbuf);
      }

      else {
        ESP_LOGI(TAG,"Unknown request");
        netconn_close(conn);
        netconn_delete(conn);
        netbuf_delete(inbuf);
      }
    }
    else {
      ESP_LOGI(TAG,"Unknown request (empty?...)");
      netconn_close(conn);
      netconn_delete(conn);
      netbuf_delete(inbuf);
    }
  }
  else { // if err==ERR_OK
    ESP_LOGI(TAG,"error on read, closing connection");
    netconn_close(conn);
    netconn_delete(conn);
    netbuf_delete(inbuf);
  }
}

// handles clients when they first connect. passes to a queue
// only handles new incoming connections, puts them to the queue, then another server deals with
static void server_task(void* pvParameters) {
  const static char* TAG = "server_task";
  struct netconn *conn, *newconn; // initialize to pointers to structures conn and newconn
  static err_t err;
  client_queue = xQueueCreate(client_queue_size,sizeof(struct netconn*));

  conn = netconn_new(NETCONN_TCP);
  netconn_bind(conn,NULL,80);
  netconn_listen(conn);
  ESP_LOGI(TAG,"server listening");
  do {
    err = netconn_accept(conn, &newconn);
    ESP_LOGI(TAG,"new client");
    if(err == ERR_OK) {
      xQueueSendToBack(client_queue,&newconn,portMAX_DELAY);
      //http_serve(newconn);
    }
  } while(err == ERR_OK);
  netconn_close(conn);
  netconn_delete(conn);
  ESP_LOGE(TAG,"task ending, rebooting board");
  esp_restart();
}

// receives clients from queue, handles them
static void server_handle_task(void* pvParameters) {
  const static char* TAG = "server_handle_task";
  struct netconn* conn;
  ESP_LOGI(TAG,"task starting");
  for(;;) {
    xQueueReceive(client_queue,&conn,portMAX_DELAY);
    if(!conn) continue;
    http_serve(conn);
  }
  vTaskDelete(NULL);
}

static void car_state_update_task(void* pvParameters) {
	while(1) {
		if (!tank.is_updated) {
			setMotorState(tank.left_motor_speed, tank.right_motor_speed);
			printf("Car state was updated!\n");
			tank.is_updated = 1;
		}
		vTaskDelay(10);
	}
}
static void count_task(void* pvParameters) {
  const static char* TAG = "count_task";
  char out[20];
  int len;
  int clients;
  const static char* word = "%i";
  uint8_t n = 0;
  const int DELAY = 1000 / portTICK_PERIOD_MS; // 1 second

  ESP_LOGI(TAG,"starting task");
  for(;;) {
    len = sprintf(out,word,n);
    clients = ws_server_send_text_all(out,len);
    if(clients > 0) {
      //ESP_LOGI(TAG,"sent: \"%s\" to %i clients",out,clients);
    }
    n++;
    vTaskDelay(DELAY);
  }
}

void app_main() {
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	    }
	ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
	wifi_init_sta();
	PWM_init();
	gpio_init();
	setMotorState(255,255);
	ws_server_start();
	xTaskCreate(&server_task,"server_task",3000,NULL,9,NULL);
	xTaskCreate(&server_handle_task,"server_handle_task",4000,NULL,6,NULL);
	xTaskCreate(&count_task,"count_task",6000,NULL,2,NULL);
	xTaskCreate(&car_state_update_task,"car_state_update_task",3000,NULL,2,NULL);
}
