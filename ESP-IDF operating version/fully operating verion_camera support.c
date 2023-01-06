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
#include "driver/adc.h"
#include "driver/i2c.h"
#include "string.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "websocket_server.h"
#include "cJSON.h"
#include "pca9685.h"
//#include "sdkconfig.h"

#define EXAMPLE_ESP_WIFI_SSID      "Sevit"    //add my wifi credentials
#define EXAMPLE_ESP_WIFI_PASS      "21061993"
#define EXAMPLE_ESP_MAXIMUM_RETRY   50
//-----------------------------------
#define DIRB 14
#define PWMB 27
#define DIRA 12
#define PWMA 13
#define left_motor_PWM 0  // PWM channels used to control motors
#define right_motor_PWM 1
#define lights_pin 4 // Main lights output
#define cam_light_contr 2 // Control LED on camera
//------------------------------------
#define DEFAULT_VREF    1100        // Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES   64          // Multisampling
#define CONV_CONST      13.9        // Used to convert ADC reading to real battery voltage
//------------------------------------
#define I2C_EXAMPLE_MASTER_SCL_IO   22    //!< gpio number for I2C master clock
#define I2C_EXAMPLE_MASTER_SDA_IO   21    // gpio number for I2C master data
#define I2C_EXAMPLE_MASTER_FREQ_HZ  100000     /*!< I2C master clock frequency */
#define I2C_EXAMPLE_MASTER_NUM      I2C_NUM_0   /*!< I2C port number for master dev */
#define I2C_EXAMPLE_MASTER_TX_BUF_DISABLE   0   /*!< I2C master do not need buffer */
#define I2C_EXAMPLE_MASTER_RX_BUF_DISABLE   0   /*!< I2C master do not need buffer */
#define I2C_ADDRESS     0x40    /*!< slave address for PCA9685 */
#define ACK_CHECK_EN    0x1     /*!< I2C master will check ack from slave */
#define ACK_CHECK_DIS   0x0     /*!< I2C master will not check ack from slave */
#define ACK_VAL         0x0     /*!< I2C ack value */
#define NACK_VAL        0x1     /*!< I2C nack value */
//------------------------------------
// FreeRTOS event group to signal when we are connected
static QueueHandle_t client_queue;
const static int client_queue_size = 10;
static EventGroupHandle_t s_wifi_event_group;
/* The event group allows multiple bits for each event, but we only care about one event
 * - are we connected to the AP with an IP? */
