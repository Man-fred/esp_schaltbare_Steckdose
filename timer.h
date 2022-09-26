#include "zcommon.h"

#define TIMER_MAX 50
extern byte is_authentified();

struct Timer_struct {
  byte art;                 // Gültigkeit des Timers, 0: gelöscht bzw. leerer Eintrag
  unsigned long zeit;       // eingestellter Zeitpunkt
  unsigned long letzter;    // letzte Durchführung
  unsigned long naechster;  // nächste Durchführung
  byte relais;              // Relais 1-4
  byte ein;                 // 1: einschalten, 0: ausschalten
  boolean aktiv;            // 1: aktiv, 0: inaktiv, aber nicht löschen
  byte nachholen;           // nach Stromausfall: 0: nicht nachholen, 1: nachholen
};

struct TimerIdx_struct {
  byte id;
  unsigned long  naechster;
};

Timer_struct timer[TIMER_MAX];
TimerIdx_struct timerIdx[TIMER_MAX + 1];
int timerIdxMax = 0;

void TimerNaechster(int i, bool neustart);
void Relais_Schalten(int datensatz, byte ein, String logtext);

void Ereignis_Timer_JSON()
{
  if (is_authentified()) {
    byte anzahl = 0;
    String json = "[";
    for (int j = 0; j < timerIdxMax; j++)        // Deaktivierte Timer Suchen
    {
      i = timerIdx[j].id;
      if (i >= 0 && i < TIMER_MAX && timer[i].art)
      {
        if (anzahl > 0)
          json += ",";
        anzahl++;
        json += "{\"id\":\""+String(i+1)+"\",\"art\":\""+String(timer[i].art)+"\",";
        json +=  "\"date\":\""+ (timer[i].zeit > 1000000000 ? PrintDate(timer[i].zeit) : "") +"\"," ;
        json +=  "\"time\":\""+PrintTime(timer[i].zeit)+"\",";
        json +=  "\"date_n\":\""+PrintDate(timer[i].naechster)+"&nbsp;"+PrintTime(timer[i].naechster)+"/ ";
        if (timer[i].letzter == 0)
          json += "-\",";
        else
          json += PrintDate(timer[i].letzter)+"&nbsp;"+PrintTime(timer[i].letzter)+"\",";
          
        json +=  "\"time_l\":\""+PrintTime(timer[i].letzter)+"\",";
        json +=  "\"relais\":\""+String(timer[i].relais)+"\",";
        json +=  "\"ein\":\""+String(timer[i].ein)+"\",";
        json +=  "\"nachholen\":\""+String(timer[i].nachholen)+"\",";
        json +=  "\"aktiv\":\""+String(timer[i].aktiv)+"\"}";
      }
      Serial.println(String(j)+"->"+String(i));
    }
    if (anzahl > 0)
      json += ",";
    anzahl++;
    json += "{\"id\":\"Z\",";
    json +=  "\"date\":\""+ PrintDate(now()) +"\"," ;
    json +=  "\"time\":\""+PrintTime(now())+"\"}";
    json += "]";
    server.sendHeader("Cache-Control", "no-cache");
    //server.setContentLength(json.length());
    //server.send(200, "application/x-www-form-urlencoded", ""); // Antwort an Internet Browser
    //server.sendContent(json);
    //server.send(200, "application/x-www-form-urlencoded", json); // Antwort an Internet Browser
    server.send(200, "text/plain", json); // Antwort an Internet Browser
  }
}

void Route_Timers_Zeigen()
{
  server.sendHeader("Location", "/index.htm#page1");
  server.send(303, "text/html", ""); // Antwort an Internet Browser
}

