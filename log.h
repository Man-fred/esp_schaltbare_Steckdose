
// if (SPIFFS.remove("/Daten.txt") ) Serial.println("Datei Daten.txt gelöscht");

void DateiZuGross()
{
  File DataFile = SPIFFS.open("/log.txt", "r");
  if (DataFile.size() > 30000) // Wenn Dateigrösse 30Kb überschreitet
  {
    DataFile.seek(-10000, SeekEnd); // Letzte 10Kb in neue Datei übernehmen
    DataFile.readStringUntil('\r'); // Zeiger auf Zeilenanfang plazieren
    
    File DataFile1 = SPIFFS.open("/logNeu.txt", "w");
    Serial.println("logNeu.txt Erzeugt");
    while (DataFile.available()) DataFile1.print(DataFile.readStringUntil('\n')); // Vom Zeiger bis Ende "/log.txt" in "/logNeu.txt" kopieren
    DataFile.close();
    DataFile1.close();
    if (SPIFFS.remove("/log.txt") ) Serial.println("Datei log.txt gelöscht");
    if (SPIFFS.rename("/logNeu.txt", "/log.txt") ) Serial.println("Datei logNeu.txt zu log.txt umbenant");
    return;
  }
  DataFile.close();
}

boolean LogSchreiben(String Daten)
{
  if (!SPIFFS.exists("/log.txt"))
  {
    File DataFile = SPIFFS.open("/log.txt", "w");
    DataFile.println(Daten);
    DataFile.close();
    return true;
  }
  DateiZuGross(); // Überprüfen ob die log.txt nicht zu gross wird
  File DataFile = SPIFFS.open("/log.txt", "a");
  DataFile.println(Daten); // + "\r\n"
  DataFile.close();
  return true;
}




