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

//---------------------SETUP FOR MENU----------------------
enum MENU{
  PAGE_MEASURING_LOADCELL ,
  PAGE_CONFIG_CALIB_POINT_VALUE ,
}; enum MENU PAGE = PAGE_MEASURING_LOADCELL;

enum BUTTON_STATUS {   
  FUNC_CLICKED,
  FUNC_HOLD,
  UP_CLICKED,
  UP_HOLDx1,
  UP_HOLDx2,
  UP_HOLDx3,
  DOWN_CLICKED,
  DOWN_HOLDx1,
  DOWN_HOLDx2,
  DOWN_HOLDx3,
  RELEASE,
}; enum BUTTON_STATUS STATE = RELEASE;

enum WHICH_PRESSED {
  BUTTON_UP,
  BUTTON_DOWN,
  BUTTON_FUNCTION,
  BUTTON_RELEASED,
}; enum WHICH_PRESSED BUT_PRESSED = BUTTON_RELEASED;

enum HOW_PRESSED {   //ms
  CLICKED = 300,
  LONG_PRESS = 3000,
  DOUBLE_LONG_PRESS = 5000,
  TRIPPLE_LONG_PRESS = 7000,
};

enum EEPROM_DATABASE{
  EEPROM_CP_VALUE_SIZE    =  2,      //int16_t 
  EEPROM_CP_HX711_SIZE    =  4,      //long
  EEPROM_BEGIN_CP_VALUE   =  0,
  EEPROM_BEGIN_CP_HX711   =  20,
  EEPROM_BEGIN_VERIFY_CP  = 100,
};
//---------------------SETUP FOR LED7SEG-----------------------
unsigned char LED_NUMBER_CODE[10] = {0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90};
uint8_t display7seg_arr[4];

//-------------------SETUP FOR CALIBRATION-----------------------
int16_t CP_value_arr[TOTAL_CP];
long CP_hx711_arr[TOTAL_CP];

int16_t temp_CP_value_arr[TOTAL_CP];
long temp_CP_hx711_arr[TOTAL_CP];

HX711 scale;
//----------------------SETUP FOR HX711--------------------------


void setup() {
  Serial.begin(115200);
  //SET UP TIMER FOR DISPLAY PER 1ms
  Timer1.initialize(1000);     // *(10^-3) ms
  Timer1.attachInterrupt(INTERRUPT);
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
  //GET DATA FROM EEPROM
  GET_EEPROM_DATA();
}

void loop() {
  switch(PAGE) {
    case PAGE_MEASURING_LOADCELL:
      FUNCTION_MEASURING_LOADCELL();
      break;
    

    case PAGE_CONFIG_CALIB_POINT_VALUE:
      FUNCTION_CONFIG_CALIB_POINT_VALUE();
      break;   
  }
}
//-----------------INTERRUPT PER 1MS-----------------
void INTERRUPT() {
  static uint16_t _fps_counter = 0;
  _fps_counter++;
  CHECK_BUTTON_STATE();
  if((_fps_counter % 20) == 0) DISPLAY_LED();
}

//--------------------MENU PAGE-----------------------
void FUNCTION_MEASURING_LOADCELL() {
  while(BUT_PRESSED != BUTTON_FUNCTION) {
    DISPLAY_UPDATE("number", CALC_INPUT_DATA(scale.read()));
  }
  //NAVIGATION
  switch(STATE) {
    case FUNC_CLICKED:
      break;
    case FUNC_HOLD:
      DISPLAY_UPDATE("string","func");
      PAGE = PAGE_CONFIG_CALIB_POINT_VALUE;
      break;
  }
}

void FUNCTION_CONFIG_CALIB_POINT_VALUE() {
  static uint8_t _CP_position =0;
  while(BUT_PRESSED != BUTTON_FUNCTION) {
    DISPLAY_UPDATE("n=", _CP_position);
    switch(STATE){
      case UP_CLICKED:
        _CP_position++;
        if (_CP_position >= TOTAL_CP) _CP_position = TOTAL_CP;
        break;
      case DOWN_CLICKED:
        _CP_position--;
        if (_CP_position <=0 ) _CP_position = 0;
        break;
    }
  }
  //NAVIGATION
  switch(STATE) {
    case FUNC_CLICKED:
      CONFIGURATION_CP_VALUE(_CP_position); 
      break;
    case FUNC_HOLD:
      DISPLAY_UPDATE("string", "base");
      PAGE = PAGE_MEASURING_LOADCELL;
      break;
  }
  
}

void CONFIGURATION_CP_VALUE(uint8_t _CP_position) {
  int16_t _temp_CP_value = CP_value_arr[_CP_position];
  while(BUT_PRESSED != BUTTON_FUNCTION) {
    if (_temp_CP_value == -1) DISPLAY_UPDATE("string", "null");
    else DISPLAY_UPDATE("number", _temp_CP_value);
    switch(STATE) {
      case UP_CLICKED:
        _temp_CP_value++;
        break;
      case UP_HOLDx1:
        _temp_CP_value+= 2;
        break;
      case UP_HOLDx2:
        _temp_CP_value+=10;
        break;
      case UP_HOLDx3:
        _temp_CP_value+=50;
        break;
      case DOWN_CLICKED:
        _temp_CP_value--;
        break;
      case DOWN_HOLDx1:
        _temp_CP_value-= 2;
        break;
      case DOWN_HOLDx2:
        _temp_CP_value-= 10;
        break;
      case DOWN_HOLDx3:
        _temp_CP_value-= 50;
        break;
    }
    if(_temp_CP_value >= 9999) _temp_CP_value = 9999;
    else if(_temp_CP_value <= -1) _temp_CP_value = -1;
  }
  switch(STATE) {
    case FUNC_CLICKED:
      VERIFY_NEW_CALIB_UPDATE(_CP_position,_temp_CP_value);
      break;
    case FUNC_HOLD:
      PAGE = PAGE_CONFIG_CALIB_POINT_VALUE;
      break;
  }
}

