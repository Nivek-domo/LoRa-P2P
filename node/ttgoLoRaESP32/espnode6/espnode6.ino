//#include <avr/boot.h>
#include <SPI.h>
#include <LoRa.h>
#include <Preferences.h>
#include <WiFi.h>
//Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//define the pins used by the LoRa transceiver module
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26
//OLED pins
#define OLED_SDA 4
#define OLED_SCL 15 
#define OLED_RST 16
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);
const int MAX_ANALOG_VAL = 1024;
//433E6 for Asia
//866E6 for Europe
//915E6 for North America
//#define BAND 868.1E6
#define BAND 868100000
long NBAND;
String UUID;

int memdoor;

const int led = 5;           
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
Preferences preferences;
//-------------------------------------------------------------------
void setup() {
  pinMode(led, OUTPUT);
  pinMode (16, INPUT_PULLUP);
  //analogReference(INTERNAL);
  digitalWrite(led, HIGH);
  traitementRX = '0';
  Serial.begin(115200);
        UUID = "0";
  UUID = WiFi.macAddress();
  Serial.println(UUID);
  UUID.remove(2, 1);
  UUID.remove(4, 1);
  UUID.remove(6, 1);
  UUID.remove(8, 1);
  UUID.remove(10, 1);
  Serial.print("SN : ");
  Serial.println(UUID);
  Serial.println("LoRa duplex Test");
  preferences.begin("Credentials", false);
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);
  
  //initialize OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
    
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("LORA duplex 115200bd ");
  display.display();

    Serial.println("LoRa duplex Test");
  
  //SPI LoRa pins
  SPI.begin(SCK, MISO, MOSI, SS);
  //setup LoRa transceiver module
  LoRa.setPins(SS, RST, DIO0);
  
  String NBANDstr = preferences.getString("frequence", "");
  NBAND = atol(NBANDstr.c_str());
  Serial.print("Frequence ");
  Serial.println(NBAND);
      if( NBAND > (BAND + 30000) || NBAND < (BAND - 30000)){
        NBAND = BAND;
        Serial.print("Mauvaise Frequence retour origine ");
        Serial.println(NBAND);
      } 
  LoRaFix(); 
  display.setCursor(0,10);
  display.println("LoRa Initializing OK!");
  display.display();
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
    LoRa.setSpreadingFactor(12);//Supported values are between 6 and 12. If a spreading factor of 6 is set, implicit header mode must be used to transmit and receive packets.
    LoRa.enableCrc();//LoRa.disableCrc();
    LoRa.setGain(6);//Supported values are between 0 and 6. If gain is 0, AGC will be enabled and LNA gain will not be used. Else if gain is from 1 to 6, AGC will be disabled and LNA gain will be used.
  Serial.println("LoRa Initializing OK!");
}

void envLoRaDoor() {
  int battValue = analogRead(A13);
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

  memdoor = (digitalRead (16));
  Serial.println(memdoor);
  Serial.print("5500FFFFFFFFFFFF");
  Serial.print(UUID); //mon ID
  Serial.print("01"); //type capteur simple numérique (liste à definir)
  Serial.print(batterie); //niveau batterie FF plein ou branché sur alim ext
  Serial.print("010");
  Serial.println(memdoor);   // lecture de l'entrée du micro pour valeur numérique
  LoRa.beginPacket();
  LoRa.print("5500FFFFFFFFFFFF");
  LoRa.print(UUID); //mon ID
  LoRa.print("01"); //type capteur simple numérique (liste à definir)
  LoRa.print(batterie); //niveau batterie FF plein ou branché sur alim ext
  LoRa.print("010");
  LoRa.print(memdoor);   // lecture de l'entrée du micro pour valeur numérique
  LoRa.endPacket();
}

void loop() {
  if (memdoor != digitalRead (16)) {
    envLoRaDoor();
  }

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

    
    display.clearDisplay();
   display.setCursor(0,0);
   display.print("LORA DUPLEX");
   display.setCursor(0,10);
   display.print("Receiv packet:");
   display.setCursor(87,10);
   display.print(rssi);
   display.print("dB");
   display.setCursor(0,20);
   display.print(synch);
   display.print("-");
   display.print(nbtxt);
   display.print("-");
   display.print(destinataire);
   display.print("-");
   display.print(emetteur);
   display.print("-");
   display.print(typeDevice);
   display.print("-");
   display.print(nivBatt);
   display.print("-");
   display.print(nbData);
   display.print("-");
   display.print(lotData);
   display.display(); 

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
      preferences.putString("frequence", String(NBAND));
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
    Serial.println(nbData);
    Serial.print("data => ");
    Serial.println(lotData);
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
