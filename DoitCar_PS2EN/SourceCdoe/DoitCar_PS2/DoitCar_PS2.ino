//材料：UNO开发板*1+2路电机&16路舵机驱动板*1+PS2游戏手柄套件*1+蜂鸣器*1
//功能：手柄控制
//IIC与A4、A5复用了，所以不能用到!!!
#include <PS2X_lib.h>  //for v1.6
/****************************IO引脚定义*****************************/
//电机引脚
#define PWMA 9 //左电机转速
#define DIRA 8 //左电机转向
#define DIRB 7 //右电机转向
#define PWMB 6 //右电机转速
//蜂鸣器引脚
#define BEEP_PIN 3 
//车头两LED灯
#define TLED_PIN 2
/*******************************************************************/
//设置引脚连接到PS2控制器：
#define PS2_DAT        13  //14    
#define PS2_CMD        11  //15
#define PS2_SEL        10  //16
#define PS2_CLK        12  //17
/******************************************************************
*选择PS2控制器的模式：
* - 压力=推杆的模拟读数
* - 震动？
*取消注释每个模式选择的行1
 ******************************************************************/
#define pressures   true
// #define pressures   false
#define rumble      true
// #define rumble      false
PS2X ps2x; //创建PS2控制器类
//现在，库不支持热插拔控制器，即连接控制器后，你必须重启Arduino
//蜂鸣器响/熄
#define BEEP_ON    digitalWrite(BEEP_PIN, LOW)//蜂鸣器响
#define BEEP_OFF   digitalWrite(BEEP_PIN, HIGH)//蜂鸣器熄
//车头两LED灯
#define TLED_ON digitalWrite(TLED_PIN, LOW);//亮灯
#define TLED_OFF digitalWrite(TLED_PIN, HIGH);//灭灯
//定义小车速度
uint8_t speedd=255;//小车正常速度
//小车控制标志量
enum DS
{
  MANUAL_DRIVE,//手动控制
}Drive_Status=MANUAL_DRIVE;
//电机控制标志量
enum DN
{ 
  GO_ADVANCE, 
  GO_LEFT, 
  GO_RIGHT,
  GO_BACK,
  STOP_STOP,
  DEF
}Drive_Num=DEF;
//电机控制相关
bool flag1=false;
bool stopFlag = true;
bool JogFlag = false;
uint16_t JogTimeCnt = 0;
uint32_t JogTime=0;

//PS2手柄
int error = 0;
byte type = 0;
byte vibrate = 0;
//电机控制
void Car_GO_FORWARD(uint8_t spel, uint8_t sper, int tim=0)//小车前进
{
  uint8_t speeddl=spel;
  uint8_t speeddr=sper;
  digitalWrite(DIRA,HIGH);
  analogWrite(PWMA,speeddl);
  digitalWrite(DIRB,HIGH);
  analogWrite(PWMB,speeddr);
  delay(tim);
}
void Car_GO_LEFT(uint8_t spel,uint8_t sper, int tim=0)//车体左转    
{
  uint8_t speeddl=spel;
  uint8_t speeddr=sper;
  digitalWrite(DIRA,LOW);
  analogWrite(PWMA,speeddl);
  digitalWrite(DIRB,HIGH);
  analogWrite(PWMB,speeddr);
  delay(tim);
}  
void Car_GO_RIGHT(uint8_t spel,uint8_t sper, int tim=0)//车体右转
{
  uint8_t speeddl=spel;
  uint8_t speeddr=sper;
  digitalWrite(DIRA,HIGH);
  analogWrite(PWMA,speeddl);
  digitalWrite(DIRB,LOW);
  analogWrite(PWMB,speeddr);
  delay(tim);
}
void Car_GO_STOP(uint8_t spel,uint8_t sper, int tim=0)//车体停止
{
  uint8_t speeddl=spel;
  uint8_t speeddr=sper;
  digitalWrite(DIRA,LOW);
  analogWrite(PWMA,speeddl);
  digitalWrite(DIRB,LOW);
  analogWrite(PWMB,speeddr);
  delay(tim);
}
void Car_GO_BACK(uint8_t spel,uint8_t sper, int tim=0)//车体后退
{ 
  uint8_t speeddl=spel;
  uint8_t speeddr=sper;
  digitalWrite(DIRA,LOW);
  analogWrite(PWMA,speeddl);
  digitalWrite(DIRB,LOW);
  analogWrite(PWMB,speeddr);
  delay(tim);
}
////////////////////////////////////////////////////////////////////////////////////
// 重置功能
void (* resetFunc) (void) = 0;

