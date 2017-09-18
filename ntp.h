#include "common.h";
#include <Time.h>
#include <WiFiUdp.h>
#define TIMEZONE 1
#define SUMMERTIME 1

IPAddress timeServerIP;
const char* ntpServerName = "time.nist.gov";

const int NTP_PACKET_SIZE = 48;
byte packetBuffer[ NTP_PACKET_SIZE];
WiFiUDP udp;

int berechne_Ostern();

boolean summertime(int year, byte month, byte day, byte hour, byte tzHours)
// European Daylight Savings Time calculation by "jurs" for German Arduino Forum
// input parameters: "normal time" for year, month, day, hour and tzHours (0=UTC, 1=MEZ)
// return value: returns true during Daylight Saving Time, false otherwise
{
  if (month < 3 || month > 10) return false; // keine Sommerzeit in Jan, Feb, Nov, Dez
  if (month > 3 && month < 10) return true; // Sommerzeit in Apr, Mai, Jun, Jul, Aug, Sep
  if (month == 3 && (hour + 24 * day) >= (1 + tzHours + 24 * (31 - (5 * year / 4 + 4) % 7)) || month == 10 && (hour + 24 * day) < (1 + tzHours + 24 * (31 - (5 * year / 4 + 1) % 7)))
    return true;
  else
    return false;
}

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

/**
   NTP senden und empfangen
*/
unsigned long GetNTP(void) {
  unsigned long ntp_time = 0;
  udp.begin(2390);
  WiFi.hostByName(ntpServerName, timeServerIP);
  sendNTPpacket(timeServerIP);
  int cb = udp.parsePacket();
  delay(500);
  int timout1 = 0;
  while (cb  < NTP_PACKET_SIZE)
  {
    timout1++;
    if  (timout1 > 10) return 0;                  // 5s Timout
    cb = udp.parsePacket();
    delay(500);
  }
  Serial.print("packet received, length=");
  Serial.println(cb);
  // We've received a packet, read the data from it
  udp.read(packetBuffer, NTP_PACKET_SIZE);
  //the timestamp starts at byte 40 of the received packet and is four bytes,
  // or two words, long. First, esxtract the two words:
  unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
  unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
  // combine the four bytes (two words) into a long integer
  // this is NTP time (seconds since Jan 1 1900):
  unsigned long secsSince1900 = highWord << 16 | lowWord;
  // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
  const unsigned long seventyYears = 2208988800UL;
  // subtract seventy years:
  unsigned long epoch = secsSince1900 - seventyYears;
  // local time from UTC
  ntp_time = epoch + TIMEZONE * 3600;
  Serial.print("Unix local time = ");
  Serial.println(ntp_time);
  if (SUMMERTIME && summertime(year(ntp_time), month(ntp_time), day(ntp_time),  hour(ntp_time), TIMEZONE)) {
    ntp_time += 3600;
    sommerzeit = true;
  } else {
    sommerzeit = false;
  }
  return ntp_time;
}

String PrintTime (unsigned long epoch)
{
  String TimeString = "";
  if ( (epoch  % 86400L) / 3600 < 10 )TimeString += "0";
  TimeString += String((epoch  % 86400L) / 3600); // Stunden
  TimeString += ":";
  if ( ((epoch % 3600) / 60) < 10 )TimeString += "0";
  TimeString += String((epoch  % 3600) / 60); // Minuten
  TimeString += ":";
  if ( (epoch % 60) < 10 )TimeString += "0";
  TimeString += String(epoch % 60); // Sekunden
  return  TimeString;
}

#define LEAP_YEAR(Y)     ( ((1970+Y)>0) && !((1970+Y)%4) && ( ((1970+Y)%100) || !((1970+Y)%400) ) )
static const uint8_t monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; // API starts months from 1, this array starts from 0

String PrintDate (unsigned long epoch)
{
  String DateString = "";
  uint8_t year;
  uint8_t month, monthLength;
  uint32_t time;
  unsigned long days;
  time = (uint32_t)epoch;
  time /= 60; // now it is minutes
  time /= 60; // now it is hours
  time /= 24; // now it is days
  year = 0;
  days = 0;
  while ((unsigned)(days += (LEAP_YEAR(year) ? 366 : 365)) <= time) year++;
  days -= LEAP_YEAR(year) ? 366 : 365;
  time  -= days; // now it is days in this year, starting at 0
  days = 0;
  month = 0;
  monthLength = 0;
  for (month = 0; month < 12; month++)
  {
    if (month == 1)
    { // february
      if (LEAP_YEAR(year)) monthLength = 29;
      else  monthLength = 28;
    } else  monthLength = monthDays[month];


    if (time >= monthLength)time -= monthLength;
    else break;

  }
  if ((time + 1) < 10)DateString += "0";
  DateString += String(time + 1); // day of month
  DateString += ".";
  if ((month + 1) < 10)DateString += "0";
  DateString += String(month + 1);  // jan is month 1
  DateString += ".";
  //DateString += String(year + 1970); // year is offset from 1970
  DateString += String(year - 30); // year is offset from 1970
  return DateString;
}

