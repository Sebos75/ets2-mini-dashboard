/*
ETS2 Mini-Dasboard
Author: Sebastian Tracz
sebos75@gmail.com
*/



#include "SPI.h"
#include "Adafruit_WS2801.h"


// oled 128 x 32
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


// strip leds
const int LED_MAX = 16;



// XBitMap Files (*.xbm), exported from GIMP,

static const unsigned char PROGMEM Cruise_Control_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0xc0, 0x07, 0x00, 0x00,
   0x80, 0x0f, 0x00, 0x00, 0x80, 0x0f, 0x00, 0x00, 0xc0, 0x07, 0x00, 0x00,
   0x00, 0x06, 0x00, 0x00, 0x00, 0xf0, 0x0f, 0x00, 0x00, 0xfe, 0x7f, 0x00,
   0x00, 0x8f, 0xf1, 0x00, 0xc0, 0x83, 0xc1, 0x03, 0xc0, 0x00, 0x01, 0x03,
   0xe0, 0x01, 0x80, 0x07, 0xf0, 0x31, 0xc0, 0x0f, 0x38, 0x31, 0x80, 0x0c,
   0x18, 0x60, 0x00, 0x18, 0x18, 0x60, 0x00, 0x18, 0x0c, 0xc0, 0x00, 0x38,
   0x0c, 0xc0, 0x00, 0x30, 0x0c, 0xc0, 0x01, 0x30, 0x3c, 0xc0, 0x01, 0x3c,
   0x7c, 0xc0, 0x03, 0x3e, 0x0c, 0x80, 0x01, 0x30, 0x0c, 0x00, 0x00, 0x30,
   0x0c, 0x00, 0x00, 0x38, 0x18, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x18,
   0x38, 0x01, 0x80, 0x0c, 0xf0, 0x01, 0xc0, 0x0f, 0xe0, 0x01, 0x80, 0x07,
   0xc0, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00 };



enum LED_TYPE  {
  LEFT_INDICATOR,
  LEFT_LIGHT,
  LEFT_BREAK,
  LEFT_REVERSE,
  LIGHT,
  LIGHT_H,
  ENGINE,
  FUEL,
  PARKING_BRAKE,
  RIGHT_REVERSE,
  RIGHT_BREAK,
  RIGHT_LIGHT,
  RIGHT_INDICATOR
};



// RBG
const uint32_t COLOR_RED        = 0xaa0000;
const uint32_t COLOR_RED_LIGHT  = 0x100000;
const uint32_t COLOR_YELLOW     = 0x440044;
const uint32_t COLOR_YELLOW_LIGHT= 0x07000a;
const uint32_t COLOR_WHITE      = 0x444444;
const uint32_t COLOR_GREEN      = 0x000088;
const uint32_t COLOR_GREEN_LIGHT = 0x000010;
const uint32_t COLOR_BLUE       = 0x008800;

const byte LAYER_BOTTOM  = 1;
const byte LAYER_TOP     = 2;


struct Pixel {
  LED_TYPE  typ;
  byte      position;
  byte      layer;
  uint32_t  color;
  byte      bitInput;
  boolean   engine;
};


const byte a_items = 13;

struct Pixel aPixels[a_items] =
{
  {LEFT_INDICATOR,  1,  LAYER_BOTTOM, COLOR_YELLOW, 5},
  {LEFT_LIGHT, 2, LAYER_BOTTOM, COLOR_RED_LIGHT, 6, true},
  {LEFT_BREAK, 2, LAYER_TOP, COLOR_RED, 1},
  {LEFT_REVERSE,3,LAYER_BOTTOM,COLOR_WHITE,0},
  
  {LIGHT,8,LAYER_BOTTOM,COLOR_GREEN_LIGHT,3, true},
  {LIGHT_H,8,LAYER_TOP,COLOR_BLUE,2, true},
  
  {ENGINE,9,LAYER_BOTTOM,COLOR_YELLOW_LIGHT,20},
  {FUEL,10,LAYER_BOTTOM,COLOR_YELLOW,13},
  {PARKING_BRAKE,10,LAYER_TOP,COLOR_RED,17},
  
  {RIGHT_REVERSE,14,LAYER_BOTTOM,COLOR_WHITE,0},
  {RIGHT_BREAK, 15, LAYER_TOP, COLOR_RED, 1},
  {RIGHT_LIGHT, 15, LAYER_BOTTOM, COLOR_RED_LIGHT, 6,true},
  {RIGHT_INDICATOR, 16, LAYER_BOTTOM, COLOR_YELLOW, 4}
  
};