void check_PS2()
{
  error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, pressures, rumble); 
  if(error == 0)
  {
    Serial.print(F("Found Controller, configured successful "));//Found Controller，配置成功
  }  
  else if(error == 1)//没有控制器，检查接线，使调试方法见。访问故障提示www.billporter.info
    Serial.println(F("No controller found!")); 
  else if(error == 2)//发现控制器但不接受命令。方法见启用调试。访问故障提示www.billporter.info
    Serial.println(F("Controller found but not accepting commands. "));
  else if(error == 3)//控制器拒绝进入压力模式，可能不支持。
    Serial.println(F("Controller not support  Pressures mode.")); 
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
      Serial.println(F("Wireless Sony DualShock Controller found "));//无线索尼DualShock控制器发现
      break;
   }
}

void PS2_Control()
{
  /* 您必须读取Gamepad才能获取新值并设置振动值
     ps2x.read_gamepad（小电机开/关，较大电机强度从0-255）
     如果不开启震动功能，请使用ps2x.read_gamepad(); 
     没有值你应该至少称之为一秒钟
  */  
  if(error == 1)
  { //如果没有控制器发现，跳过循环
    resetFunc();
  } 
  if(type == 2)
  { //guitarhero控制器
    ps2x.read_gamepad();//读控制器 
    return;
  }
  else 
  { //DualShock控制器
    ps2x.read_gamepad(false, vibrate); //读取控制器并设置大电机以“振动”速度旋转  
    if(ps2x.Button(PSB_START)) //按下START启动遥控        只要按下按钮，它将为TRUE
      Serial.println(F("Start !"));
    if(ps2x.Button(PSB_SELECT))//按下SELECT(选择)   停止
    {
      Drive_Status=MANUAL_DRIVE; 
      Drive_Num=STOP_STOP;     
    }
    if(ps2x.Button(PSB_PAD_UP))//按下 UP上 前进
    {
      speedd = map(ps2x.Analog(PSAB_PAD_UP),0,255,100,255);//方向按键压力转化为小车的速度--越用力按压速度越快，下同
      Drive_Status=MANUAL_DRIVE; 
      Drive_Num=GO_ADVANCE;
    }
    if(ps2x.Button(PSB_PAD_RIGHT))//按下 RIGHT右  右转
    {
      speedd = map(ps2x.Analog(PSAB_PAD_RIGHT),0,255,100,255);
      Drive_Status=MANUAL_DRIVE; 
      Drive_Num=GO_RIGHT;
    }
    if(ps2x.Button(PSB_PAD_LEFT))//按下 LEFT左  左转
    {
      speedd = map(ps2x.Analog(PSAB_PAD_LEFT),0,255,100,255);
      Drive_Status=MANUAL_DRIVE; 
      Drive_Num=GO_LEFT;
    }
    if(ps2x.Button(PSB_PAD_DOWN))//按下 DOWN下  后退
    {
      speedd = map(ps2x.Analog(PSAB_PAD_DOWN),0,255,100,255);
      Drive_Status=MANUAL_DRIVE; 
      Drive_Num=GO_BACK;
    }   
    vibrate = ps2x.Analog(PSAB_CROSS);  //这将根据您按下蓝色（X）按钮的难度设置大型电机振动速度
    if (ps2x.NewButtonState()) 
    { //如果任何按钮改变状态（关闭或关闭至开启）将为TRUE
      if(ps2x.Button(PSB_L3))//按下左摇杆
      {
        Serial.println(F("L3 pressed"));
      }
      if(ps2x.Button(PSB_TRIANGLE))//按下 △ 
      {
        BEEP_ON;//蜂鸣器响
        TLED_ON;//亮灯
      }  
    }
    if(ps2x.NewButtonState(PSB_CROSS)) //按下按下 X             
    {
      BEEP_OFF;//蜂鸣器灭
      TLED_OFF;//灭灯
    }
    if(ps2x.Button(PSB_R3))//按下右摇杆 摇动左摇杆控制小车前进后退左右转
    {
      if(ps2x.Analog(PSS_LX)>127)
      {
        speedd = map(ps2x.Analog(PSS_LX),127,255,0,255);
        Drive_Status=MANUAL_DRIVE; 
        Drive_Num=GO_RIGHT;
      }
      if(ps2x.Analog(PSS_LX)<127)
      {
        speedd = map(ps2x.Analog(PSS_LX),127,0,0,255);
        Drive_Status=MANUAL_DRIVE; 
        Drive_Num=GO_LEFT;
      }
      if(ps2x.Analog(PSS_LY)>127) 
      {
        speedd = map(ps2x.Analog(PSS_LY),127,255,0,255);
        Drive_Status=MANUAL_DRIVE; 
        Drive_Num=GO_BACK;
      }
      if(ps2x.Analog(PSS_LY)<127) 
      {
        speedd = map(ps2x.Analog(PSS_LY),127,0,0,255);
        Drive_Status=MANUAL_DRIVE; 
        Drive_Num=GO_ADVANCE;
      }
    }
  }
  delay(50);  
}
////////////////////////////////////////////////////////////////////////////////////

