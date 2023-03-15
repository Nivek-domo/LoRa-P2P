//Libraries for LoRa
#include <SPI.h>
#include <LoRa.h>

//Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <WiFi.h>
#include <ESP32Ping.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <esp_int_wdt.h>
#include <esp_task_wdt.h>
#include <ArduinoJson.h>
//define the pins used by the LoRa transceiver module
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

//#define BAND 868E6      //433E6 for Asia 866E6 for Europe 915E6 for North America
#define BAND 868100000    //433E6 for Asia 866E6 for Europe 915E6 for North America
long NBAND = 868100000;   //Fréquence de base

//OLED pins
#define OLED_SDA 4
#define OLED_SCL 15 
#define OLED_RST 16
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels


#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;
const int wifiAddr = 10;
char  CrLf[2] = {0x0D,0x0A};
String ssids_array[50];
String network_string;
char charBuf[50];
int scanWifi;
String MQTTserver;
String MQTTport;
String telnetClient;
String MQTT_USER;
String MQTT_PASSWORD;
String receivedData;
//variable pour LoRa                          
String LoRaId;                          //Identité du node, son adresse sans les :
String LoRaBr = "FFFFFFFFFFFF";         //Identité du broadcast
String LoRaDest;                        //Identité du destinataire
String LoRaSend;
String LoRaData;
String LoRaNumMes = "00";               //Numéro du message transmis par ce node
String LoRaDevTyp = "00";               //Type de Node ; 00 Gateway
//variables de réglages en fréquence
long LoRaSyTemp = 0 ;                   //temps d'attente de réponse de tous les nodes à portée lors de la synchro par réponse
long GWCorrection =0;                   //Valeur de la correction en fréquence de la gateway
int NodeCpt = 0;                        //Nombre de node de premier niveau ayant répondu pour la calibration de la gateway
boolean LoFlSyAl = false;               //si ce flag est levé alors c'est une synchro entre node.

//Variable propre au Node
String NodeId;
String BLEreceiv;
int flagBLE;

//variable pour NetWorkDyscovery

// See the following for generating UUIDs: https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

Preferences preferences;
WiFiClient wifiClient;
PubSubClient client(wifiClient);

