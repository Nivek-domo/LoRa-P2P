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

        UUID = "0";
        #define SIGRD 5
      #if defined(SIGRD) || defined(RSIG)
          for (uint8_t i = 1; i < 13; i += 2) {
              UUID += String(boot_signature_byte_get(i), HEX);
          }
      #endif
  Serial.print("SN : "); // création d'un UUID au format comparable à une Mac
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

      if (digitalRead (7) == LOW){Serial.println("close");}else{Serial.println("open");}
      envLoRaDoor();
      
  digitalWrite(led, LOW);//led est la commande du power latch (bst82) qui auto-coupe son alimentation, en mode donc sauvegarde de pile le programe s'arrête là.
}

void LoRaFix(){// ces paramètre permettent de respecter les normes françaises
    LoRa.setTxPower(14);//Supported values are 2 to 20 for PA_OUTPUT_PA_BOOST_PIN, and 0 to 14 for PA_OUTPUT_RFO_PIN.  LoRa.setTxPower(txPower, outputPin);
    LoRa.setSpreadingFactor(7);//Supported values are between 6 and 12. If a spreading factor of 6 is set, implicit header mode must be used to transmit and receive packets.
    LoRa.setSignalBandwidth(125E3);//signalBandwidth - signal bandwidth in Hz, defaults to 125E3.
   // LoRa.setPreambleLength(20);
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
