#ifndef COMMON_H
#define COMMON_H
#define DEBUG_ESP_HTTP_SERVER
#define DEBUG_OUTPUT Serial
#define DBG_OUTPUT_PORT Serial
#include <ESP8266WebServer.h>

ESP8266WebServer server;
#define LOGINLAENGE 32
#define COOKIE_MAX 10
#define COOKIE_ADMINISTRATOR 1
#define COOKIE_BENUTZER 2
const char * headerKeys[] = {"User-Agent","Set-Cookie","Cookie","Date","Content-Type","Connection"} ;
size_t headerKeysCount = 6;

const char sketchVersion[5] = "0.2\0";
char AdminName[LOGINLAENGE] = "admin\0";
char AdminPasswort[LOGINLAENGE] = "\0";
char UserName[LOGINLAENGE] = "user\0";
char UserPasswort[LOGINLAENGE] = "\0";
int  UserCookie[COOKIE_MAX];// = [0,0,0,0,0,0,0,0,0,0];
int  UserStatus[COOKIE_MAX];// = [0,0,0,0,0,0,0,0,0,0];
int  UserNext=0;
int  UserCurrent = -1;
const char* serverIndex = "<form method='POST' action='/upload' enctype='multipart/form-data'><input type='file' name='upload'><input type='submit' value='Upload'></form>";

#endif
