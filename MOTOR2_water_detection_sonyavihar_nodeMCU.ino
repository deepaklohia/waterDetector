//DEVELOPED BY Deepak Lohia at www.dlohia.com

#define BLYNK_TEMPLATE_ID "blynk"
#define BLYNK_TEMPLATE_NAME "MOTOR2"
#define BLYNK_FIRMWARE_VERSION "0.0.4"

#define ON HIGH
#define OFF LOW

#define BLYNK_PRINT Serial
//#define BLYNK_DEBUG
//#define APP_DEBUG

// Uncomment your board, or configure a custom board in Settings.h
//#define USE_SPARKFUN_BLYNK_BOARD
#define USE_NODE_MCU_BOARD
//#define USE_WITTY_CLOUD_BOARD
//#define USE_WEMOS_D1_MINI

#include "BlynkEdgent.h"
 
const int buff_size_sv = 5; //taking last 10 values of the SV to determine if there is water
long arr_sv[buff_size_sv];
unsigned long startMilliSec ;

int sensorLimit = 860;
int sensorValue = 0 ;
bool motorFlag = false;
bool motorDayCheck = false;
String txt;

int currentYear;
int currentMonth;
int currentDay;
bool motorDay = false;
bool motorDayOriginal ;
String motorDayStr;

unsigned long tme ;
unsigned long seconds ;
unsigned long minutes ;
unsigned long hours ;
unsigned long days ;

//***************************PINS*********************START

//NODE MCU BASED
static const uint8_t D0   = 16;
static const uint8_t D1   = 5; //SAFE PIN LOW ON BOOT 
static const uint8_t D2   = 4; //SAFE PIN LOW ON BOOT
static const uint8_t D3   = 0;
static const uint8_t D4   = 2;
static const uint8_t D5   = 14;
static const uint8_t D6   = 12; //SAFE PIN - HIGH ON BOOT
static const uint8_t D7   = 13; //SAFE PIN - HIGH ON BOOT
static const uint8_t D8   = 15; // SAFE PIN HIGH ON BOOT
static const uint8_t RX   = 3;
static const uint8_t TX   = 1;

const int sensorPin = A0;
const int motorRelay = D1 ;
const int pullupAlexaStart = D5 ; 
bool forceDaily = false;
bool pullupDayChange = false;
bool srPrint = false;
bool tempStart;

bool print_flag = false;
bool nowater_flag = false;
//unsigned long waitBeforeOff  = 2UL * 60UL * 1000UL;  //2 Min **** HOW MANY SECONDS TO WAIT BEFORE MOTOR SWITCH OFF
unsigned long waitBeforeOff ;
int waitBeforeOffSec ;

unsigned long timeOverMill;
bool test_run = false;

unsigned long motorStartTimeMillis ;
unsigned long motorEndTimeMillis ;
unsigned long motorTotalMillis ;
unsigned long lastMotorTotalMillis ;
String ranFor = "";

int lastWaterDay = 7  ;  
int lastWaterMonth = 5 ;  
int lastWaterYear = 2024 ; 

BLYNK_CONNECTED(){
  Blynk.syncVirtual(V1);  //M1 / M2 MOTOR STATUS
  Blynk.syncVirtual(V2);  //M2 /M2 SV VALUE
  Blynk.syncVirtual(V4);  //M2 SV LIMIT
  Blynk.syncVirtual(V5);  //INFO DISPLAY
  Blynk.syncVirtual(V6);  //Force Daily
  Blynk.syncVirtual(V7);  //Value Day Change
  Blynk.syncVirtual(V8);  //TEMP START
  Blynk.syncVirtual(V9);  //wait before off
  Blynk.sendInternal("utc", "iso");       // ISO-8601 formatted time
}

//event when value changes
BLYNK_WRITE(V4){
  int value = param.asInt();
  sensorLimit = value;  
}

//Force Daily 
BLYNK_WRITE(V6){
  forceDaily = param.asInt();
}

//water day change
BLYNK_WRITE(V7){
  pullupDayChange = param.asInt();
}

//water day change
BLYNK_WRITE(V8){
  tempStart = param.asInt();
}

//wait before off
BLYNK_WRITE(V9){
  waitBeforeOffSec = param.asInt();
  waitBeforeOff  = waitBeforeOffSec * 1000UL;
}