void VERIFY_NEW_CALIB_UPDATE(uint8_t _CP_position,int16_t _temp_CP_value) {
  int16_t _temp_check;
  EEPROM.get(EEPROM_BEGIN_CP_VALUE + _CP_position * EEPROM_CP_VALUE_SIZE, _temp_check);
  if (_temp_check != _temp_CP_value) {    //NEW VALUE VERIFIED
    CP_value_arr[_CP_position] = _temp_CP_value;
    CP_hx711_arr[_CP_position] = scale.read_average(10);
    EEPROM.put(EEPROM_BEGIN_CP_VALUE + _CP_position * EEPROM_CP_VALUE_SIZE, CP_value_arr[_CP_position]);
    EEPROM.put(EEPROM_BEGIN_CP_HX711 + _CP_position * EEPROM_CP_HX711_SIZE, CP_hx711_arr[_CP_position]);
    DISPLAY_UPDATE("string", "done");
  }
}

//---------------------CHECK BUTTON EVENT FUNCTION-----------------------
void CHECK_BUTTON_STATE() {
  static uint16_t _counter =0;
  if ((digitalRead(BUT_DOWN_PIN) == 0) || (digitalRead(BUT_FUNCTION_PIN) == 0) || (digitalRead(BUT_UP_PIN) == 0)) {
    _counter++;
    BUT_PRESSED = CHECK_BUT_WHICH_PRESSED();
  }
  else {
    if ((_counter > 0) && (_counter <= CLICKED)) {
      STATE = CHECK_BUT_CLICKED();
    }
    else if (_counter == 0) {
      STATE = RELEASE;
    }
    _counter = 0;
    BUT_PRESSED == BUTTON_RELEASED;    
  }
  // if button still being pressed
  if (_counter >= LONG_PRESS) {
    STATE = CHECK_BUT_LONG_PRESS();
    //LONG_PRESS
    if (_counter >= DOUBLE_LONG_PRESS) {
      STATE = CHECK_BUT_DOUBLE_LONG_PRESS();
      //LONGER PRESSED
      if (_counter >= TRIPPLE_LONG_PRESS) {
        STATE = CHECK_BUT_TRIPPLE_LONG_PRESS();
      }
    }
  }
}

uint8_t CHECK_BUT_WHICH_PRESSED() {
  if (digitalRead(BUT_DOWN_PIN) == 0) {
    return BUTTON_DOWN;
  }
  if (digitalRead(BUT_FUNCTION_PIN) == 0) {
    return BUTTON_FUNCTION;
  }
  if (digitalRead(BUT_UP_PIN) == 0) {
    return BUTTON_UP;
  }
}

uint8_t CHECK_BUT_CLICKED() {
  switch(BUT_PRESSED) {
    case BUTTON_DOWN:
      return DOWN_CLICKED;
      break;
    case BUTTON_UP:
      return UP_CLICKED;
      break;
    case BUTTON_FUNCTION:
      return FUNC_CLICKED;
      break;
  }  
}

uint8_t CHECK_BUT_LONG_PRESS() {
  switch(BUT_PRESSED) {
    case BUTTON_FUNCTION:
      return FUNC_HOLD;
      break;
    case BUTTON_UP:
      return UP_HOLDx1;
      break;
    case BUTTON_DOWN:
      return DOWN_HOLDx1;
      break;
  }
}

uint8_t CHECK_BUT_DOUBLE_LONG_PRESS() {
  switch(BUT_PRESSED) {
    case BUTTON_UP:
      return UP_HOLDx2;
      break;
    case BUTTON_DOWN:
      return DOWN_HOLDx2;
      break;
  }
}

uint8_t CHECK_BUT_TRIPPLE_LONG_PRESS() {
  switch(BUT_PRESSED) {
    case BUTTON_UP:
      return UP_HOLDx3;
      break;
    case BUTTON_DOWN:
      return DOWN_HOLDx3;
      break;
  } 
}
//-------------------------DISPLAY_LED7SEG---------------------------
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

uint16_t CALC_INPUT_DATA (long hx711_value) { 
  uint8_t _total_point =0; 
  _total_point = COUNTING_CP();
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
  static uint8_t _count_CP =0;
  for(uint8_t _index =0; _index <10; _index++) {
    if ( CP_value_arr[_index] == -1) continue;
    else {  
      EEPROM.get(EEPROM_BEGIN_CP_VALUE + (_index * EEPROM_CP_VALUE_SIZE), temp_CP_value_arr[_count_CP]);            
      EEPROM.get(EEPROM_BEGIN_CP_HX711 + (_index * EEPROM_CP_HX711_SIZE), temp_CP_hx711_arr[_count_CP]);   
      _count_CP++;
    }
  }
  return (_count_CP - 1);
}
//-----------------------EEPROM------------------------
void GET_EEPROM_DATA() {
  for( uint8_t _index =0; _index < TOTAL_CP; _index++) {
    EEPROM.get(EEPROM_BEGIN_CP_VALUE + _index * EEPROM_CP_VALUE_SIZE, CP_value_arr[_index]);
    EEPROM.get(EEPROM_BEGIN_CP_HX711 + _index * EEPROM_CP_HX711_SIZE, CP_hx711_arr[_index]);
  }
}


