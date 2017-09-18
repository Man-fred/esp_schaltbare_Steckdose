#ifndef COMMON_H
#define COMMON_H
// #define DEBUG_ESP_HTTP_SERVER
#define DEBUG_OUTPUT Serial
#define DBG_OUTPUT_PORT Serial

#define LED_ON LOW
#define LED_OFF HIGH

String mVersionNr = "E00-03-05.ino.";
#ifdef ARDUINO_ESP8266_NODEMCU
  const byte board = 1;
  String mVersionBoard = "nodemcu";
#elif ARDUINO_ESP8266_WEMOS_D1MINI
  const byte board = 2;
  String mVersionBoard = "d1_mini";
#else
  const byte board = 3;
  String mVersionBoard = "unknown";
#endif

// enables OTA updates
#include <ESP8266httpUpdate.h>

#include <ESP8266WebServer.h>

ESP8266WebServer server;
#define LOGINLAENGE 32
#define COOKIE_MAX 10
#define COOKIE_ADMINISTRATOR 1
#define COOKIE_BENUTZER 2
const char * headerKeys[] = {"User-Agent","Set-Cookie","Cookie","Date","Content-Type","Connection"} ;
size_t headerKeysCount = 6;

//const char sketchVersion[5] = "0.3\0";
char AdminName[LOGINLAENGE] = "admin\0";
char AdminPasswort[LOGINLAENGE] = "\0";
char UserName[LOGINLAENGE] = "user\0";
char UserPasswort[LOGINLAENGE] = "\0";
char UpdateServer[LOGINLAENGE] = "192.168.178.60\0";
char ZeitServer[LOGINLAENGE] = "192.168.178.1\0";
int  UserCookie[COOKIE_MAX];// = [0,0,0,0,0,0,0,0,0,0];
int  UserStatus[COOKIE_MAX];// = [0,0,0,0,0,0,0,0,0,0];
int  UserNext=0;
int  UserCurrent = -1;

boolean sommerzeit = false;
const char* serverIndex = "<form method='POST' action='/upload' enctype='multipart/form-data'><input type='file' name='upload'><input type='submit' value='Upload'></form>";

#endif
