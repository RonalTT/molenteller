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


#include <Wire.h>
#include <EEPROMex.h>
#include <LiquidCrystal_I2C.h>

/*
 * kies 0x27 als je een PCF8574 van NXP heb,
 * of gebruik 0x3F als je een chip van Ti (Texas Instruments) heb PCF8574A
 */

LiquidCrystal_I2C lcd(0x27, 20, 4);

/*
 * het molensymbool is gemaakt met de generator aan op:
 * https://maxpromer.github.io/LCD-Character-Creator/
 */

byte mill[] = {
  0x00,
  0x11,
  0x0A,
  0x04,
  0x0E,
  0x15,
  0x0E,
  0x0E
};


int reedSchakTel = 2;
int reedSchakReset = 4;
int buttonAState = 1;
int buttonAStateOld = 1;
int buttonResetState = 1;
int buttonResetStateOld = 1;
int buttonSureState = 0;
int buttonSureStateOld = 0;
int LED_red = 5;
int LED_Forward = 6;
int LED_Still = 7;
long rotationTimeA;
long rotationTimeAOld;
float rotationSpeed;
float numberOfSails;
float bw = 71.0;                                                         // aantal kammen / staven bovenwiel van uw molen. Schrijf de waarde met .0 er achter.
float sl = 37.0;                                                         // aantal kammen / staven schijfloop/wieg/bonkelaar van uw molen. Schrijf de waarde met .0 er achter.
float conversion = 60.0/(15.0*(bw/sl));
unsigned long numberOfRevolutions;
unsigned long revolutionsSpurwheel = 1;
unsigned long bijwerkTijd = millis();
unsigned int jaartal = 2019;
unsigned int opslagEEprom = 200;
int addresjaartal = 88;                                                  // adres in EEprom waar jaartal wordt gelezen en geschreven.
int addresomwenteling = 78;                                              // adres in EEprom waar omwentelingen spoorwiel wordt gelezen en geschreven. 
int addresOpslag = 68;


void wiekSnelheid();
void totaalOmw();
void stilGevlucht();
void bijwerkEEprom();
void weetZekerDisplay();
void resetTotaalOmw();



void setup()
{
  lcd.init();
  lcd.backlight();
  lcd.createChar(1, mill);
  Serial.begin(9600);
  pinMode(reedSchakTel, INPUT_PULLUP);
  pinMode(reedSchakReset, INPUT_PULLUP);
  pinMode(LED_red, OUTPUT);
  pinMode(LED_Forward, OUTPUT);
  pinMode(LED_Still, OUTPUT);
  rotationTimeAOld = millis();
  revolutionsSpurwheel = EEPROM.readFloat(addresomwenteling);            // leest het aantal omwentelingen uit de EEprom.
  jaartal = EEPROM.readInt(addresjaartal);                               // leest datum uit de EEprom. (deze twee strepen // VOOR deze regel typen om het jaartal terug naar 2019 te zetten) 
  opslagEEprom = EEPROM.readInt(addresOpslag);
  lcd.setCursor(0, 1);
  lcd.print ("  molenteller  V1.0 ");
  lcd.setCursor(0, 2);
  lcd.print ("  zonder commentaar ");
  delay (5000);
  Serial.print ("jaartal inlezen : ");
  Serial.println (jaartal); 
  Serial.print ("aantal omwentelingen inlezen : ");
  Serial.println (revolutionsSpurwheel);
  Serial.print ("aantal malen opslag inlezen : ");
  Serial.println (opslagEEprom);  
  numberOfRevolutions = revolutionsSpurwheel / (bw / sl);
  stilGevlucht();
}



void loop()  
{
  (buttonAState) = digitalRead(reedSchakTel);
  if (buttonAState != buttonAStateOld)
    {
    if (buttonAState == LOW)
      {
      rotationTimeA = millis();
      wiekSnelheid();
      totaalOmw();
      revolutionsSpurwheel = revolutionsSpurwheel + 1;
      }
    }
  buttonAStateOld = buttonAState;
  
  if (millis() - rotationTimeA == 8000)
    {
    stilGevlucht();
    }

  if (millis() - bijwerkTijd > 300000)                                   // hier is vastgelegd dat de gegevens elke 5 minuten naar de EEprom worden geschreven 
    {
    bijwerkEEprom();
    }

  (buttonResetState) = digitalRead(reedSchakReset);
  if (buttonResetState != buttonResetStateOld)
    {
    if (buttonResetState == LOW)
      {
      weetZekerDisplay();
      rotationTimeA = millis();
      }
    }  
    buttonResetStateOld = buttonResetState;
}



void wiekSnelheid()
{
  digitalWrite(LED_Still, LOW);
  delay(10);
  digitalWrite(LED_Forward, HIGH);
  rotationSpeed = rotationTimeA - rotationTimeAOld;
  rotationTimeAOld = rotationTimeA;
  numberOfSails = 60000 / rotationSpeed * conversion;
  if (numberOfSails > 120 || numberOfSails < 0)
    {
    numberOfSails = 0;
    }
  lcd.setCursor(0, 0);
  lcd.print("    ");
  lcd.setCursor(0, 0);
  lcd.print(numberOfSails,0);
  lcd.setCursor(4, 0);
  lcd.print("enden per minuut");
  lcd.setCursor(0, 3);
  lcd.print("                    ");
  digitalWrite(LED_Forward, LOW);
  Serial.print("Aantal enden : ");
  Serial.println(numberOfSails, 0);
  Serial.print("Tijd voor 1 omwenteling (millisec) : ");
  Serial.println(rotationSpeed, 0);
}