uint32_t aBottomLayer[LED_MAX];
uint32_t aTopLayer[LED_MAX];

// strip WS2801
const int stripDataPin  = 15;    // Yellow wire on Adafruit Pixels
const int stripClockPin = 16;    // Green wire on Adafruit Pixels


#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);


Adafruit_WS2801 strip = Adafruit_WS2801((uint16_t)LED_MAX, (uint16_t)1, stripDataPin, stripClockPin);



void setup() {
  Serial.begin(115200);
  strip.begin();
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  display.display();

  display.clearDisplay();
  
  
  
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.println(" * ETS2 *");
  display.println("dashboard");
  delay(1000);
  
  test_lights(1500,1);
  delay(1000);
  test_lights(1500,0);
  show_cruise_control(0);

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0,0);
  display.println(" * ETS2 * dashboard            ");
  display.println("  dashboard                    ");
  display.display();
  display.startscrollleft(1,50000);

  
}

#define PACKET_SYNC 0xFF
#define PACKET_VER  2

int serial_byte;
int cruise_control;
int serial_lights, serial_lights2, serial_lights3;
int text_len;

void loop() {
  
  if (Serial.available() < 16)
    return;
  
  serial_byte = Serial.read();
  if (serial_byte != PACKET_SYNC)
    return;
    
  serial_byte = Serial.read();
  if (serial_byte != PACKET_VER)
  {
    return;
  }
  display.stopscroll();
  serial_byte = Serial.read(); // speed
  serial_byte = Serial.read(); // rpm
  serial_byte = Serial.read(); // Brake air pressure
  serial_byte = Serial.read(); // Brake temperature
  serial_byte = Serial.read(); // Fuel ratio
  serial_byte = Serial.read(); // Oil pressure
  serial_byte = Serial.read(); // Oil temperature
  serial_byte = Serial.read(); // Water temperature
  serial_byte = Serial.read(); // Battery voltage
  
  serial_byte = Serial.read(); // cruise control
  cruise_control = round(serial_byte * 3.6);
  
  serial_lights = Serial.read();
  serial_lights2 = Serial.read();
  serial_lights3 = Serial.read();

  strip_show_controls(serial_lights,serial_lights2,serial_lights3);
  show_cruise_control(cruise_control);
  
 
  
  // Text length
  text_len = Serial.read();
  
  // Followed by text
  if (0 < text_len && text_len < 127)
  {
    for (int i = 0; i < text_len; ++i)
    {
      while (Serial.available() == 0) // Wait for data if slow
      {
        delay(2);
      }
      serial_byte = Serial.read();
      if (serial_byte < 0 && serial_byte > 127)
        return;
      
    }
  }


}


void led(LED_TYPE typ, boolean bOn)
{
  uint32_t color = 0;
  byte index;
  
  color = 0;
  // check index
  for (byte i=0;i<a_items;i++){
    if (aPixels[i].typ == typ){
      index = aPixels[i].position -1;
      if (bOn) {
        color = aPixels[i].color;
      }
      if (aPixels[i].layer == LAYER_BOTTOM) {
        aBottomLayer[index] = color;
      } else { 
        aTopLayer[index] = color;
      }
      break;
    }
  }

  if (aTopLayer[index] > 0) {
    color = aTopLayer[index];
  } else {
    color = aBottomLayer[index];
  }
  strip.setPixelColor(index, color);
  strip.show();
}



void test_lights(int time,boolean on){
  for (byte i=0;i<a_items;i++){
    led(aPixels[i].typ,on);
    delay(time / 60);
    if (on) {
      show_cruise_control(i*30);
    }
  }
  delay(time);
}
void strip_show_controls(int ser_lights,int ser_lights2,int ser_lights3) {
  byte state;
  boolean engine_state;
  engine_state = (ser_lights3 & 0x01);
  for (byte i=0;i<a_items;i++){
    if (aPixels[i].bitInput < 10) {
      state = ser_lights >> aPixels[i].bitInput & 0x01;
    } else if (aPixels[i].bitInput < 20) {
      state = ser_lights2 >> (aPixels[i].bitInput-10) & 0x01;
    } else {
      state = engine_state;
    }

    if (aPixels[i].engine == true && engine_state == false)
    {
      state = false;
    }
    led(aPixels[i].typ,state);
  }
  
}

void show_cruise_control(int val){
  display.clearDisplay();
  display.setTextSize(4);
  display.setTextColor(WHITE);
  display.setCursor(38,0);
  if (val)
  {
    // Clear the buffer.
    display.drawXBitmap(0,0,Cruise_Control_bits,32,32,1);
    display.println(val);
  } 
  display.display();
}

