/*
 * Eerste officiele versie van de 'Endenteller voor windmolens'.
 *
 * This code is written by Ronald Troost and is in the public domain.
 *
 * My thanks go out to various writers who taught me Arduino / C ++ via the internet
 */


/* ======================================================================================
 * De 3 kleuren LED heeft 4 pootjes, 1 lange, 2 korte en een die er tussin zit,
 * de lange poot is de min (ground of gnd), er naast zit rood, de middellange poot.
 * Aan de andere kant, is de eerste van de twee korte blauw en de laatste is groen. 
 * Dus op een rijtje is het rood-min-blauw-groen.
 * 
 * De aansluitingen op het Arduino Uno board:
 * 5 V(+) en GND(-) zijn de plus en min. Deze gaan naar VCC(+) en GND(-) van het LED Display. 
 * GND(-) gaat ook naar de beide reedswitches en naar de lange poot van de LED.
 * 
 * De andere zijde van de teller reedswitch gaat naar ingang 2 van het board.
 * De andere zijde van de reset reedswitch gaat naar ingang 4 van het board.
 * 
 * De uitgangen 5, 6, en 7 sturen de LED aan. Uitgang 5 gaat naar 'rood'.
 * Uitgang 6 gaat naar 'groen' en uitgang 7 gaat naar 'blauw'.
 * Analoog A4 gaat naar SDA van het LED Display en A5 naar SCL.
 * 
 * Het Arduino board kan het beste gevoed worden met een 9 Volt 1 Ampere adapter.
 * ====================================================================================== 
 */


#include <Wire.h>                                                        // Deze library maakt het mogelijk om met I2C / TWI aparaten te communiceren.
#include <EEPROMex.h>                                                    // Deze library maakt het mogelijk om te lezen en te schrijven naar de EEprom.
#include <LiquidCrystal_I2C.h>                                           // Library om de LiquidCrystal LCD display op een Arduino board aan te sluiten.

/*
 * kies 0x27 als je een PCF8574 van NXP heb,
 * of gebruik 0x3F als je een chip van Ti (Texas Instruments) heb PCF8574A
 */

LiquidCrystal_I2C lcd(0x27, 20, 4);                                      // instellen LCD display.

/*
 * het molensymbool is gemaakt met de generator aan op:
 * https://maxpromer.github.io/LCD-Character-Creator/
 */

byte mill[] = {                                                          // | creeert het molen karakter
  0x00,                                                                  // |
  0x11,                                                                  // |
  0x0A,                                                                  // |
  0x04,                                                                  // |
  0x0E,                                                                  // |
  0x15,                                                                  // |
  0x0E,                                                                  // |
  0x0E                                                                   // | creeert het molen karakter
};


int reedSchakTel = 2;                                                    // digital pin 2 reedswitch vooruit.
int reedSchakReset = 4;                                                  // digital pin 4 reedswitch reset.
int buttonAState = 1;                                                    // status reedschakelaar A.
int buttonAStateOld = 1;                                                 // vorige status reedschakelaar A.
int buttonResetState = 1;                                                // status resetschakelaar B.
int buttonResetStateOld = 1;                                             // vorige status resetschakelaar B.
int buttonSureState = 0;                                                 // status zekerweten resetschakelaar B.
int buttonSureStateOld = 0;                                              // vorige status zekerweten resetschakelaar B.
int LED_red = 5;                                                         // rode LED.
int LED_Forward = 6;                                                     // groene LED telpuls.
int LED_Still = 7;                                                       // blauwe LED die stilstand meld.
long rotationTimeA;                                                      // tijdstip van de laatste telpuls.
long rotationTimeAOld;                                                   // tijdstip van de vorige telpuls.
float rotationSpeed;                                                     // tijd waarin het spoorwiel een omwenteling maakt.
float numberOfSails;                                                     // aantal enden (wieken) per minuut.
float bw = 71.0;                                                         // aantal kammen / staven bovenwiel van uw molen. Schrijf de waarde met .0 er achter.
float sl = 37.0;                                                         // aantal kammen / staven schijfloop/wieg/bonkelaar van uw molen. Schrijf de waarde met .0 er achter.
float conversion = 60.0/(15.0*(bw/sl));                                  // berekende tijd voor een omwenteling van het spoorwiel bij 60 enden per minuut.
unsigned long numberOfRevolutions;                                       // aantal omwentelingen bovenas
unsigned long revolutionsSpurwheel = 1;                                  // totaal aantal omwentelingen per jaar, van het spoorwiel.
unsigned long bijwerkTijd = millis();                                    // na deze tijd worden de gegevens naar het geheugen geschreven.
unsigned int jaartal = 2019;                                             // variabele waarin het jaar vastgelegd wordt.
unsigned int opslagEEprom = 200;                                         // variabele waarin het aantal malen dat er opgeslagen is wordt vastgelegd.
int addresjaartal = 88;                                                  // adres in EEprom waar jaartal wordt gelezen en geschreven.
int addresomwenteling = 78;                                              // adres in EEprom waar omwentelingen spoorwiel wordt gelezen en geschreven. 
int addresOpslag = 68;                                                   // adres waarin aantal malen dat er opgeslagen is wordt bewaard. 


