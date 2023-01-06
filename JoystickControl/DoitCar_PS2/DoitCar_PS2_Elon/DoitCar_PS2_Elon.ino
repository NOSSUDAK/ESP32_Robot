
#include <PS2X_lib.h>  //for v1.6
/****************************IO引脚定义*****************************/
// Motor and devices pins
#define PWMA 9 //
#define DIRA 8 //
#define DIRB 7 //
#define PWMB 6 //
#define BEEP_PIN 3 
#define TLED_PIN 2
#define Forward 0
#define Back 1
/*******************************************************************/
//Pins used for PS2 receiver：
#define PS2_DAT        13  //14    
#define PS2_CMD        11  //15
#define PS2_SEL        10  //16
#define PS2_CLK        12  //17
/**********************************************************/

#define pressures   false
#define rumble      true
PS2X ps2x; //Create

#define BEEP_OFF   digitalWrite(BEEP_PIN, HIGH)
#define TLED_ON digitalWrite(TLED_PIN, HIGH);
#define TLED_OFF digitalWrite(TLED_PIN, LOW);
int left;
int right;
uint8_t MotorSpeed=0;// Motor speed
const int Pin_tone = 3; // номер порта зуммера
//частоты ноты
int frequences[3] = {  700, 900, 1000};
//длительность нот
int durations[3] = {  50, 50, 50};
//PS2
int error = 0;
byte type = 0;
byte vibrate = 0;
//电机控制
void hornSound()
{
  for (int i = 0; i <= 3; i++) { // Цикл от 0 до количества нот
    tone(Pin_tone, frequences[i]); // Включаем звук, определенной частоты
    delay(durations[i]);  // Дауза для заданой ноты
    noTone(Pin_tone); // Останавливаем звук
    BEEP_OFF;
    }
}
void reportVoltage()
{
  float batVoltage;    // Battery voltage value
  String stringVoltage;// Battery voltage
  batVoltage = 2*analogRead(A1) * (5.0 / 1023.0);
  stringVoltage = String("Voltage = ");
  stringVoltage += batVoltage;
  stringVoltage += " V";
  Serial.println(stringVoltage);// print out the value you read:
}
void LeftMotorControl(uint8_t LMSpeed,uint8_t dir)
{
  if(dir == 0)
  {
    digitalWrite(DIRA,HIGH);
  }
  else
  {
    digitalWrite(DIRA,LOW);
  }
  analogWrite(PWMA,LMSpeed);
 }
 
void RightMotorControl(uint8_t RMSpeed,uint8_t dir)
{
  if(dir == 0)
  {
    digitalWrite(DIRB,HIGH);
  }
  else
  {
    digitalWrite(DIRB,LOW);
  }
  analogWrite(PWMB,RMSpeed);
}

void (* resetFunc) (void) = 0;

void check_PS2()
{
  error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, pressures, rumble); 
  if(error == 0)
  {
    Serial.print(F("Found Controller, configured successful "));//Found Controller，
  }  
  /*
  else if(error == 1)//www.billporter.info
    Serial.println(F("No controller found!")); 
  else if(error == 2)//www.billporter.info
    Serial.println(F("Controller found but not accepting commands. "));
  else if(error == 3)
    Serial.println(F("Controller not support  Pressures mode.")); 
    */
  type = ps2x.readType(); 
  switch(type) 
  {
    case 0:
      Serial.println(F("Unknown Controller type found "));//发现未知控制器类型
      break;
    case 1:
      Serial.println(F("DualShock Controller found "));//发现DualShock控制器////////////////////////////
      break;
    case 2:
      Serial.println(F("GuitarHero Controller found "));//guitarhero控制器发现
      break;
    case 3:
      Serial.println(F("Wireless Sony DualShock Controller found "));//DualShock
      break;
   }
}

void PS2_Control()
{
  if(error == 1)
  { 
    resetFunc();
  } 
  else 
  { //DualShock
    ps2x.read_gamepad(false, vibrate); 
    
    
    if (ps2x.NewButtonState()) 
    { 
      
      if(ps2x.Button(PSB_TRIANGLE))// △ 
      {
        TLED_ON;
        hornSound();
      }  
    }
    if(ps2x.NewButtonState(PSB_CROSS)) // X             
    {
      TLED_OFF;
      reportVoltage();
    }
      /*
      if(ps2x.Analog(PSS_LX)>127)
      {
        MotorSpeed = map(ps2x.Analog(PSS_LX),127,255,0,255);
        Drive_Status=MANUAL_DRIVE; 
        Drive_Num=GO_RIGHT;
      }
      if(ps2x.Analog(PSS_LX)<127)
      {
        MotorSpeed = map(ps2x.Analog(PSS_LX),127,0,0,255);
        Drive_Status=MANUAL_DRIVE; 
        Drive_Num=GO_LEFT;
      }*/
    if(ps2x.Button(PSB_PAD_UP))//
    {
      MotorSpeed = 255;
      LeftMotorControl(MotorSpeed,Forward);
      RightMotorControl(MotorSpeed,Forward);
    }
    else if(ps2x.Button(PSB_PAD_DOWN))//
    {
      MotorSpeed = 255;
      LeftMotorControl(MotorSpeed,Back);
      RightMotorControl(MotorSpeed,Back);
    }
    else
    {
      if(ps2x.Analog(PSS_LY)>127) 
      {
        MotorSpeed = map(ps2x.Analog(PSS_LY),127,255,0,255);
        LeftMotorControl(MotorSpeed,Back);
      }
      if(ps2x.Analog(PSS_LY)<127) 
      {
        MotorSpeed = map(ps2x.Analog(PSS_LY),127,0,0,255);
        LeftMotorControl(MotorSpeed,Forward);
      }

      if(ps2x.Analog(PSS_RY)>127) 
      {
        MotorSpeed = map(ps2x.Analog(PSS_RY),127,255,0,255);
        RightMotorControl(MotorSpeed,Back);
      }
      
      if(ps2x.Analog(PSS_RY)<127) 
      {
        MotorSpeed = map(ps2x.Analog(PSS_RY),127,0,0,255);
        RightMotorControl(MotorSpeed,Forward);
      }
    }
  }
  delay(50);  
}

void GPIO_init()
{
  //Motor shield inputs
  pinMode(PWMA, OUTPUT);
  pinMode(DIRA, OUTPUT);
  pinMode(PWMB, OUTPUT);
  pinMode(DIRB, OUTPUT);
  
  pinMode(TLED_PIN, OUTPUT);
}
/**********************************************************************************************/
void setup()
{
  Serial.begin(9600); 
  GPIO_init();
  delay(500); 
  check_PS2();
  TLED_OFF;
}
void loop()
{
  PS2_Control();
}
/***********************************************************************************************/
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    //char inChar = (char)Serial.read();
    left = Serial.parseInt();
    right = Serial.parseInt();
    LeftMotorControl(left,Forward);
    RightMotorControl(right,Forward);
    // add it to the inputString:
    //inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    
  }
}
