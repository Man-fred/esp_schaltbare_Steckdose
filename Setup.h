String Mitteilung = "";


unsigned int hexToDec(String hexString) {

  unsigned int decValue = 0;
  int nextInt;

  for (int i = 0; i < hexString.length(); i++) {

    nextInt = int(hexString.charAt(i));
    if (nextInt >= 48 && nextInt <= 57) nextInt = map(nextInt, 48, 57, 0, 9);
    if (nextInt >= 65 && nextInt <= 70) nextInt = map(nextInt, 65, 70, 10, 15);
    if (nextInt >= 97 && nextInt <= 102) nextInt = map(nextInt, 97, 102, 10, 15);
    nextInt = constrain(nextInt, 0, 15);

    decValue = (decValue * 16) + nextInt;
  }

  return decValue;
}

void LeseEeprom(char *daten, int laenge)
{
  char a;
  do {
    a = EEPROM.read(z);
    *daten = a;
    daten++;
    z++;
    laenge--;
    if (!laenge)break;
  }
  while (a);
}

void LeseEepromStr(String *daten, int laenge)
{
  char a;
  *daten = "";
  do {
    a = EEPROM.read(z);
    *daten += String(a);
    z++;
    laenge--;
    if (!laenge)
      break;
  } while (a);
}

void SchreibeEeprom (String k)
{
  int i = 0, blja = 64;
  int  pos, wert;
  char Buchstabe;
  String HexZahl, ersatz = "f";
  k.replace("+", " ");

  while (blja--)
  {
    pos = k.indexOf('%');   // Kodierte zeichen enthalten
    if (pos < 0)break;
    HexZahl = k.substring(pos + 1, pos + 3); // Position finden
    wert = hexToDec(HexZahl);
    ersatz[0] = wert;               //Int to Chahr im String
    HexZahl = "%" + HexZahl;        //Suchmuster finden
    k.replace(HexZahl, ersatz);
  }

  while (k[i])
  {
    EEPROM.write(z, k[i]);
    z++;
    i++;
  }
  EEPROM.write(z, '\0');
  z++;
}
void SchreibeEepromCheck ()
{
  EEPROM.write(z, 0x55);
  Serial.print('Sreibe EEPROM-Pos ');
  Serial.println(z);
}
int LeseEepromCheck ()
{
  int check = 0;
  check = EEPROM.read(z);
  Serial.print('Lese EEPROM-Pos ');
  Serial.println(z);
  return check;
}