void wiekSnelheid();                                                     // Declare subroutine "wieksnelheid", telt het aantal enden (wieken).
void totaalOmw();                                                        // Declare subroutine "totaalOmw", telt het aantal omwentelingen van de bovenas.
void stilGevlucht();                                                     // Declare subroutine "stilGevlucht", geeft na X seconden aan dat de wieken stil staan.
void bijwerkEEprom();                                                    // Declare subroutine "bijwerkEEprom", hierin worden gegevens naar de EEprom geschreven.
void weetZekerDisplay();                                                 // Declare subroutine "weetZekerDisplay", weet je zeker dat je resetTotaalOmw wil uitvoeren. 
void resetTotaalOmw();                                                   // Declare subroutine "resetTotaalOmw", reset het aantal omwentelingen naar 0 en verhoogd het jaar met 1.



void setup()                                                             // the setup routine runs once when you start the Arduino.
{
  lcd.init();                                                            // initializeer het LCD display.
  lcd.backlight();                                                       // zet de backlight aan.
  lcd.createChar(1, mill);                                               // creeer een molen-character in memorycell 1.
  Serial.begin(9600);                                                    // initialize seriële communicatie op 9600 bits per seconde.
  pinMode(reedSchakTel, INPUT_PULLUP);                                   // initializeer digitale pin 2 als input.
  pinMode(reedSchakReset, INPUT_PULLUP);                                 // initializeer digitale pin 4 als input.
  pinMode(LED_red, OUTPUT);                                              // initializeer digitale pin 8 als output.
  pinMode(LED_Forward, OUTPUT);                                          // initializeer digitale pin 9 als output.
  pinMode(LED_Still, OUTPUT);                                            // initializeer digitale pin 10 als output.
  rotationTimeAOld = millis();                                           // rotationTimeAOld wordt op huidige tijd gezet. 
  revolutionsSpurwheel = EEPROM.readFloat(addresomwenteling);            // leest het aantal omwentelingen spoorwiel uit de EEprom.
  jaartal = EEPROM.readInt(addresjaartal);                               // leest de datum uit de EEprom. (deze twee strepen // VOOR deze regel typen om het jaartal terug naar 2019 te zetten) 
  opslagEEprom = EEPROM.readInt(addresOpslag);                           // leest het aantal malen in dat er opgeslagen is. (max 100.000 maal)
  lcd.setCursor(0, 1);                                                   // |Versie informatie gedurende 5 seconden 
  lcd.print ("  molenteller V1.0  ");                                    // |
  lcd.setCursor(0, 2);                                                   // |
  lcd.print ("   met commentaar   ");                                    // |
  delay (5000);                                                          // |Versie informatie gedurende 5 seconden
  Serial.print ("jaartal inlezen : ");                                   // tekst naar de seriële monitor.
  Serial.println (jaartal);                                              // verzend het jaartal naar de seriële monitor en ga een regel omlaag.  
  Serial.print ("aantal omwentelingen inlezen : ");                      // verzend de tekst naar de seriële monitor.
  Serial.println (revolutionsSpurwheel);                                 // verzend "aantal omw spoorwiel" naar de seriële monitor en ga een regel omlaag.
  Serial.print ("aantal malen opslag inlezen : ");                       // verzendt de tekst naar de seriële monitor.
  Serial.println (opslagEEprom);                                         // verzendt "aantal malen opslag in EEprom" naar de seriële monitor en ga een regel omlaag.  
  numberOfRevolutions = revolutionsSpurwheel / (bw / sl);                // aantal omwentelingen spoorwiel gedeeld door bovenwiel/bovenschijfloop is aantal omwentelingen bovenas.
  stilGevlucht();                                                        // spring naar subroutine stilGevlucht.
}                                                                        // End of subRoutine "setup".



