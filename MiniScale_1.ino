#include <Arduino.h>
#include "HX711.h"
#include <EEPROM.h>
#include <TimerOne.h>

//--------ADRUINO NANO-------------
#define   DIO     8
#define   SCLK    7
#define   RCLK    6

#define   DATA    10
#define   SCK     11

#define   FUNC    2
#define   UP      4
#define   DOWN    3

//----------SETUP MENU VARIABLES--------------
const int LOADCELL_DOUT_PIN = DATA;
const int LOADCELL_SCK_PIN = SCK;
unsigned char LED_NUM[10] = {0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90};
uint8_t menu= 0;
uint8_t temp[4];    //variable for display led 
HX711 scale;
//----------SETUP FOR CALIBRATION-------------
int8_t n_calib = 0;
int16_t n[10] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};   //scale configuration
long n_read[10];        //save value read by hx711 

int16_t temp_n[10];    //save n[] except value -1
long temp_n_read[10];   //save save_read[] base on n[]
//---------------------------------
uint16_t value_temp =0;


void setup() {
  Serial.println("Data: %d", 200);
  // put your setup code here, to run once:
  Serial.begin(115200);
  //SET UP TIMER FOR DISPLAY PER 1ms
  Timer1.initialize(1000);     // *(10^-3) ms
  Timer1.attachInterrupt(Interrupt);
  //SETUP LOADCELL
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  //SETUP LED
  pinMode(DIO, OUTPUT);
  pinMode(SCLK, OUTPUT);
  pinMode(RCLK, OUTPUT);
  //SETUP BUTTON 
  pinMode(FUNC, INPUT_PULLUP);
  pinMode(UP, INPUT_PULLUP);
  pinMode(DOWN, INPUT_PULLUP); 
  // EEPROM
  // Serial.println("RESET");
  // reset_EEPROM(-1);
  // Serial.println("BEGIN");
  for(uint8_t i = 0; i <9;i++) {
    EEPROM.get(i*2,n[i]);
    Serial.println(n[i]);
  } Serial.println("BEGIN DONE");
}

void loop() {

  // check_error();
  switch(menu){
    //update value
    case 0: {
      n_calib = 0;
      update_EEPROM();
      menu = 1;
      value_temp = measure(scale.read());
    } break;
    
    //measuring
    case 1: {
      display_change("number",measure(scale.read()));
      if ( value_temp != measure(scale.read())) {
        Serial.println(measure(scale.read()));
        Serial.println(scale.read());
        Serial.println("DONE");
      }
      value_temp = measure(scale.read());
      FUNCTION(0);
    } break;

    //calib function
    case 3: {
      display_change("n=",n_calib);
      // n_calib setting up
      if (check_page() == 1) {
        n_calib++;
        if(n_calib >= 10)  n_calib = 9;
      }
      // n_calib setting down
      if (check_page() == 2) {
        n_calib--;
        if (n_calib < 0)  n_calib = 0;
      }
      Serial.println(n_calib);
      //ENTER - CANCEL
      FUNCTION(4);
    } break;

    //calib value
    case 4: {
      
      Serial.print(n[n_calib]);
      Serial.print("\t");
      Serial.println(scale.read());
      
      if (n[n_calib] == -1) display_change("string","null");
      else display_change("number", n[n_calib]);
      // n_calib setting up
        n[n_calib] = COUNT_UP(n[n_calib]); 
      // n_calib setting down
        n[n_calib] = COUNT_DOWN(n[n_calib]);
      //UPDATE EEPROM -CANCEL
      UPDATE(n_calib, n[n_calib]);
    } break;
  }
}

//----------------INTERRUPT TIMER1----------------
void Interrupt() {
  display_led();
}

//---------------HX711--------------------
void check_error() {
  while (digitalRead(DATA) == 1){   //when input DATA is high meanings HX711 is not ready for retrieval
    delay(100);
    while (digitalRead(DATA) == 1){
      display_change("string","eror");
    } break;
  }
}