//小车控制
void CAR_Control()
{
  if(Drive_Status == MANUAL_DRIVE)
  {
    switch (Drive_Num) 
    {
      case GO_ADVANCE:Car_GO_FORWARD(speedd,speedd);JogFlag = true;JogTimeCnt = 1;JogTime=millis();break;
      case GO_LEFT: Car_GO_LEFT(speedd,speedd);JogFlag = true;JogTimeCnt = 1;JogTime=millis();break;
      case GO_RIGHT:Car_GO_RIGHT(speedd,speedd);JogFlag = true;JogTimeCnt = 1;JogTime=millis();break;
      case GO_BACK:Car_GO_BACK(speedd,speedd);JogFlag = true;JogTimeCnt = 1;JogTime=millis();break;
      case STOP_STOP: Car_GO_STOP(0,0); JogTime = 0;JogFlag=false;stopFlag=true;break;
      default:break;
    }
    Drive_Num=DEF;
    //小车保持姿态300ms
    if(millis()-JogTime>=300)
    {
      JogTime=millis();
      if(JogFlag == true) 
      {
        stopFlag = false;
        if(JogTimeCnt <= 0) 
        {
          JogFlag = false; stopFlag = true;
        }
        JogTimeCnt--;
      }
      if(stopFlag == true) 
      {
        JogTimeCnt=0;
        Car_GO_STOP(0,0);
      }
    }
  }
}
//IO初始化
void GPIO_init()
{
  //电机
  pinMode(PWMA, OUTPUT);
  pinMode(DIRA, OUTPUT);
  pinMode(PWMB, OUTPUT);
  pinMode(DIRB, OUTPUT);
  //蜂鸣器
  pinMode(BEEP_PIN, OUTPUT);
  //车头两LED灯
  pinMode(TLED_PIN, OUTPUT);
}
/**********************************************************************************************/
void setup()
{
  Serial.begin(9600); 
  GPIO_init();
  delay(500); //添加延迟给无线ps2模块一些启动时间，在配置之前
  check_PS2();
  Car_GO_STOP(0,0);
  BEEP_OFF;//蜂鸣器熄
  TLED_OFF;
}
void loop()
{
  PS2_Control();//手柄控制
  CAR_Control();//小车控制
}
/***********************************************************************************************/
















































