void loop()                                                              // the loop routine is running all the time.  
{
  (buttonAState) = digitalRead(reedSchakTel);                            // controleer of reed schakelaar A is geactiveerd.
  if (buttonAState != buttonAStateOld)                                   // als: de schakelaar bediend wordt.
    {
    if (buttonAState == LOW)                                             // als: de "telpuls-schakelaar" is laag.
      {
      rotationTimeA = millis();                                          // dan: zet variabele rotationTimeA op huidige tijd.
      wiekSnelheid();                                                    // dan: spring naar subRoutine "wiekSnelheid bepalen".
      totaalOmw();                                                       // dan: spring naar subRoutine "totaal aantal omwentelingen per jaar".
      revolutionsSpurwheel = revolutionsSpurwheel + 1;                   // dan: verhoog variabele revolutionsSpurwheel met 1.
      }
    }
  buttonAStateOld = buttonAState;                                        // zet de tijd uit buttonAState in buttonAStateOld
  
  if (millis() - rotationTimeA == 8000)                                  // als: de huidgie tijd min rotationTimeA is gelijk aan 8000 miliseconden.
    {
    stilGevlucht();                                                      // dan: spring naar subRoutine "gevlucht staat stil".
    }

  if (millis() - bijwerkTijd > 300000)                                   // als: de huidgie tijd min bijwerkTijd is groter dan 300.000 miliseconden (dat is 5 minuten)
    {
    bijwerkEEprom();                                                     // dan: spring naar subRoutine "bijwerkEEprom" om de EEPROM bij te werken.
    }

  (buttonResetState) = digitalRead(reedSchakReset);                      // controleer of reed schakelaar RESET is geactiveerd.
  if (buttonResetState != buttonResetStateOld)                           // als: de schakelaar bediend wordt.
    {
    if (buttonResetState == LOW)                                         // als: de reset schakelaar "weetZekerDisplay" is laag.
      {
      weetZekerDisplay();                                                // dan: jump to subRoutine "weetZekerDisplay.
      rotationTimeA = millis();                                          // dan: zet variabele rotationTimeA op huidige tijd.  
      }
    }  
    buttonResetStateOld = buttonResetState;                              // zet de tijd uit buttonResetState in buttonResetStateOld.
}                                                                        // End of subRoutine "loop". 



void wiekSnelheid()                                                      // subRoutine "bepaald de snelheid van de wieken in enden per minuut".
{
  digitalWrite(LED_Still, LOW);                                          // blauwe LED uit omdat deze aan is wanneer het gevlucht stil stond.
  delay(10);                                                             // hele korte pauze (tijd in milliseconds).
  digitalWrite(LED_Forward, HIGH);                                       // groene LED aan.
  rotationSpeed = rotationTimeA - rotationTimeAOld;                      // berekent de tijd voor een omwenteling van het spoorwiel.
  rotationTimeAOld = rotationTimeA;                                      // zet de tijd uit rotationTimeA in rotationTimeAOld.
  numberOfSails = 60000 / rotationSpeed * conversion;                    // berekent het aantal enden (wieken) wat er per minuut voorbij komt.
  if (numberOfSails > 120 || numberOfSails < 0)                          // | als: aantal omw groter is dan 120 of aantal omw is kleiner dan 0.
    {                                                                    // |
    numberOfSails = 0;                                                   // | dan: aantal omw = 0.
    }                                                                    // | correctie voor een te hoog of te laag aantal enden.
  lcd.setCursor(0, 0);                                                   // plaats de cursor op positie 1, regel 1.
  lcd.print("    ");                                                     // verzend de spaties naar het LCD display.
  lcd.setCursor(0, 0);                                                   // plaats de cursor op positie 1, regel 1.
  lcd.print(numberOfSails,0);                                            // verzend het "aantal enden per minuut" naar het LCD display.
  lcd.setCursor(4, 0);                                                   // plaats de cursor op positie 5, regel 1.
  lcd.print("enden per minuut");                                         // verzend de tekst naar het LCD display.
  lcd.setCursor(0, 3);                                                   // plaats de cursor op positie 0, regel 4.
  lcd.print("                    ");                                     // verzend de spaties naar het LCD display.
  digitalWrite(LED_Forward, LOW);                                        // groene LED uit.
  Serial.print("Aantal enden : ");                                       // verzend de tekst naar de seriële monitor.
  Serial.println(numberOfSails, 0);                                      // verzend het "aantal enden per minuut" naar de seriële monitor en ga een regel omlaag.
  Serial.print("Tijd voor 1 omwenteling (millisec) : ");                 // verzend de tekst naar de seriële monitor.
  Serial.println(rotationSpeed, 0);                                      // verzend de "tijd voor 1 omwenteling spoorwiel" naar de seriële monitor en ga een regel omlaag.
}                                                                        // End of subRoutine "wieksnelheid".



