#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <FS.h>
#include "common.h";

#include "fs.h";
#include "log.h"
#include "ntp.h"

#ifdef IICTEST
# include <Wire.h>               //http://arduino.cc/en/Reference/Wire (included with Arduino IDE)
# define PIN_SDA D2// bisher NodeMCU D5
# define PIN_SCK D1// bisher NodeMCU D6
# define IO_I2C_ADDRESS 0x20     
# define RTC_I2C_ADDRESS 0x68
# include "Adafruit_MCP23017.h"  //https://github.com/adafruit/Adafruit-MCP23017-Arduino-Library http://ww1.microchip.com/downloads/en/DeviceDoc/20001952C.pdf
  Adafruit_MCP23017 mcp;
# include "RTClib.h"             //https://github.com/adafruit/RTClib
  RTC_DS3231 RTC;
# include "oled.h";
# define dPinModeOutput(pin) mcp.pinMode(pin, OUTPUT);
# define dPinModeInput(pin) mcp.pinMode(pin, INPUT);
# define dPinModeInputPullup(pin) mcp.pinMode(pin, INPUT); mcp.pullUp(pin, HIGH);  // turn on a 100K pullup internally
# define dWrite(pin, value) mcp.digitalWrite(pin, value)
# define dRead(pin) mcp.digitalRead(pin)
  byte Taster[4] = {5, 4, 7, 6}; // A4-A7
  byte Relay[4] = {8, 9, 10, 11}; //B1-B4
  byte LED_IIC[4] = {12, 13, 14, 15}; //B5-B8
# ifdef USE_LED_BUILTIN
    byte statusLED = LED_BUILTIN; // D4 -> 2 bei Wemos d1 mini, D0 -> 16 bei Nodemcu
#   define dPinModeLED pinMode(statusLED, OUTPUT);
#   define dWriteLED(value) digitalWrite(statusLED, value)
#   define LED_ON LOW
#   define LED_OFF HIGH
# else  
    byte statusLED = 0; // A1
#   define dPinModeLED mcp.pinMode(statusLED, OUTPUT);
#   define dWriteLED(value) mcp.digitalWrite(statusLED, value)
#   define LED_ON LOW
#   define LED_OFF HIGH
# endif
#else
// ********** esp8266 - Ports ohne IIC *****************
# define dPinModeOutput(pin) pinMode(pin, OUTPUT);
# define dPinModeInput(pin) pinMode(pin, INPUT);
# define dPinModeInputPullup(pin) pinMode(pin, INPUT_PULLUP); // turn on a 100K pullup internally
# define dWrite(pin, value) digitalWrite(pin, value)
# define dRead(pin) digitalRead(pin)
  byte Taster[4] = {D2, D1, D4, D3}; //2,0,4,5
  byte Relay[4] = {D5, D6, D7, D0}; //13,12,14,16 (D8 testweise D0)
  byte statusLED = D8; // muss beim Booten unbedingt LOW sein!
# define dPinModeLED pinMode(statusLED, OUTPUT);
# define dWriteLED(value) digitalWrite(statusLED, value)
# define LED_ON LOW
# define LED_OFF HIGH
#endif //ifdef IIC


extern "C" {
#include "user_interface.h"
}
/*
  struct FSInfo {
    size_t totalBytes;
    size_t usedBytes;
    size_t blockSize;
    size_t pageSize;
    size_t maxOpenFiles;
    size_t maxPathLength;
  };
*/
FSInfo fs_info;

IPAddress apIP(192, 168, 168, 30); // wenn in AP-Mode

#include <DNSServer.h>
const byte DNS_PORT = 53;
DNSServer dnsServer;

char ssid[32] = "\0";
char passwort[64] = "\0";

EspClass esp;
int z = 0;                   //Aktuelle EEPROM-Adresse zum lesen
#include "Setup.h"

