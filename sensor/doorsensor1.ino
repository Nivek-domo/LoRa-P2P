int led = 6;           // the PWM pin the LED is attached to
int brightness = 0;    // how bright the LED is
int fadeAmount = 5;    // how many points to fade the LED by
int battPercent;

#include <avr/boot.h>
//Libraries for LoRa
#include <SPI.h>
#include <LoRa.h>
#include <EEPROM.h>
//define the pins used by the LoRa transceiver module
#define SCK 13
#define MISO 12
#define MOSI 11
#define SS 10
#define RST 5
#define DIO0 2
#define DIO1 3

//433E6 for Asia
//866E6 for Europe
//915E6 for North America
//#define BAND 868.1E6
#define BAND 868100000
long NBAND;

String LoRaSend;
String LoRaData;
String UUID;

int memdoor;

//-------------------------------------------------------------------
void setup() {
  pinMode(led, OUTPUT);
  pinMode (7, INPUT_PULLUP);
  analogReference(INTERNAL);
   pinMode(A0,INPUT);
  digitalWrite(led, HIGH);
  //initialize Serial Monitor
  Serial.begin(115200);


/*  
 *         #define SIGRD 5
      #if defined(SIGRD) || defined(RSIG)
          Serial.print("Signature : ");
          for (uint8_t i = 0; i < 5; i += 2) {
              Serial.print(" 0x");
              Serial.print(boot_signature_byte_get(i), HEX);
          }
          Serial.println();
      
          Serial.print("Serial Number : ");
          for (uint8_t i = 14; i < 24; i += 1) {
              Serial.print(" 0x");
              Serial.print(boot_signature_byte_get(i), HEX);
          }
          Serial.println();
      #endif
*/
        UUID = "0";
        #define SIGRD 5
      #if defined(SIGRD) || defined(RSIG)
          //Serial.print("SN : ");
          for (uint8_t i = 1; i < 13; i += 2) {
              //Serial.print(" 0x");
              //Serial.print(boot_signature_byte_get(i), HEX);
              UUID += String(boot_signature_byte_get(i), HEX);
          }
          //Serial.println();
      #endif
  Serial.print("SN : ");
  UUID.toUpperCase();
  Serial.println(UUID);

  Serial.println("LoRa duplex Test");

    //SPI LoRa pins
  //SPI.begin(SCK, MISO, MOSI, SS);
  SPI.begin();
  //setup LoRa transceiver module
  LoRa.setPins(SS, RST, DIO0);
  //LoRa.setPins(10, 5, 2);

  EEPROM.get(1, NBAND);
  Serial.print("Frequence ");
  Serial.println(NBAND);
  if( NBAND > (BAND + 30000) || NBAND < (BAND - 30000)){
    NBAND = BAND;
    Serial.print("Mauvaise Frequence retour origine ");
    Serial.println(NBAND);
  }
  
  if (!LoRa.begin(NBAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  LoRaFix();
  delay(500);  

  /*
   *  SPI.begin(SCK,MISO,MOSI,NSS); 
LoRa.setPins(NSS,RST,DI0);  
LoRa.begin(BAND); // or LoRa.begin(BAND, PABOOST) 
LoRa.setSpreadingFactor(SpreadingFactor); 
LoRa.setSignalBandwidth(SignalBandwidth); 
LoRa.setPreambleLength(PreambleLength); 
LoRa.setCodingRate4(CodingRateDenominator); 
LoRa.setSyncWord(SyncWord); 
LoRa.disableCrc();

      
exemple 5501010158BF25054FF4FFFFFFFFFFFF00
55 sychro 01 nb de byte data attendu 01 capteur tor (tout ou rien numérique 0/1) 01valeur data =1 58BF25054FF4 adresse mac de l'emetteur FFFFFFFFFFFF destinantaire ici général et 00 message original
*/
      if (digitalRead (7) == LOW){Serial.println("close");}else{Serial.println("open");}
      envLoRaDoor();
      
  digitalWrite(led, LOW);
}

void LoRaFix(){
  
/*    LoRa.setTxPower(14);//Supported values are 2 to 20 for PA_OUTPUT_PA_BOOST_PIN, and 0 to 14 for PA_OUTPUT_RFO_PIN.  LoRa.setTxPower(txPower, outputPin);
    LoRa.setSpreadingFactor(12);//Supported values are between 6 and 12. If a spreading factor of 6 is set, implicit header mode must be used to transmit and receive packets.
    LoRa.setSignalBandwidth(500E3);//signalBandwidth - signal bandwidth in Hz, defaults to 125E3.
//Supported values are 7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3, 41.7E3, 62.5E3, 125E3, 250E3, and 500E3.
    LoRa.enableCrc();//LoRa.disableCrc();
    //LoRa.setGain(6);//Supported values are between 0 and 6. If gain is 0, AGC will be enabled and LNA gain will not be used. Else if gain is from 1 to 6, AGC will be disabled and LNA gain will be used.
 */
    LoRa.setTxPower(14);//Supported values are 2 to 20 for PA_OUTPUT_PA_BOOST_PIN, and 0 to 14 for PA_OUTPUT_RFO_PIN.  LoRa.setTxPower(txPower, outputPin);
    LoRa.setSpreadingFactor(7);//Supported values are between 6 and 12. If a spreading factor of 6 is set, implicit header mode must be used to transmit and receive packets.
    LoRa.setSignalBandwidth(125E3);//signalBandwidth - signal bandwidth in Hz, defaults to 125E3.
   // LoRa.setPreambleLength(20);
//Supported values are 7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3, 41.7E3, 62.5E3, 125E3, 250E3, and 500E3.
    LoRa.enableCrc();//LoRa.disableCrc();
    LoRa.setGain(6);//Supported values are between 0 and 6. If gain is 0, AGC will be enabled and LNA gain will not be used. Else if gain is from 1 to 6, AGC will be disabled and LNA gain will be used.
  Serial.println("LoRa Initializing OK!");
}

void envLoRaDoor(){
      int battValue = analogRead(A0);
      delay(300);
      battValue = analogRead(A0);
        // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
      Serial.println(battValue);
      String batterie;
      if(battValue >= 700){ 
          batterie = "FF";
      }else{
          battPercent = (battValue - 550);
          Serial.println(battPercent);
          battPercent = (battPercent/1.2);
          Serial.println(battPercent);
          batterie = String(battPercent);
          if(battPercent>=99){batterie= "A0";}
          if(battPercent<=9){batterie= "0";batterie += String(battPercent);}
          if(battPercent<=1){batterie= "00";}
      }
      
      /*float voltage = battValue * (5.0 / 1023.0);
        // print out the value you read:
      Serial.println(voltage);*/
  
      memdoor = (digitalRead (7));
      Serial.println(memdoor);
      String Envoie = "55";//synchro  
      Envoie += "00"; //compteur du message ici 0 = original
      Envoie += "FFFFFFFFFFFF"; // public adress destinataire
      Envoie += UUID;//mon ID
      Envoie += "01"; //type capteur simple numérique (liste à definir)
      Envoie += batterie; //niveau batterie FF plein ou branché sur alim ext
      Envoie += "01"; //nb de data
      Envoie += "0";  // pour la mise en forme on met un 0 pour completer le nb de byte de data
      Envoie += (digitalRead (7));   // lecture de l'entrée du micro pour valeur numérique
      Serial.println(Envoie);
      LoRa.beginPacket();
      LoRa.print(Envoie);
      LoRa.endPacket();
}

void loop(){
  if (memdoor != digitalRead (7)){
    envLoRaDoor();
  }
/*  Serial.println(brightness);
  
  analogWrite(led, brightness);

  // change the brightness for next time through the loop:
  brightness = brightness + fadeAmount;

  // reverse the direction of the fading at the ends of the fade:
  if (brightness <= 0 || brightness >= 255) {
    fadeAmount = -fadeAmount;
  }
  // wait for 30 milliseconds to see the dimming effect
  delay(10);
*/
    //try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    //received a packet
    received();
  }

  while (Serial.available()) // Attendre l'entré de l'utilisateur
  {    
      delay(10);
      LoRaSend = Serial.readString() ;
    transmittion();    
  }
}


void transmittion(){
      Serial.println(LoRaSend);
      LoRa.beginPacket();
      LoRa.print(LoRaSend);
      LoRa.endPacket();  
}

void received(){
  Serial.println("Received packet ");

    //read packet
    while (LoRa.available()) {
      LoRaData = LoRa.readString();
      Serial.println(LoRaData);
    }

    //print RSSI of packet
    int rssi = LoRa.packetRssi();
    int snr = LoRa.packetSnr();
    int freqError =  LoRa.packetFrequencyError();
    Serial.print(" with RSSI ");    
    Serial.print(rssi);
    Serial.println("dBm"); 
    Serial.print(" SNR ");    
    Serial.println(snr);
    Serial.print(" FrequencyError ");    
    Serial.print(freqError);
    Serial.println(" HZ ");  

    if(LoRaData == "CA"){
        NBAND = NBAND - freqError;
        Serial.print(" CalibrationNewFrequency ");    
        Serial.print(String(NBAND));
        Serial.println(" MHZ ");    
        
          if (!LoRa.begin(NBAND)) {
              Serial.println("Starting LoRa failed!");
              while (1);
          }
        EEPROM.put(1, NBAND);
        LoRaFix();
        Serial.println(" change ok "); 
    }
}

void print_val(char *msg, uint8_t val)
{
    Serial.print(msg);
    Serial.println(val, HEX);
}