void totaalOmw()                                                         // subRoutine "telt het totaal aantal omwentelingen per jaar".
{
  numberOfRevolutions = revolutionsSpurwheel / (bw / sl);                // aantal omwentelingen bovenas is aantal omwentelingen spoorwiel gedeeld door bovenwiel/bovenschijfloop is.
  lcd.setCursor(0, 1);                                                   // plaats de cursor op positie 1, regel 2.
  lcd.print("        ");                                                 // verzend de spaties naar het LCD display.
  lcd.setCursor(0, 1);                                                   // plaats de cursor op positie 1, regel 2.
  lcd.print(numberOfRevolutions);                                        // verzend "aantal omwentelingen bovenas" naar het LCD display.
  lcd.setCursor(7, 1);                                                   // plaats de cursor op positie 8, regel 2.
  lcd.print(" omw. in ");                                                // verzend tekst naar het LCD display.
  lcd.setCursor(16, 1);                                                  // plaats de cursor op positie 17, regel 2. 
  lcd.print(jaartal);                                                    // verzend het "jaartal" naar het LCD display.  
  lcd.setCursor(0, 2);                                                   // plaats de cursor op positie 1, regel 3.
  lcd.print ("                    ");                                    // verzend de spaties naar het LCD display.
  lcd.setCursor(0, 3);                                                   // plaats de cursor op positie 1, regel 4.
  lcd.print ("                    ");                                    // verzend de spaties naar het LCD display.  
  Serial.print("totaalOmw : ");                                          // verzend tekst naar de seriële monitor.
  Serial.println(numberOfRevolutions);                                   // verzend "aantal omwentelingen bovenas" naar de seriële monitor en ga een regel omlaag.
  Serial.println("");                                                    // lege regel.
}                                                                        // End of subRoutine "totaalOmw".



void stilGevlucht()                                                      // subRoutine "als de wieken X seconden stil staan wordt dat gemeld".
{
  digitalWrite(LED_Still, HIGH);                                         // blauwe LED aan.
  lcd.setCursor(0, 0);                                                   // plaats de cursor op positie 1, line 1.
  lcd.print("        ");                                                 // verzend de spaties naar het LCD display.
  lcd.setCursor(0, 0);                                                   // plaats de cursor op positie 1, line 1.
  lcd.print(numberOfRevolutions);                                        // verzend "aantal omwentelingen bovenas" naar het LCD display.
  lcd.setCursor(7, 0);                                                   // plaats de cursor op positie 8, line 1.
  lcd.print(" omw. in ");                                                // verzend de tekst naar het LCD display.
  lcd.setCursor(16, 0);                                                  // plaats de cursor op positie 17, line 1. 
  lcd.print(jaartal);                                                    // verzend het "jaartal" naar het LCD display.
  lcd.setCursor(0, 1);                                                   // plaats de cursor op positie 1, line 2.
  lcd.print ("De wieken staan stil");                                    // verzend de tekst naar het LCD display.
  lcd.setCursor(0, 2);                                                   // plaats de cursor op positie 1, line 3.
  lcd.print ("                    ");                                    // verzend de spaties naar het LCD display.
  lcd.setCursor(0, 3);                                                   // plaats de cursor op positie 1, line 3.
  lcd.print ("                    ");                                    // verzend de spaties naar het LCD display.
  lcd.setCursor(2, 3);                                                   // plaats de cursor op positie 3, line 4,
                                                                         // de tekst (molennaam 3 regels lager) kun je uitlijnen met het eerste cijfer (2).
  lcd.write(1);                                                          // teken karakter uit geheugenplaats 1.
  lcd.print(" de Heimolen ");                                            // verzend de molennaam (max 18 karakters) naar het LCD display. (uitlijnen voor je eigen molennaam),
  lcd.write(1);                                                          // teken karakter uit geheugenplaats 1.
  Serial.println ("Stil gevlucht");                                      // print de tekst naar de seriële monitor en ga een regel omlaag.
  Serial.println("");                                                    // lege regel.
}                                                                        // End of subRoutine "stilGevlucht".