void totaalOmw()
{
  numberOfRevolutions = revolutionsSpurwheel / (bw / sl);
  lcd.setCursor(0, 1);
  lcd.print("        ");
  lcd.setCursor(0, 1);
  lcd.print(numberOfRevolutions);
  lcd.setCursor(7, 1);
  lcd.print(" omw. in ");
  lcd.setCursor(16, 1);
  lcd.print(jaartal);
  lcd.setCursor(0, 2);
  lcd.print ("                    ");
  lcd.setCursor(0, 3);
  lcd.print ("                    ");
  Serial.print("totaalOmw : ");
  Serial.println(numberOfRevolutions);
  Serial.println("");
}



void stilGevlucht()
{
  digitalWrite(LED_Still, HIGH);
  lcd.setCursor(0, 0);
  lcd.print("        ");
  lcd.setCursor(0, 0);
  lcd.print(numberOfRevolutions);
  lcd.setCursor(7, 0);
  lcd.print(" omw. in ");
  lcd.setCursor(16, 0);
  lcd.print(jaartal);
  lcd.setCursor(0, 1);
  lcd.print ("De wieken staan stil");
  lcd.setCursor(0, 2);
  lcd.print ("                    ");
  lcd.setCursor(0, 3);
  lcd.print ("                    ");
  lcd.setCursor(2, 3);                                                   // de tekst (molennaam 3 regels lager) kun je uitlijnen met het eerste cijfer (2).
                      
  lcd.write(1);
  lcd.print(" de Heimolen ");                                            // verzend de molennaam (max 18 karakters) naar het LCD display. (uitlijnen voor je eigen molennaam),
  lcd.write(1);
  Serial.println ("Stil gevlucht");
  Serial.println("");
}



void bijwerkEEprom()
{ 
  opslagEEprom = opslagEEprom +1;
  Serial.println("Opslaan op EEprom  ");
  EEPROM.writeFloat(addresomwenteling, revolutionsSpurwheel);
  EEPROM.writeInt(addresjaartal, jaartal);
  EEPROM.writeInt(addresOpslag, opslagEEprom);
    Serial.print ("Opslaan aantal omwentelingen : ");
    Serial.print (revolutionsSpurwheel / (bw / sl), 0);
    Serial.print (" - ");
    Serial.println (EEPROM.readFloat(addresomwenteling) / (bw / sl), 0);
    Serial.print ("Opslaan jaartal : ");
    Serial.print (jaartal);
    Serial.print (" - ");
    Serial.println (EEPROM.readInt(addresjaartal));
    Serial.print ("Opslaan aantal schrijfacties : ");
    Serial.print (opslagEEprom);
    Serial.print (" - ");
    Serial.println (EEPROM.readInt(addresOpslag));
  bijwerkTijd = millis();
  Serial.println("");
}



void weetZekerDisplay()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Weet u dit zeker?   ");
  lcd.setCursor(0, 1);
  lcd.print("Binnen 2 seconde nog");
  lcd.setCursor(0, 2);
  lcd.print("een keer.           ");
  lcd.setCursor(0, 3);
  lcd.print("                    ");
  delay (1000);
  for (int i = 0; i <= 10; i++) {
  digitalWrite(LED_Still, LOW);
  Serial.println("knipper");
  delay(80);
  (buttonSureState) = digitalRead(reedSchakReset);
   if (buttonSureState != buttonSureStateOld)
     {
     if (buttonSureState == LOW)
       { 
       resetTotaalOmw();
       break;
       }
     }
     buttonSureStateOld = buttonSureState;
   digitalWrite(LED_Still, HIGH);
   delay(120);
  }
  Serial.println("");
  wiekSnelheid();
  totaalOmw();
}



void resetTotaalOmw()
{
  Serial.println("resetTotaalOmw");
  jaartal = jaartal +1;
  revolutionsSpurwheel = 1;
  EEPROM.writeInt(addresjaartal, jaartal);
  EEPROM.writeFloat(addresomwenteling, revolutionsSpurwheel);
  digitalWrite(LED_Still, LOW);
  delay(200);
  digitalWrite(LED_Forward, HIGH);
  delay(200);
  digitalWrite(LED_Forward, LOW);
  delay(40);
  digitalWrite(LED_Still, HIGH);
  delay(200);
  digitalWrite(LED_Still, LOW);
  delay(40);
  digitalWrite(LED_red, HIGH);
  delay(300);
  digitalWrite(LED_red, LOW);
  delay(40);
  digitalWrite(LED_Still, HIGH);
  delay(200);
  digitalWrite(LED_Still, LOW);
  delay(40);
  digitalWrite(LED_Forward, HIGH);
  delay(200);
  digitalWrite(LED_Forward, LOW);
  Serial.println ("reset Omwentelingen en verhoog jaartal");
  Serial.println (jaartal);
  Serial.println (revolutionsSpurwheel);
  Serial.println("");
}
