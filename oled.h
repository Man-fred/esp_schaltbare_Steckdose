#ifndef OLED_H
#define OLED_H
# include <Adafruit_GFX.h>
# include <Adafruit_SSD1306.h>

#include "zcommon.h"
# define DISPLAY_I2C_ADDRESS 0x3C 
# define OLED_RESET 4

Adafruit_SSD1306 oled(OLED_RESET);

// by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
// Show image buffer on the display hardware.
// Since the buffer is intialized with an Adafruit splashscreen
// internally, this will display the splashscreen.
void oledSplash() {
  oled.begin(SSD1306_SWITCHCAPVCC, DISPLAY_I2C_ADDRESS);  // initialize with the I2C addr 0x3C (for the 128x32)
  oled.display();
}

void testOled() {
  oled.clearDisplay();

  // text display tests
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  oled.setCursor(0,0);
  oled.println("Hello, world!");
  oled.setTextColor(BLACK, WHITE); // 'inverted' text
  oled.println(3.141592);
  oled.setTextSize(2);
  oled.setTextColor(WHITE);
  oled.print("0x"); 
  oled.println(0xDEADBEEF, HEX);
  oled.display();
}

void oledStatus() {
  oled.clearDisplay();

  // text display tests
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  oled.setCursor(0,0);
  if (WLANok) {
    oled.setTextColor(BLACK, WHITE); // 'inverted' text
    oled.print(" WLAN ");
  } else if (AP){
    oled.setTextColor(WHITE);
    oled.print("  AP  ");
  } else {
    oled.setTextColor(WHITE);
    oled.print(" wlan ");
  }
  if (NTPok) {
    oled.setTextColor(BLACK, WHITE); // 'inverted' text
    oled.print(" NTP ");
  } else {
    oled.setTextColor(WHITE);
    oled.print(" ntp ");
  }
  if (RTCok) {
    oled.setTextColor(BLACK, WHITE); // 'inverted' text
    oled.print(" RTC ");
  } else {
    oled.setTextColor(WHITE);
    oled.print(" rtc ");
  }
  if (IOok) {
    oled.setTextColor(BLACK, WHITE); // 'inverted' text
    oled.print(" I/O ");
  } else {
    oled.setTextColor(WHITE);
    oled.print(" i/o ");
  }
  oled.setTextColor(WHITE);
  oled.println("");
  oled.println(PrintDate(now()) + " " + PrintTime(now()) );//+ "("+String(abs(RTCTime - NTPTime))+")");
  oled.println(PrintDate(RTCTime) + " " + PrintTime(RTCTime) );//+ "("+String(abs(RTCTime - NTPTime))+")");
  //oled.println("naechster Timer:");  
  //oled.println("...");//("22.03.18 22:00 R2 aus");
  oled.display();
}


#endif