void bijwerkEEprom()                                                     // Subroutine "gegevens naar EEprom schrijven".
{ 
  opslagEEprom = opslagEEprom +1;                                        // verhoogt opslag met 1.  
  Serial.println("Opslaan op EEprom  ");                                 // verzendt de tekst naar het LCD display en ga een regel omlaag.  
  EEPROM.writeFloat(addresomwenteling, revolutionsSpurwheel);            // plaatst omwentelingen spoorwiel in Eeprom geheugenplaats.
  EEPROM.writeInt(addresjaartal, jaartal);                               // plaatst jaartal in Eeprom geheugenplaats.
  EEPROM.writeInt(addresOpslag, opslagEEprom);                           // plaatst het aantal malen in dat er opgeslagen is. (max 100.000 maal)    
    Serial.print ("Opslaan aantal omwentelingen : ");                    // verzendt de tekst naar het LCD display.
    Serial.print (revolutionsSpurwheel / (bw / sl), 0);                  // verzendt "aantal omwentelingen spoorwiel" naar het LCD display.
    Serial.print (" - ");                                                // verzendt een streepje naar het LCD diplay.
    Serial.println (EEPROM.readFloat(addresomwenteling) / (bw / sl), 0); // leest "aantal omwentelingen spoorwiel" uit het geheugen en verzendt dit naar de seriële monitor en ga een regel omlaag.  
    Serial.print ("Opslaan jaartal : ");                                 // verzendt de tekst naar het LCD display.
    Serial.print (jaartal);                                              // verzendt het jaartal naar het LCD display.
    Serial.print (" - ");                                                // verzendt een streepje naar het LCD diplay. 
    Serial.println (EEPROM.readInt(addresjaartal));                      // leest "het jaartal" uit het geheugen en verzendt dit naar de seriële monitor en ga een regel omlaag.
    Serial.print ("Opslaan aantal schrijfacties : ");                    // verzendt de tekst naar het LCD display.
    Serial.print (opslagEEprom);                                         // verzendt het aantal malen dat er in de EEprom geschreven is naar het LCD display.
    Serial.print (" - ");                                                // verzendt een streepje naar het LCD diplay. 
    Serial.println (EEPROM.readInt(addresOpslag));                       // leest het aantal "schrijfacties EEprom" uit het geheugen en verzendt dit naar de seriële monitor en ga een regel omlaag.                         
  bijwerkTijd = millis();                                                // zet de huidige tijd in variabele 'bijwerkTijd'.
  Serial.println("");                                                    // lege regel.  
}                                                                        // End of subroutine "bijwerkEEprom".