boolean sommerzeitTest() {
  if (SUMMERTIME) {
    time_t jetzt = now();
    return (summertime(year(jetzt), month(jetzt), day(jetzt),  hour(jetzt), TIMEZONE) ? !sommerzeit : sommerzeit);
  }
  return false;
}

boolean feiertag(time_t test)
{
  String testDate;
  int tag;
  int monat = 3;
  int ostersonntag;

  testDate = String(day(test)) + "." + String(month(test));

  // Zuerst die festen Feiertage
  // Hinweis: 24.12 und 31.12 sind eigentlich keine Feiertage, werden aber hier als solche behandelt
  if (testDate == ("1.1")) {
    return true; // Neujahr
  }
  if (testDate == ("1.5")) {
    return true; // Maifeiertag
  }
  if (testDate == ("3.10")) {
    return true; // Tag d. dt. Einheit
  }
  if (testDate == ("24.12")) {
    return true; // Heiligabend (eigentlich kein Feiertag)
  }
  if (testDate == ("25.12")) {
    return true; // Erster Weihnachtsfeirtag
  }
  if (testDate == ("26.12")) {
    return true; // Zweiter Weihnachsfeiertag
  }
  if (testDate == ("31.12")) {
    return true; // Silvester (eigentlich kein Feiertag)
  }

  // Nachfolgend einige Feiertage die nicht bundeseinheitlich sind: (ggf. auskommentieren)
  // if (testDate==("6.1")) {return true;}   // Heilige Drei Könige (nur in best. Bundesländern)
  // if (testDate==("15.8")) {return true;}  // Mariae Himmelfahrt (nur im Saarland)
  if (testDate == ("31.10")) {
    return year() == 2017; // Reformationstag (nur in best. Bundesländern) und in 2017
  }
  // if (testDate==("1.11")) {return true;}  // Allerheiligen (nur in best. Bundesländern)



  if (month() == 3) {
    tag = day(); // Wenn März, aktuellen Tag ermitteln
  }
  if (month() == 4) {
    tag = day() + 31;
  }
  if (month() == 5) {
    tag = day() + 31 + 30;
  }
  if (month() == 6) {
    tag = day() + 31 + 30 + 31;
  }

  ostersonntag = berechne_Ostern();
  if (ostersonntag == tag)  {
    return true; // Ostersonntag
  }
  if (ostersonntag - 2 == tag)  {
    return true; // Karfreitag
  }
  if (ostersonntag + 1 == tag)  {
    return true; // Ostermontag
  }
  if (ostersonntag + 39 == tag)  {
    return true; // Christi Himmelfahrt
  }
  if (ostersonntag + 49 == tag)  {
    return true; // Pfingstsonntag
  }
  if (ostersonntag + 50 == tag)  {
    return true; // Pfingstmontag
  }
  if (ostersonntag + 60 == tag)  {
    return true; // Fronleichnam (nicht bundeseinheitlich)
  }

  // Buss- und Bettag gibt es nur in eingen Bundeländern, also ggf. auskommentieren
  // if (buss_und_bettag()) {return true;}

  return false;
}


int berechne_Ostern()
{
  // nach der Gauß-Formel
  // Rückgabewert: Datum von Ostern ab 1. März (max=56 für 25. April)

  int a;
  int b;
  int c;
  int d;
  int e;
  int f;
  int k;
  int M;
  int N;
  int p;
  int q;
  int jahr = year();


  // Die "magische" Gauss-Formel anwenden:
  a = jahr % 19;
  b = jahr % 4;
  c = jahr % 7;
  k = jahr / 100;
  p = (8 * k + 13) / 25;
  q = k / 4;
  M = (15 + k - p - q) % 30;
  N = (4 + k - q) % 7;
  d = (19 * a + M) % 30;
  e = (2 * b + 4 * c + 6 * d + N) % 7;
  f = 22 + d + e; // Tag auf Maerz bezogen: 32 bedeutet Erster April usw.


  if (f == 57) {
    f = 50; // Wenn der 26. April ermittelt wird (31+26=57), dann muss Ostern am 19.4. sein (19+31=50)
  }
  /// Falls der 25. April ermittelt wird, gilt dies nur wenn d=28 und a>10
  if ((f == 56) && (d == 28) && (a > 10))  {
    f = 49;
  }
  return f;

}

boolean buss_und_bettag()
{
  // Buss- und Bettag fällt auf den Mittwoch zwischen demm 16. und 22. November
  if ( (month() == 11) && (day() > 15) && (day () < 23) )
  {
    if (weekday() == 4) {
      return true;  // Wenn heute Mittwoch ist
    }
  }

  return false;

}
