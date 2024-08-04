//Designed and developed by Deepak Lohia at www.dlohia.com
#include <Wire.h>
#include <LiquidCrystal.h>
#define ON LOW
#define OFF HIGH

const int buff_size_sv = 5; //taking last 10 values of the SV to determine if there is water
long arr_sv[buff_size_sv];

String verDate = "17-06-2024";
const int motorRelay = 7; 
const int sensorLimitPin = A5; 
const int sensorPin = A1; 
int sensorValue ;
int sensorLimit = 850 ;

bool motorFlag;
bool firstRunFlag ;
bool testRunFlag;
bool noWaterFlag;
String txt;
String ranFor;
String l1; //l1 is main motor status
String l2 ; //l2 is current status or log

String strMotarStatus;
unsigned long motorStartTimeMillis ;
unsigned long motorEndTimeMillis ;
unsigned long motorTotalMillis ;
unsigned long lastMotorTotalMillis ;

unsigned long testRunFlagStartMilli;
unsigned long testRunFlagTimeOut = 90UL * 1000UL ; // 90 secs ***** in seconds
unsigned long testRunFlagInterval = 30UL * 60UL * 1000UL ; // 30 min *****in minutes
unsigned long nexttestRun;

unsigned long noWaterStartMilli;
unsigned long noWaterTotalMilli;
unsigned long noWaterTimeOut  = 25UL * 1000UL ; // 25 mili seconds **** HOW MANY SECONDS TO WAIT BEFORE MOTOR SWITCH OFF
unsigned long tme ;
unsigned long seconds ;
unsigned long minutes ;
unsigned long hours ;

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

void setup() {
  Serial.begin(9600);

  pinMode(motorRelay, OUTPUT);
  digitalWrite(motorRelay, OFF);  

  lcd.begin(16,2);   // initialize the lcd for 16 chars 2 lines, turn on backlight
  lcd.clear();
  lcd.setCursor(0,0);
  delay(1000);
 
  l1 = "JAI GURU DEV" ;
  l2 = "JAI SHIV SHANKAR" ;
  printLCD()  ;

  l1 = "Ver Date: "  ;
  l2 = verDate ;
  printLCD()  ;
  
  l1 = "Motor Intrvl:"  ;  
  l2 = String((testRunFlagInterval / 1000 ) / 60 ) + " MIN" ;
  printLCD()  ;

  delay(1000);
 
  //SV DEFAULT VALUES
  for (int i = 0; i < buff_size_sv; i++) {
    arr_sv[i] = 1000;
  }
  testRunFlag = true;
}

