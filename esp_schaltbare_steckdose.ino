#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <FS.h>
#include "common.h";

#include "fs.h";
#include "log.h"
#include "ntp.h"
#include <TimeLib.h>

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
/// #define DNS
byte Taster[4] = {D2, D1, D4, D3}; //2,0,4,5
byte Relay[4] = {D5, D6, D7, D8}; //13,12,14,16

IPAddress apIP(192, 168, 168, 30); // wenn in AP-Mode

#if defined DNS
#include <DNSServer.h>
const byte DNS_PORT = 53;
DNSServer dnsServer;
#endif

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
//12-15: Wechselschalter: "!": Strom fließt bei 1 (Relais ein), "0": Strom fließt bei 0 (Relais aus)
byte val[16] = {0, 0, 0, 0, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1};
// Zustand der Taster
byte TasterStatus[4];
unsigned long TasterZeit[4];
String Temp = "";
boolean AP = 0; // Acsespoint Modus aus
boolean WLAN_Fehlt = 1;
unsigned long NTPTime = 0, ZeitTemp = 0;
int timout = 0; //


#include "timer.h"

void Zeit_Einstellen()
{
  if (!WLAN_Fehlt) {
    NTPTime = GetNTP();
    timout = 0;
    while (NTPTime == 0 )
    {
      timout++;
      if  (timout > 10)  break;
      NTPTime = GetNTP();
    }
    setTime(NTPTime);
    Serial.print("NTP Time: ");
    Serial.println(PrintTime(NTPTime));
    if (!summertime( year(), month(), day(),  hour(), 0))  adjustTime(-60 * 60);
    Serial.print("Zeit nach Sommer- Winterzeitanpassung: ");
    Serial.println( PrintTime(now()) );
  }
}

void WlanStation()
{
  if (WLAN_Fehlt) {
    WiFi.begin(ssid, passwort);
    timout = 0;
    Serial.print("Status ");
    Serial.println(WiFi.status());
    while (WiFi.status() != WL_CONNECTED && !WiFi.localIP())
    {
      delay(500);
      Serial.print("O");
      timout++;
      if  (timout > 10) // Wenn Anmeldung nicht möglich
      {
        Serial.println("");
        Serial.println("Wlan verbindung fehlt");
        WLAN_Fehlt = 1;
        break;
      }
    }
    Serial.print("WL_Status ");
    Serial.println(WiFi.status());
    WiFi.printDiag(Serial);
    //if (WiFi.status() == WL_CONNECTED)
    {
      WLAN_Fehlt = 0;
      Serial.println("");
      Serial.println("Mit Wlan verbunden");
      Serial.print("IP Adresse: ");
      Serial.println(WiFi.localIP());
      Zeit_Einstellen();
    }
  }
  readInput();
}

void Relais_Init()
{
  if (SPIFFS.exists("/relais.dat")) // Timern aus Datei laden
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
    pinMode(Relay[k], OUTPUT);
    pinMode(Taster[k], INPUT_PULLUP);
    if (val[k + 4] < 2) {
      val[k] = val[k + 4];
    }
    Relais_Schalten(k, val[k], "Server gestartet");
  }
}

void readInput() {
  char inser = Serial.read();
  if (inser == 'e') {
    Einstellen();
  } else if (false && inser == 'f') {
    Serial.println("Please wait 30 secs for SPIFFS to be formatted");
    SPIFFS.format();
    Serial.println("Spiffs formatted");
    //See more at: http://www.esp8266.com/viewtopic.php?f=29&t=8194#sthash.mj02URAZ.dpuf
    SPIFFS.info(fs_info);
    Serial.println("totalBytes " + String(fs_info.totalBytes));
  } else if (inser == 'i') {
    Serial.println("");
    Serial.println("Mit Wlan verbunden");
    Serial.print("IP Adresse: ");
    Serial.println(WiFi.localIP());
    Serial.println("Zeit: " + PrintDate(now()) + " " + PrintTime(now()));
  }
}

void setup()                // Wird 1 Mal beim Start ausgefuehrt
{
  char inser;               // Serielle daten ablegen
  String nachricht = "";    //  Setup Formular

  EEPROM.begin(250);                                 // EEPROM initialisieren mit 200 Byts
  if (!SPIFFS.begin()) Serial.println("Failed to mount file system");

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  while (Serial.available())
    inser = Serial.read();
  Serial.println("Start, Einstellen: e+<Enter>, Format SPIFFS: f+<Enter>");
  delay(20000);

  readInput();

  Serial.println("Weiter");
  Relais_Init();
  z = 0;
  LeseEeprom(ssid, sizeof(ssid));        // EEPROM lesen
  if (ssid[0] == 255) // Wenn EEPROM leer ist dann:
  {
    for (i = 0; i < 6; i++) EEPROM.write(i, '\0');
    EEPROM.commit();
    Einstellen();
  }

  LeseEeprom(passwort, sizeof(passwort));
  LeseEeprom(AdminPasswort, LOGINLAENGE);
  LeseEeprom(AdminName, LOGINLAENGE);
  LeseEeprom(UserPasswort, LOGINLAENGE);
  LeseEeprom(UserName, LOGINLAENGE);

  Serial.println("Freies RAM = " + String(system_get_free_heap_size()));

  if (ssid[0] == '\0')  //Wenn ssid leer dann Access Point Modus starten
  {
    Serial.println("Starte in Access Point modus" );
    Serial.println("IP http://192.168.168.30");
    Serial.print("SSID: WebSchalter, Passwort: ");
    Serial.println(passwort);
    WiFi.mode(WIFI_AP);      // access point modus
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    strncpy(ssid, "WebSchalter\0", 13);
    WiFi.softAP(&ssid[0], &passwort[0]);  // Name des Wi-Fi netzes
#if defined DNS
    dnsServer.start(DNS_PORT, "*", apIP);
#endif
    AP = 1;
  }
  else // Wenn ssid angegeben dann in Stationmodus mit Router verbinden
  {
    Serial.print("Verbinde mit ");
    Serial.println(ssid);
    WiFi.setPhyMode(WIFI_PHY_MODE_11G);
    WiFi.setOutputPower(2.5);
    WiFi.mode(WIFI_STA);
    WlanStation();
  } // ENDE Stationmodus / Access Point modus Auswahl

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

  Temp = PrintDate(now()) + "   " + PrintTime (now()) + "   Server gestartet";
  LogSchreiben(Temp);

  httpStart();
  // gespeicherte Timer laden und Reihenfolge setzen, hier mit Argument neustart=true
  Timer_Laden(true);
}