void TimerNr_speichern(int datensatz, boolean nurIndex = false)
{
  if (!nurIndex) {
    File DataFile = SPIFFS.open("/timern.dat", "r+");
    DataFile.seek(sizeof(timer[0])*datensatz, SeekSet);
    DataFile.write(reinterpret_cast<uint8_t*>(&timer[datensatz]), sizeof(timer[0]));
    DataFile.close();
  }

  // Index aktualisieren
  byte positionAlt = timerIdxMax;
  unsigned long naechsterAlt = 0;
  byte positionNeu = 0;
  if (timerIdxMax > 0) {
    for (int j = timerIdxMax-1; j >= 0; j--) {
      // im Index vorhanden, hier aber eventuell falsche Position
      if (timerIdx[j].id == datensatz) {
        positionAlt = j;
        naechsterAlt = timerIdx[j].naechster;
        break;
      }
    }
  }
  if (positionAlt == timerIdxMax) {
    timerIdxMax++;
    naechsterAlt = 4294967295; // max. unsigned long
    timerIdx[timerIdxMax-1].naechster = naechsterAlt;
  }
  Serial.println("Alt/Max: " + String(positionAlt)+"/"+String(timerIdxMax) );
  if (naechsterAlt > timer[datensatz].naechster && timer[datensatz].art > 0) {
  Serial.println("Kleiner: " + String(timer[datensatz].naechster)+"<"+String(naechsterAlt) );
    positionNeu = timerIdxMax-1;
    for (i = positionAlt-1; i >= 0; i--) {
      // alle größeren im Index verschieben
      if (timer[datensatz].naechster <= timerIdx[i].naechster) {
        timerIdx[i + 1].naechster = timerIdx[i].naechster;
        timerIdx[i + 1].id = timerIdx[i].id;
        // merken, wo eigefügt werden soll
        positionNeu = i;
      }
    }
  } else if (naechsterAlt < timer[datensatz].naechster || timer[datensatz].art == 0) {
  Serial.println("Groesser: " + String(timer[datensatz].naechster)+">"+String(naechsterAlt) );
    for (i = positionAlt; i < timerIdxMax-1; i++) {
      // alle kleineren im Index verschieben
      if (timer[datensatz].naechster >= timerIdx[i+1].naechster || timer[datensatz].art == 0) {
        Serial.println(String(i+1)+" -> "+String(i));
        timerIdx[i].naechster = timerIdx[i+1].naechster;
        timerIdx[i].id = timerIdx[i+1].id;
        // merken, wo eigefügt werden soll
        positionNeu = i+1;
      }
    }
  } else {
    positionNeu = positionAlt;
  Serial.println("Gleich: " + String(timer[datensatz].naechster)+"="+String(naechsterAlt) );
  }
  if (timer[datensatz].art > 0) {
    timerIdx[positionNeu].naechster = timer[datensatz].naechster;
    timerIdx[positionNeu].id = datensatz;
  } else {
    timerIdxMax--;
  }
  Serial.println("Speichere Timer Nr.: " + String(datensatz)+", Pos "+String(positionNeu) );
}

void TimerNaechster(int i, bool neustart = false)
{
  // aktuelle Zeit einfrieren
  unsigned long Jetzt = now();
  // aktueller Tag, 00:00 Uhr
  unsigned long Zeit = Jetzt - (Jetzt % 86400L);

  // Timer nur setzen, wenn nicht gelöscht und durchgeführt
  if (timer[i].art && timer[i].aktiv && /*timer[i].naechster <= Jetzt && */ timer[i].naechster == timer[i].letzter)
  { //-------------------------- Timer aktiv --------------------------------
    if (timer[i].art == 1) //Timer einmalig
    {
      timer[i].naechster = timer[i].zeit;
    } //--------------------Ende Timerfunktion Einmalig ----------------------------------------------------

    else if (timer[i].art > 1 && !(neustart && timer[i].nachholen) )  
    { //-------------------- Timerfunktion Täglich und sonstige wiederholte --------------------------------
      //letzter gerade ausgeführt, neue Zeit für morgen eintragen
      timer[i].naechster = (timer[i].zeit % 86400L) + Zeit;
      Serial.println("Nächster heute ("+String(weekday())+") "+PrintDate(timer[i].naechster)+" "+PrintTime(timer[i].naechster));
      
      // Tägliche Timer
      if  (timer[i].art == 2)      
      { // täglicher Timer vorbei, morgen setzen
        if (timer[i].naechster < Jetzt)
          timer[i].naechster += 86400L;
        Serial.print("Nächster t ");
      }
      // einzelner Wochentag Timer
      if  (timer[i].art > 2 && timer[i].art < 10)  
      { 
        if (timer[i].art - 2 - weekday() == 0) {
          if (timer[i].naechster < Jetzt)
            timer[i].naechster += 7 * 86400L;
        } else if (timer[i].art - 2 - weekday() < 0) {
          timer[i].naechster += (7 + timer[i].art - 2 - weekday()) * 86400L;
        } else {
          timer[i].naechster += (timer[i].art - 2 - weekday()) * 86400L;
        }
        Serial.print("Nächster w ");
      }
      // Wochenende/Feiertag Timer
      if  (timer[i].art == 10) { 
        while (timer[i].naechster < Jetzt || 
                 (weekday(timer[i].naechster) != 7 && 
                  weekday(timer[i].naechster) != 1 &&
                  !feiertag(timer[i].naechster))) {
          timer[i].naechster += 86400L;
        }
        Serial.print("Nächster we ");
      }
      // Werktag Timer
      if  (timer[i].art == 11) { 
        while (timer[i].naechster < Jetzt || 
                  weekday(timer[i].naechster) == 7 || 
                  weekday(timer[i].naechster) == 1 ||
                  feiertag(timer[i].naechster)) {
          timer[i].naechster += 86400L;
        }
        Serial.print("Nächster wt ");
      }
      if  (timer[i].art == 12) { // Feiertage Timer
        while (timer[i].naechster < Jetzt || !feiertag(timer[i].naechster)) {
          // nächsten Feiertag suchen
          timer[i].naechster += 86400L;
          ESP.wdtEnable(0);
        }
        Serial.print("Nächster f ");
      }
      Serial.println(PrintDate(timer[i].naechster)+" "+PrintTime(timer[i].naechster));
    } // ende Tageszeit Überinstimmung
  }   // ende Timer aktiv
}

