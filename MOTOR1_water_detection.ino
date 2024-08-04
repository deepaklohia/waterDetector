#define BLYNK_TEMPLATE_ID "TAKE FROM BLYNK"
#define BLYNK_TEMPLATE_NAME "MOTOR1"
#define BLYNK_FIRMWARE_VERSION "0.0.5"

#define BLYNK_PRINT Serial
//#define BLYNK_DEBUG
//#define APP_DEBUG

// Uncomment your board, or configure a custom board in Settings.h
//#define USE_SPARKFUN_BLYNK_BOARD
//#define USE_NODE_MCU_BOARD
//#define USE_WITTY_CLOUD_BOARD
#define USE_WEMOS_D1_MINI

#include "BlynkEdgent.h"

/*
//D1 MINI 
static const uint8_t D5   = 14; //SAFE TO USE?
*/

//NODE MCU
static const uint8_t D1 = 5;  //SAFE PIN - LOW ON BOOT =

const int sensorPin = A0;
const int motorRelay = D1 ; 
String txt;

int sensorLimit = 850;
int sensorValue = 0 ;

const int buff_size_sv = 10; //taking last 10 values of the SV to determine if there is water
long arr_sv[buff_size_sv];

bool tempRun = false;
bool motorFlag = false;
bool water_detected ;

unsigned long startMilliSec ;

unsigned long tme ;
unsigned long seconds ;
unsigned long minutes ;
unsigned long hours ;
unsigned long days ;
bool nowater_flag = false;
unsigned long waitBeforeOff ; 
//unsigned long waitBeforeOff  = 2UL * 60UL * 1000UL;  //2 Min **** HOW MANY SECONDS TO WAIT BEFORE MOTOR SWITCH OFF
int waitBeforeOffSec ;

unsigned long tempRunTimeout ; 
int tempRunMin ;

unsigned long timeOverMill;
unsigned long motorStartTimeMillis ;
unsigned long motorEndTimeMillis ;
unsigned long motorTotalMillis ;
unsigned long lastMotorTotalMillis ;
String ranFor = "";
String timeStamp;
bool forceStart;

BLYNK_CONNECTED(){
  Blynk.syncVirtual(V1);  //M1 / M2 MOTOR STATUS
  Blynk.syncVirtual(V2);  //M2 /M2 SV VALUE
  Blynk.syncVirtual(V3);  //M1 SL LIMIT
  Blynk.syncVirtual(V5);  //INFO DISPLAY
  Blynk.syncVirtual(V6);  // wait before off
  Blynk.syncVirtual(V7);  // force start
  Blynk.syncVirtual(V8);  // temp run min
  Blynk.sendInternal("utc", "iso");       // ISO-8601 formatted time

  /*
  Blynk.virtualWrite(V1, timeStamp);
  Blynk.virtualWrite(V2, tankValPerct);
  Blynk.syncVirtual(V5);  //INFO DISPLAY
  */
}

//event when value changes
BLYNK_WRITE(V3){
  int value = param.asInt();
  sensorLimit = value;  
}

//wait before off
BLYNK_WRITE(V6){
  waitBeforeOffSec = param.asInt();
  waitBeforeOff  = waitBeforeOffSec * 1000UL;
}

BLYNK_WRITE(V7){
  int value = param.asInt();
  if (value){ forceStart = true; }
  else{ forceStart = false;}
}

//Temp run for
BLYNK_WRITE(V8){
  tempRunMin = param.asInt();
  tempRunTimeout =  tempRunMin * 60UL * 1000UL;  
}

// Receive UTC data
BLYNK_WRITE(InternalPinUTC) {
    String cmd = param[0].asStr();
    if (cmd == "tz_name") {
      String tz_name = param[1].asStr();
      //printV5("Timezone: " +tz_name);
    }
    else if (cmd == "iso") {
      String iso_time = param[1].asStr();
      //printV5("ISO-8601 time:   "+ iso_time);

      int pos = iso_time.indexOf("T") ;
      String raw = iso_time.substring(0, pos ); //getting just date data

      pos = raw.indexOf("-") ; //getting position of the -
      String year = raw.substring(0,pos);

      raw = raw.substring( pos + 1 , raw.length() ); //removing year info in new raw string
      pos = raw.indexOf("-") ;
      String month = raw.substring(0,pos);
      String day = raw.substring( pos + 1 );

      //getting time 
      iso_time = param[1].asStr();
      raw = iso_time.substring(iso_time.indexOf("T") + 1, iso_time.indexOf("+") ); //getting time
      String hh = raw.substring(0,2);
      String mm = raw.substring(5,3);

      int hr_24  ;
      int hr_12 ;
      String AMPM ;

      hr_24 = hh.toInt();
      if (hr_24==0) hh=12;
      else hh=hr_24%12;
      if (hr_24<12) AMPM = "AM";
      else AMPM = "PM";

      txt = hh + ":" + mm + AMPM;
      year = year.substring(2,4);
      timeStamp = day + "-" + getMonthStr(month.toInt()) + "-" + year + " [" + txt + "]";
      //timeStamp = day + "-" + getMonthStr(month.toInt()) + " [" + txt + "]";
    }
}