// Receive UTC data
BLYNK_WRITE(InternalPinUTC) {
    String cmd = param[0].asStr();
    if (cmd == "tz_name") {
      String tz_name = param[1].asStr();
      printV5("Timezone: " +tz_name);
    }
    else if (cmd == "iso") {
      String iso_time = param[1].asStr();
      printV5("ISO-8601 time:   "+ iso_time);

      int pos = iso_time.indexOf("T") ;
      String raw = iso_time.substring(0, pos ); //getting just date data

      pos = raw.indexOf("-") ; //getting position of the -
      String year = raw.substring(0,pos);

      raw = raw.substring( pos + 1 , raw.length() ); //removing year info in new raw string
      pos = raw.indexOf("-") ;
      String month = raw.substring(0,pos);
      String day = raw.substring( pos + 1 );
      currentYear = year.toInt();
      currentMonth = month.toInt();
      currentDay = day.toInt();
    }
}
void setup()
{
  Serial.begin(115200);
  delay(100);
  BlynkEdgent.begin();

  pinMode(motorRelay, OUTPUT) ;
  pinMode(pullupAlexaStart , INPUT_PULLUP) ;
  digitalWrite(motorRelay , OFF) ;
  
  Blynk.virtualWrite(V1, 0 );  
  Blynk.virtualWrite(V2, 0 );  
  Blynk.virtualWrite(V5,  "" );

  //SV DEFAULT VALUES
  for (int i = 0; i < buff_size_sv; i++) {
    arr_sv[i] = 1000;
  }
}

void loop() {
  BlynkEdgent.run();

  //if (!motorDayCheck){
  if (!motorDayCheck && currentYear > 100){
    getMotorDay();
    motorDayCheck = true;
  }

  if (pullupDayChange){
    //we will make the values vise versa if pullup is on
    if (motorDayOriginal){motorDay = false; }
    else{ motorDay = true; } 
  }else{
    motorDay = motorDayOriginal;
  }
  
  if (motorDay){ motorDayStr = "YES"; }
  else{  motorDayStr = "NO"; }

  sensorValue = get_avg_sv(analogRead(sensorPin));

 //AUTO DETECT WATER
  if (sensorValue < sensorLimit && !motorFlag){
    digitalWrite(motorRelay, ON); //START MOTOR 1
    motorFlag = true ; 
    printV5("MTR2:WTR DETD");
    test_run = false;
    motorStartTimeMillis = millis() ;
  }
  //IF FORCE START IS ON 
  else if ( (digitalRead(pullupAlexaStart) == 0 && (motorDay || forceDaily))  || tempStart  ) {
    if (!motorFlag){
      test_run = true;
      digitalWrite(motorRelay, ON); //START MOTOR 1
      motorFlag = true;
      startMilliSec = millis();
    }
    tme = millis()  -  startMilliSec ;
    seconds = tme / 1000;
    minutes = seconds / 60;
    hours = minutes / 60;
    days = hours / 24;

    tme %= 1000;
    seconds %= 60;
    minutes %= 60;
    hours %= 24;

    if (tempStart){
      txt = "TEMP-START-M2: ";
    }
    else{
      txt = "TESTING-M2: ";
    }
    txt = txt + padZero(hours) + ":" + padZero(minutes) + ":" + padZero(seconds) ;
    //printV5("TESTING MTR2: " + padZero(hours) + ":" + padZero(minutes) + ":" + padZero(seconds)) ; 
    printV5(txt) ; 
  }

  // WAIT FOR WATER BEFORE OFF .. ONLY WHEN ITS NOT RUN VIA ALEXA
  //TURN OFF THE MOTOR IF ITS ON.
  //>>> SENSOR VALUE IS DOWN --- EITHER PULLUP IS OFF , OR PULLUP IS ON --- BUT ITS NEITHER A WATER DAY AND NOR A FORCE START
  else if ((sensorValue > sensorLimit && motorFlag && !tempStart) && (digitalRead(pullupAlexaStart) == 1 || (digitalRead(pullupAlexaStart) == 0 && !motorDay && !forceDaily)) ){
    sensorValue =get_avg_sv(analogRead(sensorPin));

    //if we are NOT doing test run the we make motor wait before off
    if (!test_run){
      if (!nowater_flag){
        nowater_flag = true ;
        startMilliSec = millis() ;
        timeOverMill =  startMilliSec + waitBeforeOff ;
      }

      tme = waitBeforeOff -  (millis()  -  startMilliSec) ;   
      seconds = tme / 1000;
      minutes = seconds / 60;
      hours = minutes / 60;
      days = hours / 24;

      tme %= 1000;
      seconds %= 60;
      minutes %= 60;
      hours %= 24;
  
      txt  = padZero(hours) + ":" + padZero(minutes) + ":" + padZero(seconds);
      printV5("M2:NO WATER " + txt + " SV2:" + String(sensorValue) );
      delay(1000) ;

      //if it was a test run then stop motor after test run
      if (millis() > timeOverMill){
        sensorValue = get_avg_sv(analogRead(sensorPin));
        if (sensorValue > sensorLimit) {
          digitalWrite(motorRelay, OFF); //STOP MOTOR 2
          motorFlag = false;
          nowater_flag = false;
          motorEndTimeMillis = millis() ;
          ranFor = getMillisummery();
          delay(2000);
        }
      }
    }
    //if it was a test run then stop motor immediately
    else{
      digitalWrite(motorRelay, OFF); //STOP MOTOR 2
      motorFlag = false;
      test_run = false;
      printV5("M2:NO WATER DTD SV2:" + String(sensorValue) );
    }
  }
    //reset no water
  else if ((sensorValue < sensorLimit && motorFlag && nowater_flag ) ){
    nowater_flag = false;
  }
 
  Blynk.beginGroup();
  if (motorFlag){ 
    Blynk.virtualWrite(V1, 1);
    txt = "M2:ON SL2:" + String(sensorValue) + " [LMT:" + String(sensorLimit) + "]" ;
  } else{ 
    Blynk.virtualWrite(V1, 0); 
    txt = "M2:OFF SL2:" + String(sensorValue) + " [LMT:" + String(sensorLimit) + "]" ;

    //motor alreadyran
    if (ranFor.length()> 0){
      txt = ranFor ;
    }
  }  

  Blynk.virtualWrite(V2, sensorValue); //UPDATING M2 SENSOR VALUE

  if (!nowater_flag){
     if (print_flag){
      txt = "DATE: " + String(currentDay)  + "-" + getMonthStr(currentMonth) + "-" + String(currentYear)  + " DY:" +  motorDayStr ;
      print_flag = false;
    }
    else{
      print_flag = true;
    }
    Blynk.virtualWrite(V5,  txt );
  }
  Blynk.endGroup();
  delay(3000);
}

