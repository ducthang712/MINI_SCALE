#include <Arduino.h>
#include "HX711.h"
#include <EEPROM.h>
#include <TimerOne.h>

//------------ADRUINO LGT8F328 - PINOUT----------------
#define   LED7SEG_DIO_PIN     2   
#define   LED7SEG_SCLK_PIN    4
#define   LED7SEG_RCLK_PIN    3

#define   HX711_DATA_PIN      5   
#define   HX711_SCK_PIN       6

#define   BUT_UP_PIN          9   
#define   BUT_DOWN_PIN        7
#define   BUT_FUNCTION_PIN    8

#define   TOTAL_CP            10         //CP = "CALIB POINT"
#define   NUMBER_OF_LED7SEG   4

#define   led_dot      0x7F         //LED7SEG value for a dot "."

#define   FCNT_TIMER1       0

//---------------------SETUP FOR MENU----------------------
enum MENU{
  MEASURING_LOADCELL =0,
  CHOOSING_CALIB_POINT =1,
  CONFIG_CALIB_POINT_VALUE =2,
};
enum MENU PAGE;

enum BUTTON_FUNCTION {   //.... = FCNT from Timer1
  FUNC_CLICK = 30,
  FUNC_HOLD = 3000,
  UP_CLICK = 30,
  UP_HOLDx1 = 1000,
  UP_HOLDx2 = 3000,
  UP_HOLDx3 = 6000,
  DOWN_CLICK = 30,
  DOWN_HOLDx1 = 1000,
  DOWN_HOLDx2 = 3000,
  DOWN_HOLDx3 = 6000,
};

enum EEPROM_DATABASE{
  EEPROM_CP_VALUE_SIZE    =  2,       
  EEPROM_CP_HX711_SIZE    =  4,
  EEPROM_BEGIN_CP_VALUE   =  0,
  EEPROM_BEGIN_CP_HX711   =  20,
  EEPROM_BEGIN_VERIFY_CP= 100,
};
//--------------------SETUP FOR LED7SEG-----------------------
unsigned char LED_NUMBER_CODE[10] = {0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90};
uint8_t display7seg_arr[4];

//-------------------SETUP FOR CALIBRATION-----------------------
int16_t CP_value_arr[TOTAL_CP];
long CP_hx711_arr[TOTAL_CP];

int16_t temp_CP_value_arr[TOTAL_CP];
long temp_CP_hx711_arr[TOTAL_CP];
HX711 scale;
//------------------SETUP FOR HX711--------------------------


void setup() {
   Serial.begin(115200);
  //SET UP TIMER FOR DISPLAY PER 1ms
  Timer1.initialize(1000);     // *(10^-3) ms
  Timer1.attachInterrupt(DISPLAY_LED7SEG);
  //SETUP LOADCELL
  scale.begin(HX711_DATA_PIN, HX711_SCK_PIN);
  //SETUP LED
  pinMode(LED7SEG_DIO_PIN, OUTPUT);
  pinMode(LED7SEG_SCLK_PIN, OUTPUT);
  pinMode(LED7SEG_RCLK_PIN, OUTPUT);
  //SETUP BUTTON 
  pinMode(BUT_UP_PIN, INPUT_PULLUP);
  pinMode(BUT_DOWN_PIN, INPUT_PULLUP);
  pinMode(BUT_FUNCTION_PIN, INPUT_PULLUP); 

}

void loop() {
  switch(PAGE) {
    case MEASURING_LOADCELL:{
      FUNCTION_CALC_LOADCELL_DATA();
      break;
    }

    case CHOOSING_CALIB_POINT:{
      FUNCTION_CHOOSING_CALIB_POINT();
      break;
    }

    case CONFIG_CALIB_POINT_VALUE:{
      FUNCTION_CONFIG_CALIB_POINT_VALUE();
      break;
    }
  }
}
//--------------------MENU PAGE-----------------------
void FUNCTION_CALC_LOADCELL_DATA() {
  while(1) {
    //do something
    if( FCNT_TIMER1 != 0) break;
  }
}

void FUNCTION_CHOOSING_CALIB_POINT() {
  while(1) {
    // do something
    if (FCNT_TIMER1 != 0) break;
  }
}

void FUNCTION_CONFIG_CALIB_POINT_VALUE() {
  while(1) {
    // do something
    if (FCNT_TIMER1 != 0) break;
  }  
}
//---------------------CHECK BUTTON EVENT FUNCTION-----------------------
uint16_t CHECK_BUTTON_EVENT() {

}
//-------------------------DISPLAY_LED7SEG---------------------------
void DISPLAY_LED7SEG() {
  DISPLAY_LED();
}