void loop() {
 
  sensorLimit = getSV_Lmt() ;
  sensorValue = get_avg_sv(analogRead(sensorPin));

  if ((millis() >= nexttestRun) && !testRunFlag && !motorFlag) {
    testRunFlag = true;
   }

  //AUTO DETECT WATER
  if (sensorValue < sensorLimit){
    if (!motorFlag){
      digitalWrite(motorRelay, ON); //START MOTOR 1
      motorFlag = true ; 
      l2 ="MTR:WTR DETD";
      testRunFlag = false;
      motorStartTimeMillis = millis() ;}
    else{
      tme = millis()  -  motorStartTimeMillis ;
      seconds = tme / 1000;
      minutes = seconds / 60;
      hours = minutes / 60;
      tme %= 1000;
      seconds %= 60;
      minutes %= 60;
      hours %= 24;
      l2 =">" + padZero(hours) + ":" + padZero(minutes) + ":" + padZero(seconds);
    }
    //rest no water flag
    if (noWaterFlag){noWaterFlag = false;}
  }
  //TEST RUN MOTOR
  else if (testRunFlag) {
    if (!motorFlag){
      digitalWrite(motorRelay, ON); //START MOTOR 1
      motorFlag = true;
      testRunFlagStartMilli = millis();
    }
    //if it was a test run then stop motor after test run
    else if (motorFlag && (millis() >= (testRunFlagStartMilli + testRunFlagTimeOut))){
      digitalWrite(motorRelay, OFF); //STOP MOTOR 2
      motorFlag = false;
      testRunFlag = false;
      l2 = "NO WTR DTD";      
      printLCD();
      delay(2000);
    
      testRunFlag = false ;
      nexttestRun = millis() + testRunFlagInterval ;
      l2 = "NXT RUN IN " +  String((testRunFlagInterval / 1000 ) / 60 ) + "M" ;
      printLCD();  
      delay(2000);
    }
    //TESTING MOTOR TIME
    else{
      tme = testRunFlagTimeOut - (millis()  -  testRunFlagStartMilli) ;
      seconds = tme / 1000;
      minutes = seconds / 60;
      hours = minutes / 60;
      tme %= 1000;
      seconds %= 60;
      minutes %= 60;
      hours %= 24;

      l2 ="TSTNG: " + padZero(hours) + ":" + padZero(minutes) + ":" + padZero(seconds);
    }
  }

  //DISPLAY NEXT RUN COUNTDOWN TIME
  else if(!testRunFlag && !motorFlag){

    tme = nexttestRun - millis()  ;
    seconds = tme / 1000;
    minutes = seconds / 60;
    hours = minutes / 60;

    tme %= 1000;
    seconds %= 60;
    minutes %= 60;
    hours %= 24;
  
    l2 ="RUN IN:" + padZero(hours) + ":" + padZero(minutes) + ":" + padZero(seconds);
  }

  //WATER WAIT >> NO WATER WAITING WHEN testRunFlag IS TRUE
  else if (sensorValue > sensorLimit && motorFlag && !testRunFlag){
    sensorValue =get_avg_sv(analogRead(sensorPin));
    //if we are NOT doing test run the we make motor wait before off
    if (!testRunFlag){
      if (noWaterFlag == false){
        noWaterFlag = true ;
        noWaterStartMilli = millis() ;
        noWaterTotalMilli =  noWaterStartMilli + noWaterTimeOut ;
      }

      tme = noWaterTimeOut -  (millis()  -  noWaterStartMilli) ;   
      seconds = tme / 1000;
      minutes = seconds / 60;
      hours = minutes / 60;
      //days = hours / 24;

      tme %= 1000;
      seconds %= 60;
      minutes %= 60;
      hours %= 24;
  
      txt  = padZero(hours) + ":" + padZero(minutes) + ":" + padZero(seconds);
      l2 = "NO WTR: " + txt + " SV:" + String(sensorValue) ;
      printLCD();

      //if it was a test run then stop motor after test run
      if (millis() > noWaterTotalMilli){
        sensorValue = get_avg_sv(analogRead(sensorPin));
        if (sensorValue > sensorLimit) {
          digitalWrite(motorRelay, OFF); //STOP MOTOR 2
          motorFlag = false;
          noWaterFlag = false; //reset no water
          createRunSummery();
          l2 = "NO WTR OFF";
          printLCD();
        }
      }
    }
    //IF TEST RUN IS ON STOP IMMIDIATELY
    else if((sensorValue > sensorLimit) && motorFlag){
      digitalWrite(motorRelay, OFF); //STOP MOTOR 2
      motorFlag = false;
      testRunFlag = false;
      l2 = "NO WTR DTD";
    }
  }

  if (motorFlag){ strMotarStatus = "ON"; }else{ strMotarStatus = "OFF"; }
  l1 = ">" + strMotarStatus + " V" + String(sensorValue) + " L" + String(sensorLimit)  ;
  
  //motor alreadyran
  if (ranFor.length()> 0){
    l2 = ranFor ;
  }


  printLCD();
  delay(1000);
}
 
void printSerial(){
  Serial.println(l1);
  Serial.println(l2);
}

void printLCD(){
  l1 = l1.substring(0, 16) ;
  l2 = l2.substring(0, 16) ;
  
  printSerial();
  lcd.clear();
  lcd.setCursor(0,0); //Start at character 0 on line 0
  lcd.print(l1);
  lcd.setCursor(0,1);
  lcd.print(l2); 
  delay(1000) ;
}

String padZero(int val){
  if (val < 10){ return "0" + String(val);}  else{ return String(val);}
}

int getSV_Lmt(){
  return map(analogRead(sensorLimitPin), 0, 1023, 500, 950); // FROM LOW TO HIGH
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

void createRunSummery(){
  motorEndTimeMillis = millis() ;
  motorTotalMillis = motorEndTimeMillis - motorStartTimeMillis;
  if (lastMotorTotalMillis > 0){ motorTotalMillis += lastMotorTotalMillis ;}
  lastMotorTotalMillis = motorTotalMillis;

  tme = motorTotalMillis;
  seconds = tme / 1000;
  minutes = seconds / 60;
  hours = minutes / 60;

  tme %= 1000;
  seconds %= 60;
  minutes %= 60;
  hours %= 24;
  ranFor =  "RAN FOR:" + padZero(hours) + ":" + padZero(minutes) + ":" + padZero(seconds)   ;
}