void Timer_Laden(bool neustart = false)
{
  if (SPIFFS.exists("/timern.dat")) // Timern aus Datei laden
  {
    File DataFile = SPIFFS.open("/timern.dat", "r");
    DataFile.read(reinterpret_cast<uint8_t*>(&timer), sizeof(timer));
    DataFile.close();
  }  else {
    Serial.println("Datei timern.dat fehlt");
    File DataFile = SPIFFS.open("/timern.dat", "w");
    DataFile.write(reinterpret_cast<uint8_t*>(&timer), sizeof(timer));
    DataFile.close();
    Serial.println("Datei timern.dat erzeugt");
  }
  // Initialisierung nach Start
  timerIdxMax = 0;
  for (int k = 0; k < TIMER_MAX; k++) {
    if (timer[k].art > 0) {
      TimerNaechster(k, neustart);
      TimerNr_speichern(k, true);
    }
  }
}

void Ereignis_Timer_Laden()
{
  if (is_authentified()) {
    Timer_Laden();
    Route_Timers_Zeigen();
  }
}

void Ereignis_DeleteTimer()
{
  if (is_authentified()) {
    if (server.arg("Nr") != "")
    {
      String StrNr = server.arg("Nr");         // Relais Nr. empfangen
      int Nr = StrNr.toInt();
      if (Nr > 0 && Nr <= TIMER_MAX) {
        Nr--;
        timer[Nr].art = 0;
        TimerNr_speichern(Nr);
      }
    }
    Route_Timers_Zeigen();
  }
}

void Timer_pruefen()
{
  for (i = 0; i < 50; i++)        // alle timer-einträge durchgehen
  {
    if (timer[i].art && timer[i].naechster < (unsigned long)now() && timer[i].letzter < timer[i].naechster)
    { // aktiv, noch nicht durchgeführt
      if (timer[i].aktiv)
        Relais_Schalten(timer[i].relais - 1, timer[i].ein, "Timer "+String(i));
      timer[i].letzter = timer[i].naechster;
      if (timer[i].art == 1)
        timer[i].aktiv = false; // Timer Deaktivieren
      TimerNaechster(i);
      TimerNr_speichern(i);
    } //--------------------Ende Timerfunktion  ----------------------------------------------------
  }     // ende alle timer-einträge durchgehen
}


void Ereignis_NeueTimer()
{
  if (is_authentified()) {
    //art=2&zeit=1480790953&relais=1&ein=0&id=18
    if (server.arg("art") != "")
    {
      String StrArt = server.arg("art");         // Relais Nr. empfangen
      String StrZeit = server.arg("zeit");         // Relais Nr. empfangen
      String StrRelais = server.arg("relais");         // Relais Nr. empfangen
      String StrEin = server.arg("ein");
      String StrAktiv = server.arg("aktiv");
      String StrNachholen = server.arg("nachholen");
      String StrId = server.arg("id");
      i = StrId.toInt();
      if (i == 0)
      {
        for (i = 0; i < 50; i++)        // Deaktivirte Timer Suchen
        {
          if (!timer[i].art)  break;        // Wenn Timer nich aktiv
        }
      } else {
        i--;
      }
      Serial.println(i);
      if (i >= 0 && i < TIMER_MAX) {
        timer[i].art = StrArt.toInt();
        timer[i].zeit = StrZeit.toInt();
        timer[i].naechster = 0;
        timer[i].letzter = 0;
        timer[i].relais = StrRelais.toInt();
        timer[i].ein = StrEin.toInt();
        timer[i].aktiv = (StrAktiv == "true" ? 1 : 0);
        timer[i].nachholen = (StrNachholen == "true" ? 1 : 0);
        Serial.println("StrAktiv "+StrAktiv+":"+String(timer[i].aktiv));
        TimerNaechster(i);
        TimerNr_speichern(i);
      }
    }
    
    Route_Timers_Zeigen();
  }
}