void Einstellen()
{
  Serial.println("Einstellen");
  EEPROM.begin(250);                                 // EEPROM initialisieren mit 200 Byts
  z = 0;                        // EEPROM Star den Datensatzes

  LeseEeprom(ssid, sizeof(ssid));
  LeseEeprom(passwort, sizeof(passwort));
  LeseEeprom(AdminPasswort, LOGINLAENGE);
  LeseEeprom(AdminName, LOGINLAENGE);
  LeseEeprom(UserPasswort, LOGINLAENGE);
  LeseEeprom(UserName, LOGINLAENGE);
  //LeseEepromStr(&nachricht,100);
  /*
    Serial.println(ssid);
    Serial.println(passwort);
    Serial.println(url);
    Serial.println(nachricht);
  */
  boolean endlos = 1;
  WiFi.mode(WIFI_AP);      // access point modus
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("Web Schalter Setup", "tiramisu");  // Name des Wi-Fi netzes
#if defined DNS
  dnsServer.start(DNS_PORT, "*", apIP);
#endif
  delay(500);         // Abwarten 0,5s
  httpStart();
  server.onNotFound ( handleNotFound );
  while (endlos)
  {
#if defined DNS
    dnsServer.processNextRequest();
#endif
    server.handleClient();

  }
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

  //list directory
  server.on("/list", HTTP_GET, handleFileList);

  server.on("/upload", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/html", serverIndex);
  });
  server.on("/upload",      HTTP_POST, []() {
    server.send(200, "text/html", serverIndex);
  }, handleFileUpload);
  server.on("/upload.json", HTTP_POST, []() {
    server.send(200, "text/json", "{\"ok\":1}");
  }, handleFileUpload);
  server.on("/delete",      HTTP_GET, handleFileDelete);

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
  Serial.println("Login "+String(UserNext)+String(UserStatus[UserNext]));
  return UserNext;
}

//Check if header is present and correct
byte is_authentified() {
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
  digitalWrite(Relay[datensatz], val[datensatz + 8] ^ !val[datensatz + 12] ^ val[datensatz]);
  Serial.print(Temp);
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
    Antwort += sketchVersion;
    Antwort += ";" + String(esp.getChipId()) + ";";
    Antwort += (UserStatus[nr] == COOKIE_ADMINISTRATOR ? "Administrator" : "Eingeschränkt");
  
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
    temp +=  "\"pass2\":\"" + String(AdminPasswort) + "\",";
    temp +=  "\"pass3\":\"" + String(UserPasswort) + "\"";
    for (int k = 4; k < 16; k++) {
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
    for (int k = 4; k < 16; k++) {
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
      SchreibeEeprom (passwort);               // wenn mit * dann geladene pass abspeichern
    }
    server.arg("AdminPasswort").toCharArray(AdminPasswort, server.arg("AdminPasswort").length() + 1) ;
    SchreibeEeprom (AdminPasswort);
    SchreibeEeprom(AdminName);
    server.arg("UserPasswort").toCharArray(UserPasswort, server.arg("UserPasswort").length() + 1) ;
    SchreibeEeprom(UserPasswort);
    SchreibeEeprom(UserName);
    EEPROM.commit();
  
    ConfigRoute();
  }
}

void loop()
{
#if defined DNS
  if (AP)dnsServer.processNextRequest();
#endif
  for (int k = 0; k < 4; k++) {
    // durch Pullup ist Standard 1, gedrückt 0
    int TasterTemp = !digitalRead(Taster[k]);
    // TasterTemp Standard 0, gedrückt 1
    if (TasterTemp != TasterStatus[k] && millis() - TasterZeit[k] > 500)
    {
      Serial.print(k);
      TasterZeit[k] = millis();
      TasterStatus[k] = TasterTemp;
      if (TasterTemp)
      {
        val[k] = !val[k];         Relais_Schalten(k, val[k], "Taster");
      }
    }
  }

  if (now() != ZeitTemp)       // Ausführung 1 mal in der Sekunde
  {
    ZeitTemp = now();
    Timer_pruefen(&ZeitTemp); // Timer auch wenn offline!!
  }                            
  /*if ( (NTPTime + 60) < ZeitTemp ) //WLan reconnect
    {
    if (WLAN_Fehlt) WlanStation();
    }*/
  if ( (NTPTime + 86400) < ZeitTemp ) //Zeit Update alle 24Stunden
  {
    //if (WiFi.status() == WL_CONNECTED) // Zeit neu einstellen
    {
      Zeit_Einstellen();
    }
  }

  readInput();
  server.handleClient();              // Server Ereignisse abarbeiten
}