void DISPLAY_UPDATE(char str[], char value_to_display[]) {
  uint16_t _temp = value_to_display;
  if (strcmp(str, "number") == 0) {
    display7seg_arr[1] = LED_NUMBER_CODE[(_temp/100) %10] & led_dot;
    display7seg_arr[2] = LED_NUMBER_CODE[(_temp / 10) %10];
    display7seg_arr[3] = LED_NUMBER_CODE[_temp %10];
    if (_temp <= 999) display7seg_arr[0] = 0xFF;
    else display7seg_arr[0] = LED_NUMBER_CODE[_temp/1000];
  }
  else if (strcmp(str, "string") == 0) { 
    display7seg_arr[0] = STRING_CODE(value_to_display[0]);
    display7seg_arr[1] = STRING_CODE(value_to_display[1]);
    display7seg_arr[2] = STRING_CODE(value_to_display[2]);
    display7seg_arr[3] = STRING_CODE(value_to_display[3]); 
  }
  else if (strcmp(str,"n=") == 0) {
    display7seg_arr[0] = STRING_CODE(str[0]);
    display7seg_arr[1] = STRING_CODE(str[1]);
    display7seg_arr[2] = 0xFF;
    if ( CP_value_arr[_temp] != -1) display7seg_arr[3] = LED_NUMBER_CODE[_temp] & led_dot;
    else  display7seg_arr[3] = LED_NUMBER_CODE[_temp];  
  }
}
void DISPLAY_LED() {
    for (uint8_t _position =0; _position < 4; _position++) {
      switch(_position){
        case 0: { 
          digitalWrite(LED7SEG_RCLK_PIN, LOW);
          shiftOut(LED7SEG_DIO_PIN, LED7SEG_SCLK_PIN, MSBFIRST, display7seg_arr[0]);
          shiftOut(LED7SEG_DIO_PIN, LED7SEG_SCLK_PIN, MSBFIRST, 0x08);
          digitalWrite(LED7SEG_RCLK_PIN, HIGH);
        } break;

        case 1: { 
          digitalWrite(LED7SEG_RCLK_PIN, LOW);
          shiftOut(LED7SEG_DIO_PIN, LED7SEG_SCLK_PIN, MSBFIRST, display7seg_arr[1]);
          shiftOut(LED7SEG_DIO_PIN, LED7SEG_SCLK_PIN, MSBFIRST, 0x04);
          digitalWrite(LED7SEG_RCLK_PIN, HIGH);
        } break;

        case 2: { 
          digitalWrite(LED7SEG_RCLK_PIN, LOW);
          shiftOut(LED7SEG_DIO_PIN, LED7SEG_SCLK_PIN, MSBFIRST, display7seg_arr[2]);
          shiftOut(LED7SEG_DIO_PIN, LED7SEG_SCLK_PIN, MSBFIRST, 0x02);
          digitalWrite(LED7SEG_RCLK_PIN, HIGH);
        } break;

        case 3: { 
          digitalWrite(LED7SEG_RCLK_PIN, LOW);
          shiftOut(LED7SEG_DIO_PIN, LED7SEG_SCLK_PIN, MSBFIRST, display7seg_arr[3]);
          shiftOut(LED7SEG_DIO_PIN, LED7SEG_SCLK_PIN, MSBFIRST, 0x01);
          digitalWrite(LED7SEG_RCLK_PIN, HIGH);
        } break;
      }
    }
}
char STRING_CODE(char alphabet) {
  switch(alphabet){
    case 'n':{
      return 0xAB;
    }break;
    case 'l':{
      return 0xC7;
    }break;
    case 'u':{
      return 0xE3;
    }break;
    case 'o':{
      return 0xA3;
    }break;
    case 'd':{
      return 0xA1;
    }break;
    case '=':{
      return 0xB7;
    }break;
    case 'e':{
      return 0x86;
    }break;
    case 'r':{
      return 0xAF;
    }
    case 'f':{
      return 0x8E;
    }break;
    case 'E':{
      return 0x86;
    }break;
    case 'F':{
      return 0x8E;
    }break;
    case 'c':{
      return 0xA7;
    }break;
    case ' ': {
      return 0xFF;
    }break;
    case '.': {
      return 0x7F; 
    }break;
    case '-': {
      return 0xBF;
    }
    case 'p': {
      return 0x8C;
    }break;
    case 'P': {
      return 0x8C;
    }break;
    case 's':{
      return 0x92;
    }break;
    case 'S': {
      return 0x92;
    }break;
    case 'B': {
      return 0x7F;
    }break;
    case 'b': {
      return 0x83;
    }break;
    case 'A':{
      return 0x88;
    } break;
    case 'a': {
      return 0x88;
    }break;
  }
}

//--------------------HOW TO MEASURING DATA FROM LOADCELL--------------
float alpha(uint8_t CP_number) {
  float x = temp_CP_value_arr[CP_number + 1] - temp_CP_value_arr[CP_number];
  float y = (temp_CP_hx711_arr[CP_number + 1] - temp_CP_hx711_arr[CP_number]);
  return x/y;
}

uint16_t MEASURING (long hx711_value) { 
  uint8_t _total_point =0; 
  _total_point = COUNTING_CP() - 1;
  for( uint8_t _index = 0; _index < _total_point; _index++) {
    if (hx711_value <= temp_CP_hx711_arr[0]) 
      return 0;
    else if( hx711_value >= temp_CP_hx711_arr[_total_point])
      return temp_CP_value_arr[_total_point];
    else if ((hx711_value >= temp_CP_hx711_arr[_index]) && (hx711_value < temp_CP_hx711_arr[_index + 1])) {
      return temp_CP_value_arr[_index] + (alpha(_index) * (hx711_value - temp_CP_hx711_arr[_index]));
    } 
  }
}
uint8_t COUNTING_CP() {
  static uint8_t _count =0;
  for(uint8_t _index =0; _index <10; _index++) {
    if ( CP_value_arr[_index] == -1) continue;
    else {  
      EEPROM.get(EEPROM_BEGIN_CP_VALUE + (_index * EEPROM_CP_VALUE_SIZE), temp_CP_value_arr[_count]);            
      EEPROM.get(EEPROM_BEGIN_CP_HX711 + (_index * EEPROM_CP_HX711_SIZE), temp_CP_hx711_arr[_count]);   
      _count++;
    }
  }
  return _count;
}