int i = 0;
// int active_low = 1;          //
// 0- 3: Zustand der Relais,
// 4- 7: Aktion nach Modulstart, nach Stromausfall: 0: aus, 1: ein, 2: letzter Zustand
// 8-11: active_low: "0" beim schalten mit +5V , "1" beim schalten mit 0V
//12-15: Wechselschalter: "1": Strom fließt bei 1 (Relais ein), "0": Strom fließt bei 0 (Relais aus)
//16-31: Aktion für Taster/Relais 1-4/1-4: 1-1, 1-2, 1-3, 1-4, 2-1 ...; Verhalten 0: aus, 1: ein, 2: letzter Zustand, 3: Zustand wechseln, Standard unabhängiges Wechselschalten 
byte val[32] = {0, 0, 0, 0, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 3, 2, 2, 2, 2, 3, 2, 2, 2, 2, 3, 2, 2, 2, 2, 3 };
// Zustand der Taster
byte TasterStatus[4];
unsigned long TasterZeit[4];
byte TasterAktion[4][4];
String Temp = "";
// nach 5 Versuchen für 24 Std aufgeben
int timeout = 0; //
byte statusLEDsek = 0;

/*  Status-Flags:
    normale Funktion:  !AP, WLANok, !inSetup, NTPTime -> ruhig Blinken 5 / 5 sek
    in Setup
    AP                 schnelles Blinken 1 / 1 sek
    !WLANok         länger an als aus 5 / 1 sek
    NTP == 0           länger aus als an 1 / 5 sek
*/

boolean inSetup = true;

#include "timer.h"

void Zeit_Einstellen()
{
  if (WLANok) {
    NTPTime = GetNTP();
  } else {
    NTPTime = 0;
    NTPok = false;
  }

  if (NTPok) {
    if(abs(NTPTime - now()) > 5) {
      LogSchreibenNow("falsche Zeit");
      //Serial.println( Temp );
      setTime(NTPTime);
      LogSchreiben("NTP: Zeit gesetzt");
      //Serial.println( Temp );
    }
  }
# ifdef IICTEST
    if (RTCok) {
      RTCTime = RTC.now().unixtime();
      if (NTPok) {
        if(abs(RTCTime - NTPTime) > 5) {
          Temp = PrintDate(RTCTime) + "   " + PrintTime(RTCTime) + "   falsche RTC-Zeit";
          LogSchreiben(Temp);
          //Serial.println( Temp );
          RTCSync = NTPTime;
          RTC.adjust(DateTime(year(NTPTime), month(NTPTime), day(NTPTime), hour(NTPTime), minute(NTPTime), second(NTPTime)));
        }
      } else {
        if(abs(RTCTime - now()) > 5) {
          LogSchreibenNow("falsche Zeit");
          setTime(RTCTime);
          LogSchreiben("RTC: Zeit gesetzt");
        }
      }
    }
# endif
}

void WlanStation()
{
  if (AP) {
    WLANok = false;
  } else {
    if (!WLANok) {
      Serial.print("Verbinde mit ");
      Serial.println(ssid);
      WiFi.setPhyMode(WIFI_PHY_MODE_11G);
      WiFi.setOutputPower(2.5);
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, passwort);
      timeout = 0;
      Serial.print("Status ");
      Serial.println(WiFi.status());
      //while (WiFi.status() != WL_CONNECTED && !WiFi.localIP())
      while (!WiFi.localIP())
      {
        delay(500);
        Serial.print("O");
        if  (timeout++ > 10) // Wenn Anmeldung nicht möglich
        {
          Serial.println("");
          Serial.println("Wlan verbindung fehlt");
          break;
        }
      }
      Serial.print("WL_Status ");
      Serial.println(WiFi.status());
      WiFi.printDiag(Serial);
    }
    //if (WiFi.status() == WL_CONNECTED)
    if (WiFi.localIP())
    {
      WLANok = true;
      Serial.println("");
      Serial.println("Mit Wlan verbunden");
      Serial.print("IP Adresse: ");
      Serial.println(WiFi.localIP());
    } else {
      WLANok = false;
    }
  }
}

