#include <Arduino.h>
#include "HX711.h"
#include <EEPROM.h>
#include <TimerOne.h>

//------------ADRUINO LGT8F328 - PINOUT----------------
#define   LED7SEG_DIO_PIN     8   
#define   LED7SEG_SCLK_PIN    7
#define   LED7SEG_RCLK_PIN    6

#define   HX711_DATA_PIN      10   
#define   HX711_SCK_PIN       11

#define   BUT_UP_PIN          4   
#define   BUT_DOWN_PIN        3
#define   BUT_FUNCTION_PIN    2

#define   TOTAL_CP            10         //CP = "CALIB POINT"
#define   NUMBER_OF_LED7SEG   4

#define   led_dot      0x7F         //LED7SEG value for a dot "."

#define   COMPLETED    1
#define   FAILED       0

#define   FPS_rate   10

//BUTTON DURATION
#define   clicked_drt                5
#define   pressed_drt               1000
#define   long_pressed_drt          3000
#define   double_long_pressed_drt   6000

//EEPROM DATABASE
#define  EEPROM_CP_VALUE_SIZE      2      //int16_t 
#define  EEPROM_CP_HX711_SIZE      4      //long
#define  EEPROM_BEGIN_CP_VALUE     0
#define  EEPROM_BEGIN_CP_HX711     20
#define  EEPROM_BEGIN_VERIFY_CP   100
//---------------------SETUP FOR MENU----------------------
enum MENU{
  PAGE_MEASURING_LOADCELL ,
  PAGE_CONFIG_CALIB_POINT_VALUE ,
  PAGE_CONFIG_ACTION,
}; enum MENU PAGE = PAGE_MEASURING_LOADCELL;

enum BUTTON_STATUS {   
  RELEASED,
  CLICKED,
  PRESSED,
  LONG_PRESSED,
  DOUBLE_LONG_PRESSED,
}; enum BUTTON_STATUS STATE = RELEASED;

enum WHICH_PRESSED {
  BUTTON_RELEASED,
  BUTTON_UP,
  BUTTON_DOWN,
  BUTTON_FUNCTION,
}; enum WHICH_PRESSED BUTTON = BUTTON_RELEASED;

//---------------------SETUP FOR LED7SEG-----------------------
unsigned char LED_NUMBER_CODE[10] = {0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90};
uint8_t display7seg_arr[4];

//-------------------SETUP FOR CALIBRATION-----------------------
int16_t default_CP_value_arr[TOTAL_CP] = { 0, -1, -1, -1, -1, -1 , -1, -1, -1, 9999};
long default_CP_hx711_arr[TOTAL_CP]= {-31578, 0, 0, 0, 0, 0, 0, 0, 0, 800000};

int16_t CP_value_arr[TOTAL_CP];
long CP_hx711_arr[TOTAL_CP];

int16_t temp_CP_value_arr[TOTAL_CP];
long temp_CP_hx711_arr[TOTAL_CP];

HX711 scale;
//----------------------SETUP FOR HX711--------------------------
uint16_t _fps_counter = 0;

//--------------------------------------------------------------

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
  //DATA FROM EEPROM
  //check_setup_value();
  //GET_EEPROM_DATA();
}

void loop() {
  switch(PAGE) {
    case PAGE_MEASURING_LOADCELL:
      loadcell_measuring();
      break;
  
    case PAGE_CONFIG_CALIB_POINT_VALUE:
      config_value_calib_point();
      break;   

  }
}
//-----------------INTERRUPT PER 1MS-----------------
void INTERRUPT() {
  _fps_counter++;
  check_button_action();
  if((_fps_counter % FPS_rate) == 0) {
    //Serial.println("DISPLAY_DONE");
    DISPLAY_LED();
  }
}

//--------------------MENU PAGE-----------------------
void loadcell_measuring() {
  //uint16_t _measure_value = CALC_INPUT_DATA(scale.read());
  static uint8_t abc = 0;
  DISPLAY_UPDATE("number", abc);
  Serial.println(abc);
  delay(1);
  //NAVIGATION
  if (BUTTON == BUTTON_FUNCTION) {
    switch(STATE) {
      case CLICKED:
        abc++;
        break;
      case PRESSED:
        DISPLAY_UPDATE("string","func");
        delay(2000);
        PAGE = PAGE_CONFIG_CALIB_POINT_VALUE;
        break;
    }
  }
}

