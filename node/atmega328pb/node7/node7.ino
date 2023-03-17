#include <avr/boot.h>
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
String UUID;

int memdoor;

const int led = 6;
int battPercent;

int rssi;
int snr;
int freqError;
int traitementRX;

char synch[3];
char nbtxt[3];
char destinataire[13];
char emetteur[13];
char typeDevice[3];
char nivBatt[3];
char nbData[3];
char lotData[7];

byte outData[(sizeof(lotData) - 1) / 2];
byte pwmOut1;
byte pwmOut2;


//-------------------------------------------------------------------
void setup() {
  pinMode(led, OUTPUT);
  pinMode (7, INPUT_PULLUP);
  analogReference(INTERNAL);
  digitalWrite(led, HIGH);
  traitementRX = '0';
  Serial.begin(115200);
  UUID = "0";
#define SIGRD 5
#if defined(SIGRD) || defined(RSIG)
  for (uint8_t i = 1; i < 13; i += 2) {
    UUID += String(boot_signature_byte_get(i), HEX);
  }
   UUID.toUpperCase();
  //Serial.println();
#endif
  Serial.print("SN : ");
  Serial.println(UUID);
  Serial.println("LoRa duplex Test");
  SPI.begin();
  LoRa.setPins(SS, RST, DIO0);

  EEPROM.get(3, NBAND);
  Serial.print("Frequence ");
  Serial.println(NBAND);
  if ( NBAND > (BAND + 30000) || NBAND < (BAND - 30000)) {
    NBAND = BAND;
    Serial.print("Mauvaise Frequence retour origine ");
    Serial.println(NBAND);
  }
  LoRaFix();
  EEPROM.get(1, pwmOut1);
  Serial.print("mem PWM 1 ");
  Serial.println(pwmOut1);
  EEPROM.get(2, pwmOut2);
  Serial.print("mem PWM 2 ");
  Serial.println(pwmOut2);
  delay(500);
  envLoRaDoor();
  digitalWrite(led, LOW);
}

