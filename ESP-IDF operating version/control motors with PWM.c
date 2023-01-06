

/* LEDC (LED Controller) fade example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"


#define DIRB 14
#define PWMB 27
#define DIRA 12
#define PWMA 13
#define left_motor_PWM 0  // PWM channels used to control motors
#define right_motor_PWM 1
#define LED_OUT 25 // head lights output
int duty = 0;
static void PWM_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t PWM_timer; // create an object with PWM timer parameters
    PWM_timer.speed_mode       = LEDC_HIGH_SPEED_MODE;
    PWM_timer.timer_num        = LEDC_TIMER_1;
    PWM_timer.duty_resolution  = LEDC_TIMER_8_BIT;
    PWM_timer.freq_hz          = 100;  // set output frequency at 1 Hz
    ledc_timer_config(&PWM_timer); // pass it to config function

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t left_motor; // create an object with PWM channel parameters
    left_motor.speed_mode = LEDC_HIGH_SPEED_MODE;
    left_motor.channel    = left_motor_PWM;
    left_motor.timer_sel  = LEDC_TIMER_1;
    left_motor.intr_type  = LEDC_INTR_DISABLE;
    left_motor.gpio_num   = PWMA;
    left_motor.duty       = 0; // set duty at 0
    ledc_channel_config(&left_motor);// pass it to config function

    ledc_channel_config_t right_motor;
    right_motor.speed_mode = LEDC_HIGH_SPEED_MODE;
    right_motor.channel    = right_motor_PWM;
    right_motor.timer_sel  = LEDC_TIMER_1;
    right_motor.intr_type  = LEDC_INTR_DISABLE;
    right_motor.gpio_num   = PWMB;
    right_motor.duty       = 0; // set duty at 0
    ledc_channel_config(&right_motor);
}
void gpio_init()
{
	gpio_pad_select_gpio(DIRA);
	gpio_set_direction(DIRA, GPIO_MODE_OUTPUT);
	gpio_pad_select_gpio(DIRB);
	gpio_set_direction(DIRB, GPIO_MODE_OUTPUT);
}
void set_motor()
{
	while (1) {
	    	for (int i = 0; i < 25; i++) {
	    		ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty);
	    		ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
	    	    printf("PWM value \n");
	    	    duty +=10;
	    	    vTaskDelay(100 / portTICK_PERIOD_MS);
	    	    }
	    	duty = 0;
	    }
}
void setMotorState(int leftMotorSpeed, int rightMotorSpeed)
{
	if (leftMotorSpeed >= 0){
	 ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, leftMotorSpeed);
  	    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
  	    gpio_set_level(DIRA, 1);
	} else if (leftMotorSpeed < 0){
		leftMotorSpeed = -1*leftMotorSpeed;
		ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, leftMotorSpeed);
		ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
		gpio_set_level(DIRA, 0);
	}
	if (rightMotorSpeed >= 0){

		gpio_set_level(DIRB, 1);
		} else if (rightMotorSpeed < 0){
			rightMotorSpeed = -1*rightMotorSpeed;

			gpio_set_level(DIRA, 0);
		}
}
void app_main()
{
	PWM_init();

	xTaskCreate(&set_motor, "set_motor", configMINIMAL_STACK_SIZE, NULL, 5, NULL);

}