void Relais_Init()
{
  if (SPIFFS.exists("/relais.dat")) // Relaiszustand aus Datei laden
  {
    File DataFile = SPIFFS.open("/relais.dat", "r");
    DataFile.read(reinterpret_cast<uint8_t*>(&val), sizeof(val));
    DataFile.close();
  }  else {
    Serial.println("Datei relais.dat fehlt");
    File DataFile = SPIFFS.open("/relais.dat", "w");
    DataFile.write(reinterpret_cast<uint8_t*>(&val), sizeof(val));
    DataFile.close();
    Serial.println("Datei relais.dat erzeugt");
  }
  // Initialisierung nach Start
  for (int k = 0; k < 4; k++) {
    if (val[k + 4] < 2) {
      val[k] = val[k + 4];
    }
    Relais_Schalten(k, val[k], "Server gestartet");
  }
}

void Einstellen()
{
  if (!AP) {
    Serial.println("e: Einstellen");
    z = 0;                        // EEPROM Start der Daten
    getEeprom();

    Serial.println("Starte in Access Point modus" );
    Serial.println("IP http://192.168.168.30");
    Serial.print("SSID: WebSchalter, Passwort: tiramisu");
    WiFi.mode(WIFI_AP);      // access point modus
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP("WebSchalter", "tiramisu");  // Name des Wi-Fi netzes
    dnsServer.start(DNS_PORT, "*", apIP);
    AP = 1;
  }
}

void printUser() {
  if (!AdminPasswort[0])
    Serial.println("Serverauthentifizierung ausgeschaltet");
  else
  {
    Serial.println("Serverauthentifizierung eingeschaltet");
    Serial.print("Administratorname: ");
    Serial.print(AdminName);
    Serial.print(", Administratorpasswort : ");
    Serial.println(AdminPasswort);
    if (!UserPasswort[0])
      Serial.println("kein Benutzer neben \"admin\" angelegt");
    else
    {
      Serial.print("Benutzername : ");
      Serial.print(UserName);
      Serial.print(", Benutzerpasswort : ");
      Serial.println(UserPasswort);
    }
  }
}
void readInput() {
  char inser = Serial.read();
  if (inser == 'e' || (inSetup && !digitalRead(Taster[0]) && !digitalRead(Taster[1]))) {
    Einstellen();
  } else if (false && inser == 'f' || (inSetup && !digitalRead(Taster[0]) && !digitalRead(Taster[2]))) {
    Serial.println("Please wait 30 secs for SPIFFS to be formatted");
    SPIFFS.format();
    Serial.println("Spiffs formatted");
    //See more at: http://www.esp8266.com/viewtopic.php?f=29&t=8194#sthash.mj02URAZ.dpuf
    SPIFFS.info(fs_info);
    Serial.println("totalBytes " + String(fs_info.totalBytes));
  } else if (inser == 'i' || (inSetup && !digitalRead(Taster[0]) && !digitalRead(Taster[3]))) {
    Serial.println("");
    Serial.println("Mit Wlan verbunden");
    Serial.print("IP Adresse: ");
    Serial.println(WiFi.localIP());
    Serial.println("Zeit: " + PrintDate(now()) + " " + PrintTime(now()));
    printUser();
  }
}