/************************************************************************************************************/
void setup() {     
  scanWifi = 0;      
  flagBLE = 0;
  //initialize Serial Monitor
  Serial.begin(115200);
  while (!Serial);
  NodeId = WiFi.macAddress();
  NodeId.replace(":", "");      //supression des : de l'adresse mac
  Serial.println("identité du node :"+NodeId);
  preferences.begin("Credentials", false);
  receivedData = preferences.getString("Data", "");
  Serial.print("Read Data:");
  Serial.println(receivedData); 
  
  //reset OLED display via software
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

  //Freq adjust Lora
  preferences.begin("frequence", true);
  String NBANDstr = preferences.getString("frequence", "");
  NBAND = atol(NBANDstr.c_str());
  Serial.print("Frequence utilisée: ");
  Serial.println(NBAND);
  if( NBAND > (BAND + 30000) || NBAND < (BAND - 30000)){
    NBAND = BAND;
    Serial.print("Mauvaise Frequence retour origine : ");
    Serial.println(NBAND);
  }
  if (!LoRa.begin(NBAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  LoRaFix();
  delay(500);
  wifiTask();
  bleTask();
  
  LoRaDest = LoRaBr;            //Destinataire par défaut est le broadcast
  
   char charBuff3[MQTTserver.length()+1];
    MQTTserver.toCharArray(charBuff3,MQTTserver.length()+1);
  client.setServer(charBuff3, MQTTport.toInt());
  client.setCallback(callback);
  reconnect();
  delay (1000);
  publishSerialData("test");
}

/************************************************************************************************************/

void loop() {

    //try to parse packet
    int packetSize = LoRa.parsePacket();
    if (packetSize) {    //received a packet
      received();
    }
  
    while (Serial.available()) // Attendre l'entré de l'utilisateur
    {    
        delay(10);
        LoRaSend = Serial.readString() ;
      transmittion();    
    }
  
    // Un Calibrage de la passerelle est lancé ?
    if (LoFlSyAl == true){  //Si le flag est true calibrage locale actif
      long now = millis();
      if (now - LoRaSyTemp > 60000) { //si pas de réponse depuis 20 secondes (20 000)
        //traitement des resulats
        Serial.println("- Calibrage de la Passerelle démarré");
        if(GWCorrection > 0){
          Serial.print("- Correction de :");
          Serial.print(GWCorrection);
          Serial.println(" Hz à apporter.");
          calibration(GWCorrection);
          Serial.println("- Calibrage local terminé");
          SynchroAllNode();
          Serial.println("- Calibrage des nodes terminés");
        }
        LoFlSyAl= false;    //Calibrage terminé
      }  
    }
  
    if (scanWifi == 1){
      wifiToBLE(); 
      scanWifi = 0;    
    }
  
    if (flagBLE == 1){
      LoRaSend = BLEreceiv ;
      transmittion();       
      flagBLE = 0;
    }
      // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
   client.loop();
}

/******************************************************************************************************/

 void transmittion(){                                          //Transmission par Lora
  Serial.println("- - Transmission");
  Serial.println("Telegram à envoyer" + LoRaSend);
  telegramEncode();                                           //mise en forme des données à envoyer en LoRa
  Serial.println("LoRaSend aprés encodage: " + LoRaSend);
  LoRa.beginPacket();
  LoRa.print(LoRaSend);
  LoRa.endPacket();
  display.clearDisplay();
  display.setCursor(0,0);
  display.print("LORA DUPLEX");
  display.setCursor(0,10);
  display.print("Transmit packet:");
  display.setCursor(0,20);
  display.print(LoRaSend);
  display.display();
  Serial.println("- - Transmission");
}

void telegramEncode(){
  String telegram;
  Serial.println("telegram: " + LoRaSend);
  telegram = "55";                                          //55 synchro et identification du reseau
  Serial.println("telegram + Synchro: " + telegram);
  telegram += LoRaNumMes;                                   //Numéro du message
  Serial.println("telegram + Numéro du message: " + telegram);
  telegram += LoRaDest;                                     //Destinataire
  Serial.println("telegram + Destinataire: " + telegram);
  telegram += NodeId;                                       //ID emetteur     ------ATTENTION -------DANS LE CAS D'UN RELAIS IL VA FALLOIR GERER
  Serial.println("telegram + emetteur: " + telegram);
  telegram += LoRaDevTyp;                                   //Type de node qui transmet ; 00: Gateway
  Serial.println("telegram + Type de node emetteur: " + telegram);
  /*
   * ATTENTION VOIR COMMENT GERER LES INFOS SUIVANT LE DEVICE TYPE Car la Gateway n'a pas de raison d'être sur batterie.
   * si sur alim alors FF 
   */
  telegram += "FF";                                         //Niveau de la batterie
  Serial.println("telegram + niveau batterie: " + telegram);
  int nbData = LoRaSend.length()+1;
  if(nbData <10){telegram +="0";}
  telegram += String(nbData);
  Serial.println("telegram + longueur de LoRaData" + telegram);
  telegram += LoRaSend;
  Serial.println("telegram + LoRaData" + telegram);
  LoRaSend = telegram;
}

void received(){
  Serial.print("Received packet ");

    //read packet
    while (LoRa.available()) {
      LoRaData = LoRa.readString();
      Serial.println(LoRaData);
    }   
      String synch = LoRaData.substring(0, 2);
      Serial.print(" synchro "); 
       Serial.println(synch);
       if( synch == "55"){
      Serial.println(" telegrame reçu ok pour décryptage "); 
      String nbtxt = LoRaData.substring(2, 4);
      Serial.print(" numero du telegram "); 
       Serial.println(nbtxt);
      String destinataire = LoRaData.substring(4, 16);
      Serial.print(" destinataire "); 
       Serial.println(destinataire);
      String emetteur = LoRaData.substring(16, 28);
      Serial.print(" emetteur "); 
       Serial.println(emetteur);
      String typeDevice= LoRaData.substring(28, 30);
      Serial.print(" type device "); 
       Serial.println(typeDevice);
      String nivBatt= LoRaData.substring(30, 32);
      Serial.print(" alim "); 
       Serial.println(nivBatt);
      String nbData= LoRaData.substring(32, 34);
      Serial.print(" nb de data "); 
       Serial.println(nbData);
      String lotData= LoRaData.substring(34, 34 + (2*nbData.toInt()));
      Serial.print(" data "); 
       Serial.println(lotData);
    

    //print RSSI of packet
    int rssi = LoRa.packetRssi();
    int snr = LoRa.packetSnr();
    int freqError =  LoRa.packetFrequencyError();
    Serial.print("- with RSSI ");    
    Serial.print(rssi);
    Serial.println("dBm"); 
    Serial.print("- SNR ");    
    Serial.println(snr);
    Serial.print("- FrequencyError ");    
    Serial.print(freqError);
    Serial.println(" HZ ");   

   // Display information
   display.clearDisplay();
   display.setCursor(0,0);
   display.print("LORA DUPLEX");
   display.setCursor(0,10);
   display.print("Receiv packet:");
   display.setCursor(87,10);
   display.print(rssi);
   display.print("dB");
   display.setCursor(0,20);
   display.print(LoRaData);
   display.display(); 
   
    char charBuf[LoRaData.length()+1];
    LoRaData.toCharArray(charBuf, LoRaData.length()+1);
    publishSerialData(charBuf);  
    
    if (deviceConnected) {      
            pCharacteristic->setValue(charBuf);
            pCharacteristic->notify();
    }

    pubDataLora( lotData, "data", emetteur);
    pubDataLora( nivBatt, "alim", emetteur);
    pubDataLora( nbtxt, "nbTelegram", emetteur);
    pubDataLora( nbData, "nbdata", emetteur);
    pubDataLora( typeDevice, "type", emetteur);
    pubDataLora( destinataire, "destinataire", emetteur);
    pubDataLora( String(rssi), "rssi", emetteur);
    pubDataLora( String(snr), "snr", emetteur);
    pubDataLora( String(freqError), "ferror", emetteur);
    pubDataLora( NodeId, "gateway", emetteur);

     //Gestion du payload Lora
    if(lotData == "cal"){       //Demande de calibration en fréquence simple envoyée depuis la passerelle
      calibration(freqError);
    }

    if(lotData == "calall"){    //Demande de calibration en fréquence avec un calcul de la moyenne des ecarts entre la passereel et les nodes à porté.
      LoFlSyAl = true ;                 //Mise a jour du calibrage activé
      Serial.println("- - - Calibration demandée ");
      Serial.println(" - Préparation du message de réponse");
      LoRaDest = emetteur;              //le destinataire est l'emetteur du message (la passerelle)
      LoRaSend = "freqRep";
      LoRaSend += freqError;
      LoRaNumMes = "04";                //on ne passe pas par les relais
      Serial.println(" - Attente avant envoi");
      int tmps = random(10, 200);
      Serial.print("facteur de multiplicateur temps avant réponse: ");
      Serial.println(tmps);
      delay(100*tmps);
      Serial.println(" - Envoi Mesures freq");
      transmittion();
      Serial.println(" - Attente message de correction frequence");
    }

    String tmp = lotData.substring(0, 7);   // Gestion de la reponse lors de la demande de calibrage de la passerelle par rapport au nodes à porté
    if((tmp == "freqRep") && (LoFlSyAl == true)){             //Réponse du décalage en fréquence du node pour la passerelle
      Serial.println("- - - Calibration par moyenne des nodes de premier niveau réponse");
      Serial.print("- Décalage de fréquence du node: ");
      Serial.println(emetteur);
      String tmp2 = lotData.substring(7, lotData.length());   //Récupération de la fréquence renvoyée
      long decalage = stringToLong(tmp2);                     //passage en type long
      Serial.print("- Decalage renvoyé: ");
      Serial.print(tmp2);
      Serial.println(" Hz");
      GWCorrection = (GWCorrection - decalage)/2;
      Serial.print("- Nouvelle correction à apporter: ");
      Serial.print(GWCorrection);
      Serial.print(" Hz");
      NodeCpt++;
      Serial.print("- Nombre de Node ayant répondu: ");
      Serial.println(NodeCpt);
      Serial.println("- - - Calibration par moyenne des nodes de premier niveau réponse end");
    }
    }else{
    Serial.println("- - telegram provient de reception parasite"); 
  }
  Serial.println("- - - received LoRa packet end");
}