void printV5(String txt){
  Blynk.virtualWrite(V5,  txt ); //INFO
  if (srPrint){ Serial.println(txt);}
  delay(2000);
}

/* we are taking last x values of the SV value to see the actual status of SV */
int get_avg_sv(int val_sv)
{
  long total = 0;
  byte count = 0;
  static byte i_sv = 0;
 
  arr_sv[i_sv++] = val_sv;
  if (i_sv == buff_size_sv){ i_sv = 0;}
  
  while (count < buff_size_sv) {
    total = total + arr_sv[i_sv++];
    if (i_sv == buff_size_sv){i_sv = 0;}
    count++;
  }
  return total/buff_size_sv;
}

void getMotorDay(){
  txt =  "Todays Date:" + String(currentDay)  + "/" + String(currentMonth) + "/" + String(currentYear) ;  
  printV5(txt);  
  
  txt =  "Last Water Date :" + String(lastWaterDay)  + "/" + String(lastWaterMonth) + "/" + String(lastWaterYear) ;  
  printV5(txt);  
  
  int daydiff = dateDiff(lastWaterYear,  lastWaterMonth,  lastWaterDay , currentYear,  currentMonth,  currentDay)  ;

  /*
  if (pullupDayChange){
    daydiff =  daydiff + 1 ;
    txt = "Pull-up ON >> Adding 1 Day";  
  }
  else{
    txt = "Pull-up OFF >> NO Adjustments";  
  }
  */
  //printV5(txt);  
 
  int dayRem = daydiff % 2 ;
  txt = "Days Diffrnce = " + String(daydiff) + " Remainder = " + String(dayRem);
  //printV5(txt);   

  if (dayRem != 0){ 
    txt = "TODAY IS NOT MOTOR DAY" ;
    motorDayOriginal =  false;
    } else {
    txt = "TODAY IS MOTOR DAY" ;
    motorDayOriginal =  true;
    }  

  printV5(txt);  
  txt = "";
}

String getMillisummery(){
  motorTotalMillis = motorEndTimeMillis - motorStartTimeMillis;
  if (lastMotorTotalMillis > 0){ motorTotalMillis += lastMotorTotalMillis ;}
  lastMotorTotalMillis = motorTotalMillis;

  tme = motorTotalMillis;
  seconds = tme / 1000;
  minutes = seconds / 60;
  hours = minutes / 60;
  days = hours / 24;

  tme %= 1000;
  seconds %= 60;
  minutes %= 60;
  hours %= 24;
  return "M2 RAN FOR:" + padZero(hours) + ":" + padZero(minutes) + ":" + padZero(seconds)   ;
}

int dateDiff(int year1, int mon1, int day1, int year2, int mon2, int day2){
  int ref,dd1,dd2,i;
  int leapCnt1 = getLeap(year1);
  int leapCnt2 = getLeap(year2);
  
  ref = year1;
  if(year2<year1){ ref = year2;}

  dd1=0;
  dd1=dater(mon1);

  for(i=ref;i<year1;i++)
  {
    if(i%4==0){
      dd1+=1;
    }
  }
  dd1=dd1+day1+(year1-ref)*leapCnt1;
  dd2=0;
  for(i=ref;i<year2;i++)
  {
    if(i%4==0)
    dd2+=1;
  }
  dd2=dater(mon2)+dd2+day2+((year2-ref)*leapCnt2);
  return dd2-dd1;
}

int dater(int x){
  const int dr[]= { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
  return dr[x-1];
}

int getLeap(int yr){
  if (yr == 2020 || yr == 2024 || yr == 2028 || yr == 2032 ) {return 366 ;}
  else{ return 365; }
}

String padZero(int val){
  if (val < 10){ return "0" + String(val); }
  else{ return String(val); }
}

String getMonthStr(int m){
  String arr[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
  return arr[m-1];
}