void setup()
{
  Serial.begin(115200);
  delay(100);
  BlynkEdgent.begin();

  pinMode(motorRelay, OUTPUT) ;
  digitalWrite(motorRelay , LOW) ;

  Blynk.virtualWrite(V1, 0 );  
  Blynk.virtualWrite(V2, 0 );  
  Blynk.virtualWrite(V5,  "" );
  Blynk.sendInternal("utc", "iso");       // ISO-8601 formatted time

  //SV DEFAULT VALUES
  for (int i = 0; i < buff_size_sv; i++) {
    arr_sv[i] = 1000;
  }
}

void loop() {
  BlynkEdgent.run();
 
  sensorValue = get_avg_sv(analogRead(sensorPin));
 
 //if TEMP RUN WE DONT RUN ANYTHING ELSE************
 if (!tempRun || forceStart){ //IF TEMP RUN IS FALSE WE NEED TO RUN THE MOTOR TEMP FOR X MIN

  if (sensorValue < sensorLimit & !water_detected){
    water_detected = true ;
  }

  if (!motorFlag) {
    digitalWrite(motorRelay , HIGH) ;//MOTOR ON
    motorFlag = true; //temp MOTOR ON
    startMilliSec = millis() ;
  }
  else{
    if ( (millis()  -  startMilliSec) >= tempRunTimeout ){
      tempRun  = true ; //FIRST RUN DONE
    }
  }
 }
 //IF TEMP RUN IS NOT RUNNING**********************
 else{
  //IF WATER IS COMING AND MOTOR IS NOT ON , THEN SWITCH ON
  if (sensorValue < sensorLimit && !motorFlag){
    digitalWrite(motorRelay, HIGH); // START MOTOR 2
    motorFlag = true ;
    water_detected = true ;
    motorStartTimeMillis = millis() ;
  }

  //IF MOTOR IS ON AND THERE IS NO WATER ENTER NO WATER WAIT
  //IF FORCE START IS ON WE WILL NOT STOP MOTOR
  else if (sensorValue > sensorLimit && motorFlag & !forceStart) {
    //STOP MOTOR IMMIDIATELY IF NO WATER WAS DETECTED
    if (!water_detected){
      digitalWrite(motorRelay, LOW); //STOP MOTOR 2
      motorFlag = false;
      nowater_flag = false;
      txt = "M2:NO WATER DTD SV2:" + String(sensorValue) ;
      printV5(txt);
    }
    else{
      if (nowater_flag == false){
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
      printV5("M1:NO WATER " + txt + " SV1:" + String(sensorValue) );
      //Blynk.virtualWrite(V5,  "M1:NO WATER " + txt + " SV1:" + String(sensorValue) );
      delay(1000) ;

      if (millis() > timeOverMill){
        sensorValue = get_avg_sv(analogRead(sensorPin));
        if (sensorValue > sensorLimit) {
          digitalWrite(motorRelay, LOW); //STOP MOTOR 2
          motorFlag = false;
          nowater_flag = false;
          motorEndTimeMillis = millis() ;
          ranFor = getMillisummery();
          delay(2000);
        }
      }
    }
  }
  //reset no water
  else if ((sensorValue < sensorLimit && motorFlag && nowater_flag == true ) ){
    nowater_flag = false;
  }
 }
  
  Blynk.beginGroup();
  Blynk.virtualWrite(V2, sensorValue); //UPDATING M1 SENSOR VALUE
  if (motorFlag){ 
    Blynk.virtualWrite(V1, 1); 
    txt = "M1:ON SL1:" + String(sensorValue) + " [LMT:" + String(sensorLimit) + "]" ;
  } else{
    Blynk.virtualWrite(V1, 0); 
    txt = "M1:OFF SL1:" + String(sensorValue) + " [LMT:" + String(sensorLimit) + "]" ;
    //motor alreadyran
    if (ranFor.length()> 0){
      txt = ranFor ;
    }
  }  //UPDATING BLYNK VALUE

  if (!tempRun){
    txt.replace("M1:", "M1:T>");
  }

  if (!nowater_flag){
    printV5(txt);
    //Blynk.virtualWrite(V5,  txt );
  }
  
  Blynk.endGroup();

  delay(2000); //5 Sec Wait
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
  //return "M1 RAN FOR:" + padZero(hours) + ":" + padZero(minutes) + ":" + padZero(seconds)   ;
  return  timeStamp +  " M1 RAN:" + padZero(hours) + ":" + padZero(minutes) + ":" + padZero(seconds)   ;
}

/* we are taking last x values of the SV value to see the actual status of SV */
int get_avg_sv(int val_sv)
{
  long total = 0;
  byte count = 0;
  static byte i_sv = 0;
 
  arr_sv[i_sv++] = val_sv;
  if (i_sv == buff_size_sv){ i_sv = 0;}
  
  while (count < buff_size_sv)
  {
    total = total + arr_sv[i_sv++];
    if (i_sv == buff_size_sv){i_sv = 0;}
    count++;
  }
  return total/buff_size_sv;
}

String padZero(int val){
  if (val < 10){    return "0" + String(val);  }
  else{  return String(val);  }
}
 
String getMonthStr(int m){
  String arr[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
  return arr[m-1];
}

void printV5(String txt){
  Blynk.virtualWrite(V5,  txt ); //INFO
  Serial.println(txt);
  delay(2000);
}