void config_value_calib_point() {
  static uint8_t _CP_position =0;
  DISPLAY_UPDATE("n=", _CP_position);
  delay(1);
  //NAVIGATION
  switch(BUTTON) {
    case BUTTON_DOWN: 
      switch(STATE) {
        case CLICKED:
          _CP_position--;
          break;
      }
      break;
    case BUTTON_UP: 
      switch(STATE) {
        case CLICKED:
          _CP_position++;
          break;
      }
      break;
    case BUTTON_FUNCTION: 
      switch (STATE) {
        case CLICKED:
          configuartion_action(_CP_position);
          break;
        case PRESSED:
          DISPLAY_UPDATE("string", "base");
          delay(2000);
          PAGE = PAGE_MEASURING_LOADCELL;
          break;
      }
    break;
  }
  if (_CP_position <= 0) _CP_position = 0;
  else if (_CP_position >= TOTAL_CP) _CP_position = TOTAL_CP - 1;
}

void configuartion_action(uint8_t _CP_position) {
  delay(100);
  static int16_t _temp_CP_value = CP_value_arr[_CP_position];
  uint8_t _done = 0;
  while(_done == 0){
    if (_temp_CP_value == -1) DISPLAY_UPDATE("string", "null");
    else DISPLAY_UPDATE("number", _temp_CP_value);
    delay(1);
    switch (BUTTON) {
    //COUNT UP
    case BUTTON_UP: 
      switch(STATE) {
        case CLICKED:
          _temp_CP_value++;
          break;
        case PRESSED:
          _temp_CP_value+=2;
          delay(20);
          break;
        case LONG_PRESSED:
          _temp_CP_value+=5;
          delay(30);
          break;
        case DOUBLE_LONG_PRESSED:
          _temp_CP_value+=50;
          delay(30);
          break;
      }
      //check for maxx value
      if(_temp_CP_value >= 9999) _temp_CP_value = 9999;
      break;
    //COUNT_DOWN
    case BUTTON_DOWN:
      switch(STATE) {
        case CLICKED:
          _temp_CP_value--;
          delay(30);
          break;
        case PRESSED:
          _temp_CP_value-=2;
          delay(30);
          break;
        case LONG_PRESSED:
          _temp_CP_value-=5;
          delay(30);
          break;
        case DOUBLE_LONG_PRESSED:
          _temp_CP_value-=50;
          delay(30);
          break;
      }
      //check for min value
      if (_temp_CP_value <= -1) _temp_CP_value = -1;
     break;
    //VERIFY or HOME
    case BUTTON_FUNCTION:
      switch (STATE) {
        case CLICKED:
          VERIFY_NEW_CALIB_UPDATE(_CP_position, _temp_CP_value);
          _done = 1;
          break;
        case PRESSED:
          _done = 1;
          break;
      }
      break;
    }
    if (_temp_CP_value >= 9999) _temp_CP_value = 9999;
    else if (_temp_CP_value <= -1) _temp_CP_value = -1;
  } 
}

void VERIFY_NEW_CALIB_UPDATE(uint8_t _CP_position,int16_t _temp_CP_value) {
  int16_t _temp_check;
  EEPROM.get(EEPROM_BEGIN_CP_VALUE + _CP_position * EEPROM_CP_VALUE_SIZE, _temp_check);
  if (_temp_check != _temp_CP_value) {    //NEW VALUE VERIFIED
    CP_value_arr[_CP_position] = _temp_CP_value;
    CP_hx711_arr[_CP_position] = scale.read_average(10);
    //EEPROM.put(EEPROM_BEGIN_CP_VALUE + _CP_position * EEPROM_CP_VALUE_SIZE, CP_value_arr[_CP_position]);
    //EEPROM.put(EEPROM_BEGIN_CP_HX711 + _CP_position * EEPROM_CP_HX711_SIZE, CP_hx711_arr[_CP_position]);
    DISPLAY_UPDATE("string", "done");
    delay(2000);
  }
}