//-------------LED 7 SEG------------------
void display_change(char str[], char value[]) {
  uint16_t x = value;
  if (strcmp(str, "number") == 0) {
    temp[1] = LED_NUM[(x/100) %10] & 0x7F;
    temp[2] = LED_NUM[(x / 10) %10];
    temp[3] = LED_NUM[x %10];
    if (x <= 999) temp[0] = 0xFF;
    else temp[0] = LED_NUM[x/1000];
  }
  else if (strcmp(str, "string") == 0) { 
    temp[0] = string_code(value[0]);
    temp[1] = string_code(value[1]);
    temp[2] = string_code(value[2]);
    temp[3] = string_code(value[3]); 
  }
  else if (strcmp(str,"n=") == 0) {
    temp[0] = string_code(str[0]);
    temp[1] = string_code(str[1]);
    temp[2] = 0xFF;
    if ( n[x] != -1) temp[3] = LED_NUM[x] & 0x7F;
    else  temp[3] = LED_NUM[x];  
  }
}
void display_led() {
  for (uint8_t i =0; i < 1; i++) {
    for (uint8_t pos =0; pos < 4; pos++) {
      switch(pos){
        case 0: { 
          digitalWrite(RCLK, LOW);
          shiftOut(DIO, SCLK, MSBFIRST, temp[0]);
          shiftOut(DIO, SCLK, MSBFIRST, 0x08);
          digitalWrite(RCLK, HIGH);
        } break;

        case 1: { 
          digitalWrite(RCLK, LOW);
          shiftOut(DIO, SCLK, MSBFIRST, temp[1]);
          shiftOut(DIO, SCLK, MSBFIRST, 0x04);
          digitalWrite(RCLK, HIGH);
        } break;

        case 2: { 
          digitalWrite(RCLK, LOW);
          shiftOut(DIO, SCLK, MSBFIRST, temp[2]);
          shiftOut(DIO, SCLK, MSBFIRST, 0x02);
          digitalWrite(RCLK, HIGH);
        } break;

        case 3: { 
          digitalWrite(RCLK, LOW);
          shiftOut(DIO, SCLK, MSBFIRST, temp[3]);
          shiftOut(DIO, SCLK, MSBFIRST, 0x01);
          digitalWrite(RCLK, HIGH);
        } break;
      }
    }
  }
}
char string_code(char alpha) {
  switch(alpha){
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

//-------------VALUE CHECK LIMIT------------------

int16_t check_null( int16_t value) {
  if ( value < 0) {
    return -1;
  } return value;
}

int16_t check_max( int16_t value) {
  if (value >= 9999) return 9999;
  else return value;
}

//---------------BUTTON-------------------
uint16_t COUNT_UP (uint16_t value) {
  static uint8_t fast = 0;
  while(digitalRead(UP)== LOW) {
    delay(100);
    if(digitalRead(UP)== LOW){
      value++;
      value = check_max(value);
      display_change("number", value);
      fast++;
      if(fast >= 20) {
        value+=5;
        value = check_max(value);
        display_change("number", value);
        if(fast >=50) {
          value+=30;
          value = check_max(value);
          display_change("number", value);
        }
      } 
    } 
  } fast =0;
  return value;
}

uint16_t COUNT_DOWN (uint16_t value) {
  static uint8_t fast = 0;
  while(digitalRead(DOWN)== LOW) {
    delay(100);
    if(digitalRead(DOWN)== LOW){
      value--;
      value = check_null(value);
      display_change("number", value);
      fast++;
      if(fast >= 20) {
        value-=5;
        value = check_null(value);
        display_change("number", value);
        if(fast >=50) {
          value-=30;
          value = check_null(value);
          display_change("number", value);
        }
      } 
    } 
  } fast =0;
  return value;
}

void FUNCTION (uint8_t move) {  // move = the page want to go into
  while (digitalRead(FUNC) == LOW) {
    delay(50);
    if (digitalRead(FUNC) == LOW) {
      display_change("string","----");
      delay(2000);
      //check button calib
      if (digitalRead(FUNC) == LOW) {
        //turn into main display
        if (menu == 3) {
          display_change("string","base");
          delay(2000);
          menu =0;
          break;
        }
        //turn into calibration
        if( menu != 3 ) {
          display_change("string","func");
          delay(2000);
          menu =3;
          break;
        }                
      }
      else menu = move;
      // chaging complete
    }
  }
}

void UPDATE ( uint8_t address, uint16_t value) {
  static uint16_t check;
  static long read_average;
  while (digitalRead(FUNC) == LOW) {
    delay(100);
    if (digitalRead(FUNC) == LOW) {
      display_change("string","----");
      delay(2000);
      //HOLD to CANCEL -> MAIN DISPLAY
      if(digitalRead(FUNC) == LOW){
        display_change("string","base");
        delay(2000);
        //resetFunc();
        menu = 0;
        break;
      }
      //CLICK to UPDATE -> FUNCTION
      else {
        EEPROM.get(address*2,check);
        if (check != value) {
          read_average = scale.read_average(10);
          display_change("string","done");
          EEPROM.put(address*2, value);
          EEPROM.get(address*2, n[address]);           //update new value of n[]
          EEPROM.put((20 + (address*4)),read_average);
          n_read[address] = read_average;
          Serial.print("average: \t");
          Serial.println(read_average);     //update new value of n_read[]
          delay(2000);
          update_EEPROM();
        }
        menu =3;
        break;
      }
    }
  }
}

uint8_t check_page() {
  if (digitalRead(UP) == LOW) {
    delay(100);
    return 1;
  }
  else if (digitalRead(DOWN) == LOW) {
    delay(80);
    return 2;
  }
  return 0;
}
//-------------------EEPROM---------------
void reset_EEPROM( int16_t value ) {
  static int16_t x =0;
  for (uint8_t i =0; i< 10; i++) {
    EEPROM.get(i*2,x);
    if ( x != value) EEPROM.put(i*2, value);
    EEPROM.get(i*2, n[i]);
    Serial.println(n[i]);
  }
}

void read_EEPROM() {
  for (uint8_t i=0; i<10; i++) {
    EEPROM.get(i, n[i]);
    Serial.println(n[i]);
  }
}

void update_EEPROM() {
  Serial.println("n[]:");
  for(uint8_t i = 0; i < 10; i++) {
    EEPROM.get(i*2, n[i]);
    Serial.println(n[i]);    //get data from EEPROM[0,2,4,...-> 18] then save to n[0->9]
  }
  Serial.println("n read:");
  for(uint8_t i = 0; i < 10; i++) {
    EEPROM.get((20 + (i*4)), n_read[i]);
    Serial.println(n_read[i]);    //get data from EEPROM[20,24,28,... -> 56] then save to n_read[0->9]
  }
  Serial.println("UPDATE DONE");
}
//-------------READ EEPROM to SAVE VALUE---------
int16_t get_n (uint8_t i) {    //save n[] (2bytes - 16bits): EEPROM[0,19]
  return EEPROM.read(i*2);
}
long get_n_read(uint8_t i) {   //save n_read[] (4bytes - 32bits) : EEPROM[20,19]
  return EEPROM.read(20+(i*4));
}

//--------------------RETURN VALUE / MEASURING-----------------
float alpha(uint8_t stt) {
  float x = temp_n[stt+1] - temp_n[stt];
  float y = (temp_n_read[stt+1] - temp_n_read[stt]);
  return x/y;
}

uint16_t measure (long read_value) { 
  static uint8_t check_setup = check_setup_value()-1;
  Serial.print("CHECKK: \t");
  Serial.println(check_setup +1);
  for( uint8_t i = 0; i < check_setup; i++) {
    if (read_value <= temp_n_read[0]) 
      return 0;
    else if( read_value >= temp_n_read[check_setup])
      return temp_n[check_setup];
    else if ((read_value >= temp_n_read[i]) && (read_value < temp_n_read[i+1])) {
      // Serial.println(temp_n_read[i]);
      // Serial.println(temp_n_read[i+1]);
      // Serial.println(temp_n[i]);
      // Serial.println(alpha(i));
      return temp_n[i] + (alpha(i) * (read_value - temp_n_read[i]));
    } 
  }
} 

uint8_t check_setup_value() {
  static uint8_t count =0;
  for(uint8_t i =0; i <10; i++) {
    if ( n[i] == -1) continue;
    else {
      // temp_n[count] = n[i];
      // temp_n_read[count] = n_read[i];   
      temp_n[count] = EEPROM.get(i*2, temp_n[count]);             //n[] == save_n(i);
      temp_n_read[count] = EEPROM.get(20 + (i*4), temp_n_read[count]);   //n_read ==  save_n_read(i);
      count++;
    }
  }
  Serial.print("CHECK COUNT: \t");
  Serial.println(count);
  return count;
}
//-------------------------REBOOT---------------
void (*resetFunc)(void) = 0;