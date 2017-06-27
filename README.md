# esp_schaltbare_Steckdose
esp8266-basierte schaltbare WLAN-Steckdosensteuerung mit Timerfunktion

## Umgebung
### Software
Arduino 1.8.2 mit zusätzlicher Boardverwalter-URL http://arduino.esp8266.com/stable/package_esp8266com_index.json .
### Hardware
nodemcu 0.9 bzw wemos mini D1, 4-fach Relaisplatine und 4-fach Taster

## Funktion
### Start als AP
Wenn kein WLan-Netz angegeben ist wird der esp8266 als Accesspoint gestartet mit SSID WebSchalter, ohne Passwort. Zur ersten Konfiguration verbindet man sich mit diesem Netz und startet im Webbrowser http://192.168.168.30 .
### Start als Station
Mit konfiguriertem WLan wählt sich der esp8266 in das vorhandene Netzwerk ein und kann über Webbrowser konfiguriert werden.

Die 4 Relais können unabhängig voneinander über bis zu 50 Timereinstellungen geschaltet werden. Dazu liest der esp8266 die aktuelle Zeit (MEZ mit deutscher Sommer-/Winterzeitumstellung) vom Internet. Zusätzlich kann jedes Relais mit Taster umgeschaltet werden.

### Timerfunktion
Einstellungen sind sekundengenau täglich, für einzelne Wochentage, Wochenende möglich. Feiertagsberücksichtigung ist vorbereitet, aber noch ohne Funktion. Der Timer läuft auch ohne WLan-Verbindung weiter, benötigt aber für eine korrekte Uhrzeit eine regelmäßige Internetverbindung.

### Sicherheit
Der esp8266 lässt nur unverschlüsselte Verbindungen zu. Eingaben können aber über Passwort gesichert werden.