//---------------------CHECK BUTTON EVENT FUNCTION-----------------------
// void button_action_page_1() {
//   static uint32_t _counter =0;
//   if (digitalRead(BUT_FUNCTION_PIN)== 0) {
//     _counter++;
//     BUTTON = BUTTON_FUNCTION;
//     if ((_counter >= pressed_drt)) {
//       // Serial.println("button func pressed");
//       STATE = PRESSED;
//     } 
//   }
//   else {
//     if ((_counter >= clicked_drt) && (_counter < pressed_drt)) {
//       // Serial.println("button func clicked");
//       STATE = CLICKED;
//       _counter =0;
//     }
//     else {
//       STATE = RELEASED;
//       BUTTON = BUTTON_RELEASED;
//       _counter = 0;
//     }
//   }
// }

void check_button_action() {
  static uint32_t _counter = 0;
  if(digitalRead(BUT_UP_PIN) == 0) {
    _counter++;
    BUTTON = BUTTON_UP;
    if ((_counter >= pressed_drt) && (_counter < long_pressed_drt)) {
      STATE = PRESSED;
    }
    else if ((_counter >= long_pressed_drt) && (_counter < double_long_pressed_drt)) {
      STATE = LONG_PRESSED;
    }
    else if ((_counter >= double_long_pressed_drt)) {
      STATE = DOUBLE_LONG_PRESSED;
    }
  }

  else if(digitalRead(BUT_DOWN_PIN) == 0) {
    _counter++;
    BUTTON = BUTTON_DOWN;
    if ((_counter >= pressed_drt) && (_counter < long_pressed_drt)) {
      STATE = PRESSED;
    }
    else if ((_counter >= long_pressed_drt) && (_counter < double_long_pressed_drt)) {
      STATE = LONG_PRESSED;
    }
    else if ((_counter >= double_long_pressed_drt)) {
      STATE = DOUBLE_LONG_PRESSED;
    }
  }

  else if(digitalRead(BUT_FUNCTION_PIN) == 0) {
    _counter++;
    BUTTON = BUTTON_FUNCTION;
    if ( _counter >= pressed_drt) STATE = PRESSED;
  }

  else {
    if (_counter > 0 ) {
      STATE = CLICKED;
      _counter =0;
    }
    else {
      STATE = RELEASED;
      BUTTON = BUTTON_RELEASED;
    }
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
    if( CP_value_arr[_temp] != -1)  display7seg_arr[3] = LED_NUMBER_CODE[_temp] & led_dot; 
    else if ( CP_value_arr[_temp] == -1) display7seg_arr[3] = LED_NUMBER_CODE[_temp];
  }
}

void DISPLAY_LED() {
  for (uint8_t _index = 0; _index < 20; _index++){
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
//----------------------CHECK CONNECT HX711---------------------------
void CHECK_CONNECT_HX711() {
  static uint16_t _hx711_check = 0;
  if(digitalRead(HX711_DATA_PIN) == 0) {
    _hx711_check++;
    if(_hx711_check >= 3000) { 
      DISPLAY_UPDATE("string", "eror");
    }
  } 
  else _hx711_check = 0;
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
//--------------------INIT VALUE----------------------
void check_setup_value() {
  uint8_t _run_default =0;
  uint8_t _count_null = 0;
  for (uint8_t _index =0; _index < TOTAL_CP;_index++) {
    if(EEPROM.read(_index * EEPROM_CP_VALUE_SIZE) != -1) _count_null++;
  }
  if(_count_null >0) _run_default = 1;
  else _run_default = 0;
  get_data(_run_default);
}

void get_data(uint8_t _run_default) {
  if (_run_default == 1) {
    for (uint8_t _index =0; _index < TOTAL_CP; _index++) {
      CP_value_arr[_index] = default_CP_value_arr[_index];
      CP_hx711_arr[_index] = default_CP_hx711_arr[_index];
    }
  }
  else if (_run_default == 0) {
    for (uint8_t _index =0; _index < TOTAL_CP; _index++) {
      EEPROM.get(EEPROM_BEGIN_CP_VALUE + _index * EEPROM_CP_VALUE_SIZE, CP_value_arr[_index]);
      EEPROM.get(EEPROM_BEGIN_CP_HX711 + _index * EEPROM_CP_HX711_SIZE, CP_hx711_arr[_index]);
    }    
  }
}