float voltage ;
struct Car_state {
	int is_updated; // flag used to update car state after new data received
	int left_motor_speed;
	int right_motor_speed;
	int cam_horiz; //220 horizontal camera servo position
	int cam_vertical; //150 vertical camera servo position
	int lights; // lights state
	int bat_voltg; //raw ADC value
};
struct Car_state tank = {0, 0, 0, 220, 150, 0, 0}; // create a struct with default values
struct Hand_state {
	int is_updated;
	int base;
	int elbow;
	int wrist;
	int claw;
};
struct Hand_state hand = {1, 0, 0, 0, 0};
struct camera_position {
	int is_updated;
	int horz;
	int vert;
	int light;
};
struct camera_position camera = {1, 125, 125, 0};
const int WIFI_CONNECTED_BIT = BIT0;
static const char *TAG = "Wi-Fi station";
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
static void i2c_example_master_init(void)
{
    //ESP_LOGD(tag, ">> PCA9685");
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_EXAMPLE_MASTER_SDA_IO;
    conf.scl_io_num = I2C_EXAMPLE_MASTER_SCL_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_EXAMPLE_MASTER_FREQ_HZ;

    int i2c_master_port = I2C_EXAMPLE_MASTER_NUM;
    ESP_ERROR_CHECK(i2c_param_config(i2c_master_port, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(i2c_master_port, conf.mode,
                       I2C_EXAMPLE_MASTER_RX_BUF_DISABLE,
                       I2C_EXAMPLE_MASTER_TX_BUF_DISABLE, 0));
}
static void PWM_init(void) // setup PWM channels for motors control
{
    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t left_motor; // create an object with PWM channel parameters
    left_motor.speed_mode = LEDC_HIGH_SPEED_MODE;
    left_motor.channel    = left_motor_PWM;
    left_motor.timer_sel  = LEDC_TIMER_1;
    left_motor.intr_type  = LEDC_INTR_DISABLE;
    left_motor.gpio_num   = PWMA;
    left_motor.duty       = 0; // set duty at 0

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

    ESP_ERROR_CHECK( ledc_channel_config(&right_motor) );
    ESP_ERROR_CHECK( ledc_channel_config(&left_motor) );
    ESP_ERROR_CHECK( ledc_timer_config(&PWM_timer) );
}
void gpio_init() // set up GPIOs mode
{
	adc1_config_width(ADC_WIDTH_BIT_12); // ADC accuracy set up
	adc1_config_channel_atten(ADC_CHANNEL_6, ADC_ATTEN_DB_11); // choose ADC channel and bringing input voltage no normal range
	gpio_pad_select_gpio(DIRA);
	gpio_set_direction(DIRA, GPIO_MODE_OUTPUT);
	gpio_pad_select_gpio(DIRB);
	gpio_set_direction(DIRB, GPIO_MODE_OUTPUT);
	gpio_pad_select_gpio(lights_pin);
	gpio_set_direction(lights_pin, GPIO_MODE_OUTPUT);
	gpio_pad_select_gpio(cam_light_contr);
	gpio_set_direction(cam_light_contr, GPIO_MODE_OUTPUT);
}
static void get_bat_voltg(void* pvParameters) // measuring battery voltage
{
// one cell voltage is measured via 1/2 divider
	while(1) {
		uint32_t adc_reading = 0;
        //Multisampling for better accuracy
        for (int i = 0; i < NO_OF_SAMPLES; i++) {
            adc_reading += adc1_get_raw((adc1_channel_t)ADC_CHANNEL_6);
        }
        adc_reading /= NO_OF_SAMPLES;
        tank.bat_voltg = adc_reading;
        voltage = CONV_CONST*adc_reading/4096; //Convert adc_reading to voltage in V
        vTaskDelay(10);
	}
}
void set_motor_state(int leftMotorSpeed, int rightMotorSpeed) // set up motors speed and direction
{
	if (leftMotorSpeed >= 0){
		ledc_set_duty(LEDC_HIGH_SPEED_MODE, left_motor_PWM, leftMotorSpeed);
  	    ledc_update_duty(LEDC_HIGH_SPEED_MODE, left_motor_PWM);
  	    gpio_set_level(DIRA, 1);
	} else if (leftMotorSpeed < 0){
		leftMotorSpeed = -1*leftMotorSpeed;
		ledc_set_duty(LEDC_HIGH_SPEED_MODE, left_motor_PWM, leftMotorSpeed);
		ledc_update_duty(LEDC_HIGH_SPEED_MODE, left_motor_PWM);
		gpio_set_level(DIRA, 0);
	}
	if (rightMotorSpeed >= 0){
		ledc_set_duty(LEDC_HIGH_SPEED_MODE, right_motor_PWM, rightMotorSpeed);
  	    ledc_update_duty(LEDC_HIGH_SPEED_MODE, right_motor_PWM);
  	    gpio_set_level(DIRB, 1);
  	    //printf("Right = forward %d\n", rightMotorSpeed);
	} else if (rightMotorSpeed < 0){
		rightMotorSpeed = -1*rightMotorSpeed;
		ledc_set_duty(LEDC_HIGH_SPEED_MODE, right_motor_PWM, rightMotorSpeed);
		ledc_update_duty(LEDC_HIGH_SPEED_MODE, right_motor_PWM);
		gpio_set_level(DIRB, 0);
		//printf("Right = back\n");
	}
}

// handles websocket events
void websocket_callback(uint8_t num,WEBSOCKET_TYPE_t type,char* msg,uint64_t len){
	const static char* TAG = "Websocket";
	int value;
	switch(type) { // what kind of event happened
	case WEBSOCKET_CONNECT:
		ESP_LOGI(TAG,"client %i connected!",num);
		break;
    case WEBSOCKET_DISCONNECT_EXTERNAL:
    {
    	ESP_LOGI(TAG,"client %i sent a disconnect message",num);
    	tank.left_motor_speed = 0; // stop the robot due to connection error
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
    		//ESP_LOGI(TAG,"client sent a message:");
    		cJSON *rcvd_JSON = cJSON_Parse(msg);
    		int dataType = cJSON_GetObjectItem(rcvd_JSON,"dT")->valueint; //retrieve dataType from json
    		switch(dataType) {
    	       case 0:
    	       {
    	    	   ESP_LOGI(TAG,"Motor data received!");
    	    	   tank.left_motor_speed = cJSON_GetObjectItem(rcvd_JSON,"lS")->valueint;
    	    	   tank.right_motor_speed = cJSON_GetObjectItem(rcvd_JSON,"rS")->valueint;
    	    	   tank.is_updated = false;
    	    	   //printf("Left motor speed %d\n",tank.left_motor_speed);
    	    	   //printf("Right motor speed %d\n",tank.right_motor_speed);
    	       }
    	       	   break;
    	       case 1:
    	       {
    	    	   camera.horz = cJSON_GetObjectItem(rcvd_JSON,"h")->valueint;
    	    	   camera.vert = cJSON_GetObjectItem(rcvd_JSON,"v")->valueint;
    	    	   camera.light = cJSON_GetObjectItem(rcvd_JSON,"lt")->valueint;
    	    	   camera.is_updated = 0;
    	    	   ESP_LOGI(TAG,"Camera position data received!");
    	       }
    	       	   break;
    	       case 2:
    	       {
    	    	   tank.lights = cJSON_GetObjectItem(rcvd_JSON,"lts")->valueint;
    	    	   tank.is_updated = false;
    	    	   ESP_LOGI(TAG,"Lights data received!");
    	       }
    	       	   break;
    	       case 3:
    	       {
    	    	   hand.base = cJSON_GetObjectItem(rcvd_JSON,"b")->valueint;
    	    	   hand.elbow = cJSON_GetObjectItem(rcvd_JSON,"e")->valueint;
    	    	   hand.wrist = cJSON_GetObjectItem(rcvd_JSON,"w")->valueint;
    	    	   hand.claw = cJSON_GetObjectItem(rcvd_JSON,"c")->valueint;
    	    	   hand.is_updated = 0;
    	    	   ESP_LOGI(TAG,"Hand data received!");
    	    	}
    	       	   break;
    	   }
    		cJSON_Delete(rcvd_JSON);
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
	struct netbuf* inbuf;
	static char* buf;
	static uint16_t buflen;
	static err_t err;
	// web pages served by server
	netconn_set_recvtimeout(conn,1000); // allow a connection timeout of 1 second
	ESP_LOGI(TAG,"reading from client...");
	err = netconn_recv(conn, &inbuf); //receiving data from "conn" connection and put it to inbuf
	ESP_LOGI(TAG,"read from client");
	if(err==ERR_OK) {
		netbuf_data(inbuf, (void**)&buf, &buflen);
		if (buf) {
			// default page (if client requests just basic http page)
			if (strstr(buf,"GET / ") && !strstr(buf,"Upgrade: websocket")) { // if request type is get and doesn't want to upgrade to websocket
				ESP_LOGI(TAG,"Sending /"); // just send client basic web page
				netconn_write(conn, HTML_HEADER, sizeof(HTML_HEADER)-1,NETCONN_NOCOPY);
				netconn_close(conn);
				netconn_delete(conn);
				netbuf_delete(inbuf);
			}
			// default page websocket (if client wants to open websocket connection)
			else if(strstr(buf,"GET / ") && strstr(buf,"Upgrade: websocket")) {
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

void send_voltg() { // create and send a JSON containing raw ADC data
	char vltg_data[20]; //an array for JSON string
	int len;
	int clients;
	len = sprintf(vltg_data,"%s %d %s","{\"adc_raw\" : ", tank.bat_voltg, "}"); // fill the array
	clients = ws_server_send_text_all(vltg_data,len); // send to browser
}
static void car_state_update_task(void* pvParameters) {
    const static char* TAG = "Car";
    while(1) {
		if (!tank.is_updated) { //if new data was received
			set_motor_state(tank.left_motor_speed, tank.right_motor_speed);
			if (tank.lights) {
				gpio_set_level(lights_pin, 1);
			} else {
				gpio_set_level(lights_pin, 0);
			}
			ESP_LOGI(TAG," state was updated!\n");
			ESP_LOGI(TAG," battery voltage =  %f  V",voltage);
			send_voltg();
			tank.is_updated = 1;
		} else {
			//vTaskSuspend(NULL);
			vTaskDelay(1);
		}
		//vTaskDelay(10);
	}
}
static void hand_state_update_task (void* pvParameters) {
    const static char* TAG = "Hand";
    while(1) {
		if (!hand.is_updated) {
			setPWM(0, 4096-hand.base,hand.base);
			setPWM(1, 4096-hand.base,hand.base);
			setPWM(2, 4096-hand.elbow,hand.elbow);
			setPWM(3, 4096-hand.wrist,hand.wrist);
			setPWM(4, 4096-hand.claw,hand.claw);
			hand.is_updated = 1;
			ESP_LOGI(TAG," state was updated!\n");
			//printf("Hand position is %d, %d, %d, %d\n",hand.base,hand.elbow,hand.wrist,hand.claw);
			//printf("Hand position was updated!\n");
		} else {
			vTaskDelay(1);
		}
	}
}
static void camera_position_update_task (void* pvParameters) {
    const static char* TAG = "Camera";
    while(1) {
		if (!camera.is_updated) {
			setPWM(5, 4096-camera.horz,camera.horz);
			setPWM(6, 4096-camera.vert,camera.vert);
			//light output set level
			if (camera.light) {
				gpio_set_level(cam_light_contr, 1);
			} else {
				gpio_set_level(cam_light_contr, 0);
			}
			camera.is_updated = 1;
			ESP_LOGI(TAG," state was updated!\n");
		} else {
			vTaskDelay(1);
		}
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
    i2c_example_master_init();
    set_pca9685_adress(I2C_ADDRESS);
    resetPCA9685();
    setFrequencyPCA9685(50);  // 1000 Hz
    turnAllOff();
	ws_server_start();
	xTaskCreate(&server_task,"server_task",3000,NULL,6,NULL);
	xTaskCreate(&server_handle_task,"server_handle_task",4000,NULL,6,NULL);
	xTaskCreate(&car_state_update_task,"car_state_update_task",3000,NULL,6,NULL);
	xTaskCreate(&hand_state_update_task,"hand_state_update_task",3000,NULL,6,NULL);
	xTaskCreate(&camera_position_update_task,"camera_position_update_task",3000,NULL,6,NULL);
	xTaskCreate(&get_bat_voltg,"get_bat_voltg",3000,NULL,6,NULL);
}