void weetZekerDisplay()                                                  // Subroutine "weet u zeker dat, display?"
{
  lcd.clear();                                                           // maak het LCD display leeg.
  lcd.setCursor(0, 0);                                                   // plaats de cursor op positie 1, line 1.
  lcd.print("Weet u dit zeker?   ");                                     // verzend de tekst naar het LCD display.
  lcd.setCursor(0, 1);                                                   // plaats de cursor op positie 1, line 2.
  lcd.print("Binnen 2 seconde nog");                                     // verzend de tekst naar het LCD display.
  lcd.setCursor(0, 2);                                                   // plaats de cursor op positie 1, line 3.
  lcd.print("een keer.           ");                                     // verzend de tekst naar het LCD display.
  lcd.setCursor(0, 3);                                                   // plaats de cursor op positie 1, line 1.
  lcd.print("                    ");                                     // verzend de spaties naar het LCD display.
  delay (1000);                                                          // hele korte pauze (tijd in milliseconds). 
  for (int i = 0; i <= 10; i++) {                                        // LED knippert 10 maal.
  digitalWrite(LED_Still, LOW);                                          // blauwe LED uit.
  Serial.println("knipper");                                             // stuur "knipper" naar de seriële monitor en ga een regel omlaag.
  delay(80);                                                             // vertraging LED uit.
  (buttonSureState) = digitalRead(reedSchakReset);                       // controleer of reed schakelaar B RESET is geactiveerd.
   if (buttonSureState != buttonSureStateOld)                            // als: de reedschakelaar B RESET bediend wordt.
     {
     if (buttonSureState == LOW)                                         // als: de reedschakelaar B RESET laag is.
       { 
       resetTotaalOmw();                                                 // dan: spring naar subRoutine "reset totaal aantal omwentelingen per jaar".  
       break;                                                            // dan: breek het geknipper af en spring naar subroutineloop.
       }
     }
     buttonSureStateOld = buttonSureState;                               // zet de tijd uit buttonSureState in buttonSureStateOld.
   digitalWrite(LED_Still, HIGH);                                        // blauwe LED aan.
   delay(120);                                                           // hele korte pauze (tijd in milliseconds).   
  }
  Serial.println("");                                                    // lege regel.  
  wiekSnelheid();                                                        // spring naar subRoutine "wiekSnelheid".
  totaalOmw();                                                           // spring naar subRoutine "totaalOmw".           
}                                                                        // End of subroutine "weetZekerDisplay".



void resetTotaalOmw()                                                    // Subroutine "zet het aantal omwentelingen 1 maal per jaar terug op 0 en het jaartal met 1 verhogen".
{
  Serial.println("resetTotaalOmw");                                      // verzend de tekst naar de seriële monitor en ga een regel omlaag.
  jaartal = jaartal +1;                                                  // verhoogt jaartal met 1.
  revolutionsSpurwheel = 1;                                              // zet omwentelingen spoorwiel op 1.
  EEPROM.writeInt(addresjaartal, jaartal);                               // zer het jaartal in het EEprom geheugen.
  EEPROM.writeFloat(addresomwenteling, revolutionsSpurwheel);            // zet het aantal omwentelingen spoorwiel in het Eeprom geheugen.
  digitalWrite(LED_Still, LOW);                                          // blauwe LED uit, als deze aan was ivm stilstaand gevlucht.
  delay(200);                                                            // korte pauze (tijd in milliseconds).   
  digitalWrite(LED_Forward, HIGH);                                       // groene LED aan.
  delay(200);                                                            // korte pauze (tijd in milliseconds). 
  digitalWrite(LED_Forward, LOW);                                        // groene LED uit.
  delay(40);                                                             // hele korte pauze (tijd in milliseconds). 
  digitalWrite(LED_Still, HIGH);                                         // blauwe LED aan.
  delay(200);                                                            // korte pauze (tijd in milliseconds). 
  digitalWrite(LED_Still, LOW);                                          // blauwe LED uit.
  delay(40);                                                             // hele korte pauze (tijd in milliseconds). 
  digitalWrite(LED_red, HIGH);                                           // rode LED aan.
  delay(300);                                                            // korte pauze (tijd in milliseconds). 
  digitalWrite(LED_red, LOW);                                            // rode LED uit.
  delay(40);                                                             // hele korte pauze (tijd in milliseconds).   
  digitalWrite(LED_Still, HIGH);                                         // blauwe LED aan.
  delay(200);                                                            // korte pauze (tijd in milliseconds). 
  digitalWrite(LED_Still, LOW);                                          // blauwe LED uit.
  delay(40);                                                             // hele korte pauze (tijd in milliseconds).   
  digitalWrite(LED_Forward, HIGH);                                       // groene LED aan..
  delay(200);                                                            // korte pauze (tijd in milliseconds). 
  digitalWrite(LED_Forward, LOW);                                        // groene LED uit.
  Serial.println ("reset Omwentelingen en verhoog jaartal");             // verzend de tekst naar de seriële monitor en ga een regel omlaag.
  Serial.println (jaartal);                                              // verzend het "jaartal" naar de seriële monitor en ga een regel omlaag.
  Serial.println (revolutionsSpurwheel);                                 // verzend het "aantal omwentelingen van het spoorwiel" naar de seriële monitor en ga een regel omlaag.
  Serial.println("");                                                    // lege regel.  
}                                                                        // End of subroutine "resetTotaalOmw".