void LoRaFix() {
  if (!LoRa.begin(NBAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  LoRa.setTxPower(14);//Supported values are 2 to 20 for PA_OUTPUT_PA_BOOST_PIN, and 0 to 14 for PA_OUTPUT_RFO_PIN.  LoRa.setTxPower(txPower, outputPin);
  LoRa.setSpreadingFactor(7);//Supported values are between 6 and 12. If a spreading factor of 6 is set, implicit header mode must be used to transmit and receive packets.
  LoRa.setSignalBandwidth(125E3);//signalBandwidth - signal bandwidth in Hz, defaults to 125E3.
  LoRa.enableCrc();//LoRa.disableCrc();
  LoRa.setGain(6);//Supported values are between 0 and 6. If gain is 0, AGC will be enabled and LNA gain will not be used. Else if gain is from 1 to 6, AGC will be disabled and LNA gain will be used.
  Serial.println("LoRa Initializing OK!");
}

void envLoRaDoor() {
  int battValue = analogRead(A0);
    //Serial.println(battValue);
  delay(300);
  battValue = analogRead(A0);
    Serial.println(battValue);
  String batterie;
  if (battValue >= 900) {
    batterie = "FF";
  } else {
    battPercent = ((battValue - 600) / 1.5);
    Serial.println(battPercent);
    batterie = String(battPercent);
    if (battPercent >= 99) {
      batterie = "A0";
    }
    if (battPercent <= 9) {
      batterie = "0";
      batterie += String(battPercent);
    }
    if (battPercent <= 1) {
      batterie = "00";
    }
  }

  memdoor = (digitalRead (7));
  Serial.println(memdoor);
  Serial.print("5500FFFFFFFFFFFF");
  Serial.print(UUID); //mon ID
  Serial.print("01"); //type capteur simple numérique (liste à definir)
  Serial.print(batterie); //niveau batterie FF plein ou branché sur alim ext
  Serial.print("03");
  Serial.print(pwmOut1, HEX);   // lecture de l'entrée du micro pour valeur numérique
  Serial.print(pwmOut2, HEX);   // lecture de l'entrée du micro pour valeur numérique
  Serial.print("0");
  Serial.print(memdoor);   // lecture de l'entrée du micro pour valeur numérique
  LoRa.beginPacket();
  LoRa.print("5500FFFFFFFFFFFF");
  LoRa.print(UUID); //mon ID
  LoRa.print("01"); //type capteur simple numérique (liste à definir)
  LoRa.print(batterie); //niveau batterie FF plein ou branché sur alim ext
  LoRa.print("03");
  if(pwmOut1<=15){LoRa.print("0");}
  LoRa.print(pwmOut1, HEX);   // lecture de l'entrée du micro pour valeur numérique
  if(pwmOut2<=15){LoRa.print("0");}
  LoRa.print(pwmOut2, HEX);   // lecture de l'entrée du micro pour valeur numérique
  LoRa.print("0");
  LoRa.print(memdoor);   // lecture de l'entrée du micro pour valeur numérique
  LoRa.endPacket();
}

void loop() {
  if (memdoor != digitalRead (7)) {
    envLoRaDoor();
  }
  
    //analogWrite(pin, value)
    analogWrite(6, pwmOut1);  
    analogWrite(9, pwmOut2);
    
  //reception de LoRa
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    Serial.println("");
    Serial.print("Received packet ");
    Serial.print(packetSize);
    Serial.println(" bytes");

    byte dataLora[packetSize];
    int i = 0;
    while (LoRa.available()) {
      dataLora[i] = LoRa.read();
      i++;
    }

    //**********************************************************************************

    memcpy(synch, dataLora, 2);
    memcpy(nbtxt, dataLora + 2, 2);
    memcpy(destinataire, dataLora + 4, 12);
    memcpy(emetteur, dataLora + 16, 12);
    memcpy(typeDevice, dataLora + 28, 2);
    memcpy(nivBatt, dataLora + 30, 2);
    memcpy(nbData, dataLora + 32, 2);
    lotData[0] = dataLora[34];
    lotData[1] = dataLora[35];

    if (packetSize == 36) {
      dataLora[36] = NULL;
      dataLora[37] = NULL;
      dataLora[38] = NULL;
      dataLora[39] = NULL;
    }
    if (packetSize == 38) {
      lotData[2] = dataLora[36];
      lotData[3] = dataLora[37];
      dataLora[38] = NULL;
      dataLora[39] = NULL;
    }

    lotData[2] = dataLora[36];
    lotData[3] = dataLora[37];
    lotData[4] = dataLora[38];
    lotData[5] = dataLora[39];

    //**********************************************************************************
    Serial.print(synch);
    Serial.print("-");
    Serial.print(nbtxt);
    Serial.print("-");
    Serial.print(destinataire);
    Serial.print("-");
    Serial.print(emetteur);
    Serial.print("-");
    Serial.print(typeDevice);
    Serial.print("-");
    Serial.print(nivBatt);
    Serial.print("-");
    Serial.print(nbData);
    Serial.print("-");
    Serial.print(lotData);
    Serial.println("");

    //print RSSI of packet
    rssi = LoRa.packetRssi();
    snr = LoRa.packetSnr();
    freqError =  LoRa.packetFrequencyError();
    Serial.print("RSSI=");
    Serial.print(rssi);
    Serial.print("dBm - ");
    Serial.print("SNR=");
    Serial.print(snr);
    Serial.print(" - FrequencyError ");
    Serial.print(freqError);
    Serial.println(" HZ ");

    if (String(synch) == "CA") {
      NBAND = NBAND - freqError;
      Serial.print(" CalibrationNewFrequency ");
      Serial.print(String(NBAND));
      Serial.println(" MHZ ");
      EEPROM.put(3, NBAND);
      LoRaFix();
      Serial.println(" change ok ");
    }
    //Serial.print (synch);
    if (String(synch) == "55") {
      Serial.println("synchro ok -> pour traitement");
      traitementRX = '1';
    } else {
      Serial.println("synchro ko -> abandon");
      traitementRX = '0';
    }
  }

  if (traitementRX == '1') {
    TrameRxOK();
  }

  while (Serial.available()) // Attendre l'entré de l'utilisateur
  {
    delay(10);
    String LoRaSend = Serial.readString() ;
    Serial.println(LoRaSend);
    LoRa.beginPacket();
    LoRa.print(LoRaSend);
    LoRa.endPacket();
  }
}

void TrameRxOK() {
  traitementRX = '0';
  Serial.println(destinataire);
  Serial.println(UUID);
  if (UUID == String(destinataire)) {
    Serial.println("Pour moi -> exploitation des datas");
    Serial.print("emetteur => ");
    Serial.println(emetteur);
    Serial.print("nb bytes data => ");
    //Serial.println(nbData);
    int nbInt = ((nbData[0]-48)*10)+(nbData[1]-48);
    Serial.println(nbInt);
    Serial.print("data => ");
    Serial.println(lotData);
      unHex(lotData, outData, nbInt*2);
      for (byte i = 0; i < nbInt; i++) {
        for (int j = 7; j >= 0; j-- ){
              //Serial.print("Bit Number ");
              //Serial.print(j);
              //Serial.print (" is ");
              //Serial.println(bitRead(outData[i], j));
              if (bitRead(outData[i], j) == 1)
                Serial.print("1");
              else
                Serial.print("0");
            }
              Serial.println();
      }   
    
    pwmOut1 = outData[0];
      EEPROM.put(1, pwmOut1);
    pwmOut2 = outData[1];
      EEPROM.put(2, pwmOut2);

    delay(50);
    //envoie état comme ack
    envLoRaDoor();  
  
  } else {
    Serial.println("trame à repeter");
    Serial.println(nbtxt);
    if(atoi(nbtxt) < 4){
      //nbtxtVal=nbtxtVal+1;
      Serial.println(((atoi(nbtxt))+1));
      String envoieRepet = "550";
      envoieRepet += String(((atoi(nbtxt))+1));
      envoieRepet += String(destinataire);
      envoieRepet += String(emetteur);
      envoieRepet += String(typeDevice);
      envoieRepet += String(nivBatt);
      envoieRepet += String(nbData);
      envoieRepet += String(lotData);
      Serial.println(envoieRepet);
      delay(random(100, 150));
      LoRa.beginPacket();
      LoRa.print(envoieRepet);
      LoRa.endPacket();
      }else{
      Serial.println("fin des répétitions");
      }
  }

}


void unHex(const char* inP, byte* outP, size_t len) {
  for (; len > 1; len -= 2) {
    byte val = asc2byte(*inP++) << 4;
    *outP++ = val | asc2byte(*inP++);
  }
}

byte asc2byte(char chr) {
  byte rVal = 0;
  if (isdigit(chr)) {
    rVal = chr - '0';
  } else if (chr >= 'A' && chr <= 'F') {
    rVal = chr + 10 - 'A';
  }
  return rVal;
}
