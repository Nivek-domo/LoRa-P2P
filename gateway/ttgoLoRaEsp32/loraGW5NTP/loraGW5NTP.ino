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
//#include "EEPROM.h"
//#define EEPROM_SIZE 128
#include <Preferences.h>
#include <esp_int_wdt.h>
#include <esp_task_wdt.h>

//define the pins used by the LoRa transceiver module
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

//433E6 for Asia
//866E6 for Europe
//915E6 for North America
#define BAND 868.1E6

//OLED pins
#define OLED_SDA 4
#define OLED_SCL 15 
#define OLED_RST 16
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels


#include <NTPClient.h>
#include <WiFiUdp.h>
#define MY_TZ_OFFSET_HOURS 0//si on veut mettre heure été ou hiver, trop chiant à gérer donc UTC
#define MY_TZ_STRING "Paris Time"
bool timeset = false;
const long utcOffsetInSeconds = 3600 * MY_TZ_OFFSET_HOURS;
char daysOfTheWeek[7][12] = {"Dim", "Lun", "Mar", "Mer", "Jeu", "Ven", "Sam"};
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
unsigned long count = 0;

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
String NodeId;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

String LoRaSend;
String LoRaData;
String BLEreceiv;
int flagBLE;

Preferences preferences;
WiFiClient wifiClient;
PubSubClient client(wifiClient);


void setup() {     
  scanWifi = 0;      
  flagBLE = 0;
  //initialize Serial Monitor
  Serial.begin(115200);

 //Definition de l'identité du node
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

  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
    LoRa.setTxPower(14);//Supported values are 2 to 20 for PA_OUTPUT_PA_BOOST_PIN, and 0 to 14 for PA_OUTPUT_RFO_PIN.  LoRa.setTxPower(txPower, outputPin);
    LoRa.setSpreadingFactor(7);//Supported values are between 6 and 12. If a spreading factor of 6 is set, implicit header mode must be used to transmit and receive packets.
    LoRa.setSignalBandwidth(125E3);//signalBandwidth - signal bandwidth in Hz, defaults to 125E3.
//Supported values are 7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3, 41.7E3, 62.5E3, 125E3, 250E3, and 500E3.
    LoRa.enableCrc();//LoRa.disableCrc();
    LoRa.setGain(6);//Supported values are between 0 and 6. If gain is 0, AGC will be enabled and LNA gain will not be used. Else if gain is from 1 to 6, AGC will be disabled and LNA gain will be used.
  bleTask();
  
  Serial.println("LoRa Initializing OK!");
  display.setCursor(0,10);
  display.println("LoRa Initializing OK!");
  display.display();
  delay(4000);
  wifiTask();  

  Serial.println("ntp update");
  timeClient.begin();

   char charBuff3[MQTTserver.length()+1];
    MQTTserver.toCharArray(charBuff3,MQTTserver.length()+1);
  client.setServer(charBuff3, MQTTport.toInt());
  client.setCallback(callback);
  reconnect();
  delay (1000);
  publishSerialData("test");
}

void loop() {
     display.fillRect(00,57,128,70,BLACK);
     display.setCursor(00,57);
     display.print("Heure UTC => ");
     display.print(timeClient.getFormattedTime());
     display.display();   
     
    count++;
  // My ESP32 is faster than my tested ESP8266, so the modulo function is about one to two seconds (delay) . 
  // will do the math on this another time, but it is definitely faster than my ESP8266 module. as it should be.
  int my_modulo_delayer = 7900000;
  if (timeset == false or count % my_modulo_delayer == 0)
  {
    doTime();
  }
 /*  display.setCursor(0,40);
   display.print(NTP.getTimeDateString ());
   display.display(); */
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



/*String read_String(char add)
{
  int i;
  char data[100]; //Max 100 Bytes
  int len=0;
  unsigned char k;
  k=EEPROM.read(add);
  while(k != '\0' && len<500)   //Read until null character
  {    
    k=EEPROM.read(add+len);
    data[len]=k;
    len++;
  }
  data[len]='\0';
  return String(data);
}*/

String getValue(String data, char separator, int index){
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found <=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
      found++;
      strIndex[0] = strIndex[1]+1;
      strIndex[1] = (i==maxIndex) ? i+1 : i;
    }
  }
  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}



void transmittion(){
      Serial.println(LoRaSend);
      LoRa.beginPacket();
      LoRa.print(LoRaSend);
      LoRa.endPacket();
   //display.clearDisplay();
   //display.setCursor(0,0);
   //display.print("LORA DUPLEX");
   
     display.fillRect(00,9,128,10,BLACK);
     display.fillRect(00,40,128,56,BLACK);
     display.setCursor(0,10);
     display.print("Transmit packet:");
     display.setCursor(0,40);
     display.print(LoRaSend);
     display.display();   
  
}

void received(){
  Serial.print("Received packet ");

    //read packet
    while (LoRa.available()) {
      LoRaData = LoRa.readString();
      Serial.println(LoRaData);
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
    Serial.print(" with RSSI ");    
    Serial.print(rssi);
    Serial.println("dBm"); 
    Serial.print(" SNR ");    
    Serial.println(snr);
    Serial.print(" FrequencyError ");    
    Serial.print(freqError);
    Serial.println(" HZ ");   

   // Dsiplay information
   //display.clearDisplay();
   //display.setCursor(0,0);
   //display.print("LORA DUPLEX");
   
   display.fillRect(00,10,128,29,BLACK);
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
    if(NodeId != emetteur){
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
    pubDataLora( timeClient.getFormattedTime(), "time", emetteur);
    }
}}}