# ifdef IICTEST
void testIIC()
{
  byte error, address;
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for (address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      if (address == IO_I2C_ADDRESS)  IOok  = 1;
      if (address == RTC_I2C_ADDRESS) RTCok = 1;
      if (address == DISPLAY_I2C_ADDRESS) DISPLAYok = 1;
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");

      nDevices++;
    }
    else if (error == 4)
    {
      Serial.print("Unknown error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
}
#endif


// Wird 1 Mal beim Start ausgefuehrt
void setup()
{
  Serial.begin(115200);
  Serial.println("Setup()");
  
  Serial.setDebugOutput(true);
# ifdef IICTEST
    Wire.begin(PIN_SDA, PIN_SCK);
    testIIC();
    if (IOok) {
      mcp.begin(0);
    }
    if (DISPLAYok) {
      oledSplash();
    }
# endif
  for (int k = 0; k < 4; k++) {
    dPinModeInputPullup(Taster[k]);
    dPinModeOutput(Relay[k]);
    dWrite(Relay[k], val[8+k]);
#   ifdef IICTEST
      dPinModeOutput(LED_IIC[k]);
      dWrite(LED_IIC[k], 0);
#   endif
  }
  dPinModeLED;
  char inser;               // Serielle daten ablegen
  String nachricht = "";    //  Setup Formular

  EEPROM.begin(512);                                 // EEPROM initialisieren mit 200 Byts
  if (!SPIFFS.begin()) Serial.println("Failed to mount file system");

  while (Serial.available())
    inser = Serial.read();
  Serial.println("Start mit Pause,");
  Serial.println("Einstellen: e+<Enter>,");
  Serial.println("Format SPIFFS: f+<Enter>,");
  Serial.println("Info: i+<Enter>");
}

// nach 2 Sekunden 2. Teil von setup
void setup2() {
  Serial.println("Weiter");
  Relais_Init();

# ifdef IICTEST
  if (RTCok) {
    RTC.begin();
    RTCTime = RTC.now().unixtime();
    setTime(RTCTime);
    Serial.println(PrintDate(now()) + "   " + PrintTime (now()) + "   RTC gestartet");
  } else {
    Serial.println("Unable to sync with the RTC");
  }
# endif
  if (!AP) {
    z = 0;
    LeseEeprom(ssid, sizeof(ssid));        // EEPROM lesen
    if (ssid[0] == 255 || ssid[0] == '\0') // Wenn EEPROM leer oder SSID nicht gesetzt ist dann:
    {
      //for (i = 0; i < 6; i++) EEPROM.write(i, '\0');
      //EEPROM.commit();
      Einstellen();
    }
    else {
      // Wenn ssid angegeben dann in Stationmodus mit Router verbinden
      getEeprom();
      WlanStation();
      Zeit_Einstellen();

      Temp = PrintDate(now()) + "   " + PrintTime (now()) + "   Server gestartet";
      LogSchreiben(Temp);

      // gespeicherte Timer laden und Reihenfolge setzen, hier mit Argument neustart=true
      Timer_Laden(true);
    }
  } else {
    Zeit_Einstellen();
  }

  // ENDE Stationmodus / Access Point modus Auswahl
# ifdef IICTEST
    if (DISPLAYok) {
      oledStatus();
    }
# endif
  printUser();
  httpStart();
  Serial.println("Freies RAM = " + String(system_get_free_heap_size()));
  inSetup = false;
}

void getEeprom() {
  z = 0;
  LeseEeprom(ssid, sizeof(ssid));
  LeseEeprom(passwort, sizeof(passwort));
  LeseEeprom(AdminPasswort, LOGINLAENGE);
  LeseEeprom(AdminName, LOGINLAENGE);
  LeseEeprom(UserPasswort, LOGINLAENGE);
  LeseEeprom(UserName, LOGINLAENGE);
  LeseEeprom(UpdateServer, LOGINLAENGE);
  LeseEeprom(timeserver, LOGINLAENGE);
  LeseEeprom(name_timer, LOGINLAENGE);
  LeseEeprom(name_r[0], LOGINLAENGE);
  LeseEeprom(name_r[1], LOGINLAENGE);
  LeseEeprom(name_r[2], LOGINLAENGE);
  LeseEeprom(name_r[3], LOGINLAENGE);
  if (LeseEepromCheck() != 0x55)
  {
    ssid[0] = 0;
    passwort[0] = 0;
    AdminPasswort[0] = 0;
    strcpy(AdminName, "admin");
    UserPasswort[0] = 0;
    strcpy(UserName, "user");
    UpdateServer[0] = 0;
    strcpy(timeserver, "time.nist.gov");
    strcpy(name_timer, "Wifi 4-fach Timer");
    name_r[0][0] = 0;
    name_r[1][0] = 0;
    name_r[2][0] = 0;
    name_r[3][0] = 0;
  }
  //LeseEepromStr(&nachricht,100);
  /*
    Serial.println(ssid);
    Serial.println(passwort);
    Serial.println(url);
    Serial.println(nachricht);
  */

}

void httpStart() {
  // Behandlung der Ereignisse anschliessen
  // Seiten aus dem SPIFFS-Speicher werden bei onNotFoud() ausgewertet
  server.on("/daten.js", ConfigJson);
  server.on("/setup.php", ConfigSave);

  server.on("/schalte.php", Ereignis_Schalte);      // Aktion bei index.html
  server.on("/zustand.php", Ereignis_Zustand);      // Ajax-Antwort bei index.html

  server.on("/timer.json", Ereignis_Timer_JSON);    // json-Daten bei timer.html
  server.on("/settimer.php", Ereignis_NeueTimer);   // Formulardaten für neu/anpassen bei timer.html
  server.on("/delete.php", Ereignis_DeleteTimer);   // Formulardaten für löschen bei timer.html
  server.on("/laden.html", Ereignis_Timer_Laden);   // Timer neu aus Speicher laden und sortieren

  server.on("/deletelog.php", Ereignis_DeleteLog);  // löscht ohne Rückfrage die Datei log.txt

  server.on("/update.php", updateVersion);          // Update OTA

  //list directory
  server.on("/list", HTTP_GET, handleFileList);

  server.on("/upload", HTTP_GET, []() {
    if (is_authentified()) {
      server.sendHeader("Connection", "close");
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "text/html", serverIndex);
    }
  });
  server.on("/upload", HTTP_POST, []() {
    if (is_authentified()) {
      server.send(200, "text/html", serverIndex);
    }
  }, handleFileUpload);
  
  server.on("/upload.json", HTTP_POST, []() {
    if (is_authentified()) {
      server.send(200, "text/json", "{\"ok\":1}");
    }
  }, handleFileUpload);
  server.on("/delete", HTTP_GET, handleFileDelete);

  //called when the url is not defined here
  //use it to load content from SPIFFS
  server.onNotFound([]() {
    if (is_authentified()) {
      if (!handleFileRead(server.uri())) {
        String notfound = "File Not Found\r\n\r\n";
        notfound += server.uri();
        notfound += ( server.method() == HTTP_GET ) ? " GET " : " POST ";
        notfound += server.args();

        for ( byte i = 0; i < server.args(); i++ ) {
          notfound += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\r\n";
        }
        server.send(404, "text/plain", notfound);
      }
    }
  });
  server.collectHeaders(headerKeys, headerKeysCount);
  server.begin();  // Starte den Server
  Serial.println("HTTP Server gestartet");
}

byte newCookie(int level) {
  if (++UserNext >= COOKIE_MAX) UserNext = 0;
  UserCookie[UserNext] = random(1000000, 9999999);
  UserStatus[UserNext] = level;
  server.sendHeader("Set-Cookie", "ESPSESSIONID=" + String(UserCookie[UserNext]));
  UserCurrent = UserNext;
  Serial.println("Login " + String(UserNext) + String(UserStatus[UserNext]));
  return UserNext;
}

//Check if header is present and correct
byte is_authentified() {
  //AdminPasswort[0] = 0;
  //Serial.println(server.client().remoteIP().toString()+":"+String(server.client().remotePort()) );
  if (server.hasHeader("Cookie")) {
    //Serial.print("Authentification cookie: ");
    String cookie = server.header("Cookie");
    //Serial.print(cookie);
    if (cookie.indexOf("ESPSESSIONID=") != -1) {
      int testuser = cookie.substring(13, 20).toInt();
      if (testuser > 0) {
        //Serial.print(" test ");
        //Serial.print(testuser);
        for (int k = 0; k < 10; k++) {
          //Serial.print(" test ");
          //Serial.print(UserCookie[k]);
          if (UserCookie[k] == testuser) {
            UserCurrent = k;
            //Serial.print(" user ");
            //Serial.print(UserCurrent);
            //Serial.println(" successful");
            return k;
          }
        }
        //Serial.println(" failed");
      }
    }
  }
  //Serial.println("Authentification No Cookie");
  if (!AdminPasswort[0]) { // Wenn Benutzerpasswort angegeben wurde
    Serial.println("Login nicht nötig");
    return newCookie(COOKIE_ADMINISTRATOR);
  } else if (server.authenticate(&AdminName[0], &AdminPasswort[0])) {
    return newCookie(COOKIE_ADMINISTRATOR);
  } else if (UserPasswort[0] && server.authenticate(&UserName[0], &UserPasswort[0]) ) {
    Serial.println("Login User");
    return newCookie(COOKIE_BENUTZER);
  } else {
    server.requestAuthentication();
    return 0;
  }
}

void ConfigRoute()
{
  server.sendHeader("Location", "/index.htm#page3");
  server.send(303, "text/html", ""); // Antwort an Internet Browser
}

void Relais_Schalten(int datensatz, byte ein, String logtext)
{
  if (NTPTime == 0) {
    Temp = "                   ";
  } else {
    Temp = PrintDate(now()) + "   " + PrintTime (now());
  }
  Temp += "   Relais: " + String(datensatz + 1);
  if (ein > 0) {
    Temp += " ON ";
    val[datensatz] = 1;
  } else {
    Temp += " OFF";
    val[datensatz] = 0;
  }
  Temp += "     ";
  Temp += logtext;
  dWrite(Relay[datensatz], val[datensatz + 8] ^ !val[datensatz + 12] ^ val[datensatz]);
# ifdef IICTEST
    dWrite(LED_IIC[datensatz], val[datensatz]);
# endif

  Serial.println(Temp);
  LogSchreiben(Temp);

  File DataFile = SPIFFS.open("/relais.dat", "r+");
  DataFile.seek(sizeof(val[0])*datensatz, SeekSet);
  DataFile.write(reinterpret_cast<uint8_t*>(&val[datensatz]), sizeof(val[0]));
  DataFile.close();
}

void Ereignis_Schalte()
{
  if (is_authentified()) {
    String RelayNr = server.arg("Relay");          // Relais Nr. empfangen
    int RelayNrInt = RelayNr.toInt();
    String ArgumentOn = server.arg("On");
    byte RelayOn = ArgumentOn.toInt();
    Relais_Schalten(RelayNrInt - 1, RelayOn, server.client().remoteIP().toString());
    Ereignis_Zustand();
  }
}

void Ereignis_Zustand()
{
  if (byte nr = is_authentified()) {
    String Antwort = "";
    for (int k = 0; k < 16; k++) {
      Antwort += String(val[k]) + ";";
    }
    Antwort += PrintDate(now()) + " " + PrintTime(now()) + ";";
    // Version + ChipId
    Antwort += mVersionNr + mVersionVariante + mVersionBoard;
    Antwort += ";" + String(esp.getChipId()) + ";";
    Antwort += (UserStatus[nr] == COOKIE_ADMINISTRATOR ? "Administrator" : "Eingeschränkt");
    Antwort += ";" + String(NTPok) + ";" + String(RTCok) + ";" + String(IOok) + ";" + String(DISPLAYok);
    Antwort += ";" + String(name_timer) + ";" + String(name_r[0]) + ";" + String(name_r[1]) + ";" + String(name_r[2]) + ";" + String(name_r[3]);
    server.sendHeader("Cache-Control", "no-cache");
    server.send(200, "text/plain", Antwort); // Antwort an Internet Browser senden
  }
}

void Ereignis_log()    //
{
  if (is_authentified()) {
    //if (!SPIFFS.begin()) Serial.println("Failed to mount file system");
    File DataFile = SPIFFS.open("/log.txt", "r");
    if (!DataFile)Serial.println("Failed to open log.txt");
    server.sendHeader("Cache-Control", "no-cache");
    size_t sent = server.streamFile(DataFile, "text/plain");
    DataFile.close();
  }
}

void Ereignis_DeleteLog()
{
  if (is_authentified()) {
    if (AdminPasswort[0]) // Wenn Benutzerpasswort angegeben wurde
    {
      if (!server.authenticate(&AdminName[0], &AdminPasswort[0]))
        return server.requestAuthentication();
    }
    if (SPIFFS.remove("/log.txt") ) Serial.println("Datei log.txt gelöscht");
    server.send(200, "text/plain", "log geloescht");
  }
}


void ConfigJson()      // Wird ausgeuehrt wenn "http://<ip address>/" aufgerufen wurde
{
  if (is_authentified()) {
    int i;
    if (ssid[0] == 255) ssid[0] = 0;            // Wenn speicher leer alles auf 0
    if (passwort[0] == 255) passwort[0] = 0;
    if (AdminPasswort[0] == 255) AdminPasswort[0] = 0;

    String temp = "";              // Sternchen einfügen bei Passwortanzeige

    temp += "{\"ssid1\":\"" + String(ssid) + "\",";
    temp +=  "\"pass1\":\"" + String(passwort) + "\",";
    temp +=  "\"name2\":\"" + String(AdminName) + "\",";
    temp +=  "\"pass2\":\"" + String(AdminPasswort) + "\",";
    temp +=  "\"name3\":\"" + String(UserName) + "\",";
    temp +=  "\"pass3\":\"" + String(UserPasswort) + "\",";
    temp +=  "\"timeserver\":\"" + String(timeserver) + "\",";
    temp +=  "\"name_timer\":\"" + String(name_timer) + "\",";
    temp +=  "\"name_r1\":\"" + String(name_r[0]) + "\",";
    temp +=  "\"name_r2\":\"" + String(name_r[1]) + "\",";
    temp +=  "\"name_r3\":\"" + String(name_r[2]) + "\",";
    temp +=  "\"name_r4\":\"" + String(name_r[3]) + "\",";
    temp +=  "\"update\":\"" + String(UpdateServer) + "\"";
    for (int k = 4; k < 32; k++) {
      temp +=  ",\"setup" + String(k) + "\":" + String(val[k]);
    }
    temp +=  " }";
    server.sendHeader("Cache-Control", "no-cache");
    server.send(200, "text/plain", temp);
  }
}

void ConfigSave()      // Wird ausgeuehrt wenn "http://<ip address>/setup.php"
{
  if (is_authentified()) {
    File DataFile = SPIFFS.open("/relais.dat", "r+");
    DataFile.seek(sizeof(val[0]) * 4, SeekSet);
    for (int k = 4; k < 32; k++) {
      val[k] = server.arg("setup" + String(k))[0] - 48;
      DataFile.write(reinterpret_cast<uint8_t*>(&val[k]), sizeof(val[0]));
    }
    DataFile.close();

    z = 0;
    server.arg("ssid").toCharArray(ssid, server.arg("ssid").length() + 1) ;
    SchreibeEeprom(ssid);
    String temp1 = server.arg("passwort");
    if (temp1[2] != '*') {
      temp1.toCharArray(passwort, temp1.length() + 1) ;
      SchreibeEeprom (passwort);
    } else {
      SchreibeEeprom (passwort);               // wenn mit * dann geladenes passwort abspeichern
    }
    server.arg("AdminName").toCharArray(AdminName, server.arg("AdminName").length() + 1) ;
    server.arg("AdminPasswort").toCharArray(AdminPasswort, server.arg("AdminPasswort").length() + 1) ;
    SchreibeEeprom (AdminPasswort);
    SchreibeEeprom(AdminName);
    server.arg("UserName").toCharArray(UserName, server.arg("UserName").length() + 1) ;
    server.arg("UserPasswort").toCharArray(UserPasswort, server.arg("UserPasswort").length() + 1) ;
    server.arg("UpdateServer").toCharArray(UpdateServer, server.arg("UpdateServer").length() + 1) ;
    server.arg("timeserver").toCharArray(timeserver, server.arg("timeserver").length() + 1) ;
    server.arg("name_timer").toCharArray(name_timer, server.arg("name_timer").length() + 1) ;
    server.arg("name_r1").toCharArray(name_r[0], server.arg("name_r1").length() + 1) ;
    server.arg("name_r2").toCharArray(name_r[1], server.arg("name_r2").length() + 1) ;
    server.arg("name_r3").toCharArray(name_r[2], server.arg("name_r3").length() + 1) ;
    server.arg("name_r4").toCharArray(name_r[3], server.arg("name_r4").length() + 1) ;
    SchreibeEeprom(UserPasswort);
    SchreibeEeprom(UserName);
    SchreibeEeprom(UpdateServer);
    SchreibeEeprom(timeserver);
    SchreibeEeprom(name_timer);
    SchreibeEeprom(name_r[0]);
    SchreibeEeprom(name_r[1]);
    SchreibeEeprom(name_r[2]);
    SchreibeEeprom(name_r[3]);
    SchreibeEepromCheck();
    EEPROM.commit();

    ConfigRoute();
  }
}

void statusLedBlink(byte an, byte aus) {
  if (statusLEDsek++ >= an) {
    dWriteLED(LED_OFF);
    //Serial.print(0);
    if (statusLEDsek > (an + aus))
      statusLEDsek = 0;
  } else {
    dWriteLED(LED_ON);
    //Serial.print(1);
  }
}

void loop()
{
  if (inSetup) {
    if (now() != ZeitTemp)       // Ausführung 1 mal in der Sekunde
    {
      ZeitTemp = now();
      if (timeout++ > 2)
        setup2();
      Serial.print(".");
    }
  } else {
    for (int k = 0; k < 4; k++) {
      // durch Pullup ist Standard 1, gedrückt 0
      int TasterTemp = !dRead(Taster[k]);
      // TasterTemp Standard 0, gedrückt 1
      if (TasterTemp != TasterStatus[k] && millis() - TasterZeit[k] > 250) {
        // Prellen abfangen
        //Serial.print(k);
        TasterZeit[k] = millis();
        TasterStatus[k] = TasterTemp;
        if (TasterTemp)
        {
          val[k] = !val[k];
          String logText = "Taster " + String(k + 1);
          Relais_Schalten(k, val[k], logText);
        }
      }
    }
    time_t jetzt = now();

    if (jetzt != ZeitTemp) {       // Ausführung 1 mal je Sekunde
#     ifdef IICTEST
        oledStatus();
#     endif
      if ( (NTPTime + 86400) < jetzt || (NTPTime == 0 && !RTCok)) { //Zeit Update alle 24Stunden oder wenn gar keine Uhrzeit vorhanden ist
        WlanStation();
        Zeit_Einstellen();
        jetzt = now();
      }    
      if (ZeitTempMin + 60 < jetzt) { // Ausführung 1 mal je Minute
        ZeitTempMin = jetzt;
      }
      if (ZeitTempStd + 3600 < jetzt) { // Ausführung 1 mal je Stunde
        if (sommerzeitTest()) {
          Zeit_Einstellen();
          jetzt = now();
        }
        ZeitTempStd = jetzt;
      }
      if (ZeitTempTag + 86400 < jetzt) { // Ausführung 1 mal je Tag
        ZeitTempTag = jetzt;
      }
      ZeitTemp = jetzt;
      //  in Setup
      if (AP) {
        //  AP                 schnelles Blinken 1 / 1 sek
        statusLedBlink(2, 2);
      } else if (!WLANok) {
        //  !WLANok         länger an als aus 5 / 1 sek
        statusLedBlink(5, 2);
      } else if ((NTPTime + 86400) < jetzt) {
        //  NTP ungültig       länger aus als an 1 / 5 sek
        statusLedBlink(2, 5);
      } else {
        //  normale Funktion "Station":  !AP, WLANok, !inSetup, NTPTime -> ruhig Blinken 5 / 5 sek
        statusLedBlink(5, 5);
      }
      // Timer auch wenn offline, aber nur wenn gueltige Zeit
      if (NTPTime || RTCok) {
        Timer_pruefen(&ZeitTemp);
      }
    }

    if (AP) dnsServer.processNextRequest();
    server.handleClient();              // Server Ereignisse abarbeiten
  }
  readInput();
}




