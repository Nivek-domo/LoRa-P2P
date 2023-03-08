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

#define MQTTpubQos          2

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

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      Serial.println("reco");
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      Serial.println("deco");
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic){
    std::string value = pCharacteristic->getValue();

    if(value.length() > 0){
      Serial.print("Rx : ");
      Serial.println(value.c_str());
      BLEreceiv = value.c_str();
      flagBLE = 1;
      
    if(BLEreceiv.startsWith("scan,")){ 
      Serial.println("ok test");       
      scanWifi = 1;      
    }
      
    if(BLEreceiv.startsWith("55,")){
      Serial.print("start config : ");
      writeString(wifiAddr, value.c_str()); 
    hard_restart();     
    }
    }
  }
  
  void writeString(int add, String data){
    /*int _size = data.length();
    for(int i=0; i<_size; i++){
      EEPROM.write(add+i, data[i]);
    }
    EEPROM.write(add+_size, '\0');
    EEPROM.commit();*/
    preferences.putString("Data", data);

    Serial.println("Network Credentials have been Saved");

    preferences.end();
  }

  void hard_restart() {
    esp_task_wdt_init(1,true);
    esp_task_wdt_add(NULL);
    while(true);
  }
};

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    char charBuff1[MQTT_USER.length()+1];
    char charBuff2[MQTT_PASSWORD.length()+1];
    MQTT_USER.toCharArray(charBuff1,MQTT_USER.length()+1);
    MQTT_PASSWORD.toCharArray(charBuff2,MQTT_PASSWORD.length()+1);
    // Attempt to connect
    if (client.connect(clientId.c_str(),charBuff1,charBuff2)) {
      Serial.println("connected");
      // ... and resubscribe
    String stringTwo = WiFi.macAddress();
    stringTwo += "/gateway/rx";
    char charBuf[stringTwo.length()+1];
    stringTwo.toCharArray(charBuf, stringTwo.length()+1);
    client.subscribe(charBuf);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void wifiToBLE(){
        WiFi.mode(WIFI_STA);
        // WiFi.scanNetworks will return the number of networks found
            delay(500);
        int n =  WiFi.scanNetworks();
          if (n == 0) {
            pCharacteristic->setValue("no networks found .");
            pCharacteristic->notify();
            pCharacteristic->setValue(CrLf);
            pCharacteristic->notify();
          } else {
            pCharacteristic->setValue("networks found .");
            pCharacteristic->notify();
            pCharacteristic->setValue(CrLf);
            pCharacteristic->notify();
            delay(500);
            for (int i = 0; i < n; ++i) {
              ssids_array[i + 1] = WiFi.SSID(i);
              Serial.print(i + 1);
              Serial.print(": ");
              Serial.println(ssids_array[i + 1]);
              network_string = i + 1;
              network_string = network_string + ": " + WiFi.SSID(i) + " (RSSI:" + WiFi.RSSI(i) + ")";
                      
              network_string.toCharArray(charBuf, network_string.length()+1);
              pCharacteristic->setValue(charBuf);
              pCharacteristic->notify();
              pCharacteristic->setValue(CrLf);
              pCharacteristic->notify();
            }
         }
}

void setup() {     
  scanWifi = 0;      
  flagBLE = 0;
  //initialize Serial Monitor
  Serial.begin(115200);

  preferences.begin("Credentials", false);
 
  receivedData = preferences.getString("Data", "");
  Serial.print("Read Data:");
  Serial.println(receivedData); 
 
 /* Serial.println("\nTesting EEPROM Library\n");
  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("Failed to initialise EEPROM");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }
  delay (4000);
  receivedData = read_String(wifiAddr);
  Serial.print("Read Data:");
  Serial.println(receivedData);
  delay (4000);*/
  
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

   char charBuff3[MQTTserver.length()+1];
    MQTTserver.toCharArray(charBuff3,MQTTserver.length()+1);
  client.setServer(charBuff3, MQTTport.toInt());
  client.setCallback(callback);
  reconnect();
  delay (1000);
  publishSerialData("test");
}

void loop() {

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

void bleTask(){
  // Create the BLE Device
  BLEDevice::init("ESP32 LoRa GW bis"); // Give it a name

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_TX,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
                      
  pCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_RX,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Attente connection client BLE...");
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

void wifiTask() 

  if(receivedData.length() > 0){
    String testTrame = getValue(receivedData, ',', 0);
    String wifiName = getValue(receivedData, ',', 1);
    String wifiPassword = getValue(receivedData, ',', 2);
    MQTTserver = getValue(receivedData, ',', 3);
    MQTTport = getValue(receivedData, ',', 4);
    telnetClient = getValue(receivedData, ',', 5);
    MQTT_USER = getValue(receivedData, ',', 6);
    MQTT_PASSWORD = getValue(receivedData, ',', 7);

    if(wifiName.length() > 0 && wifiPassword.length() > 0){
      Serial.print("WifiName : ");
      Serial.println(wifiName);

      Serial.print("wifiPassword : ");
      Serial.println(wifiPassword);

      Serial.print("MQTTserver : ");
      Serial.println(MQTTserver);

      Serial.print("MQTTport : ");
      Serial.println(MQTTport);

      Serial.print("telnetClient : ");
      Serial.println(telnetClient);

      Serial.print("MQTT_USER : ");
      Serial.println(MQTT_USER);

      Serial.print("MQTT_PASSWORD : ");
      Serial.println(MQTT_PASSWORD);

      WiFi.begin(wifiName.c_str(), wifiPassword.c_str());
      Serial.print("Connecting to Wifi");
      while(WiFi.status() != WL_CONNECTED){
        Serial.print(".");
        delay(1000);
      }
      Serial.println();
      Serial.print("Connected with IP: ");
      Serial.println(WiFi.localIP());
      

      Serial.print("Ping Host: ");
      Serial.println(MQTTserver);
      //Serial.println(remote_host);
      MQTTserver.toCharArray(charBuf, MQTTserver.length()+1);
      if(Ping.ping(charBuf)){
        Serial.println("Success!!");
      }else{
        Serial.println("ERROR!!");
      }
      
    }
  } 
}

void transmittion(){
      Serial.println(LoRaSend);
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

 /*   stringTwo = stringOne;
    stringTwo += "/destination";
    Serial.println(stringTwo);
    Serial.println(stringTwo.length()+1);
     dataBuf[stringTwo.length()+1];
    stringTwo.toCharArray(dataBuf, stringTwo.length()+1);
     dataCBuf[destinataire.length()+1];
    destinataire.toCharArray(dataCBuf, destinataire.length()+1);
  client.publish(dataBuf, dataCBuf);*/
  
}}}

void pubDataLora(String dataLora, String payloadLora, String emetteurLora){
    String stringTwo = WiFi.macAddress();
    stringTwo += "/";
    stringTwo += emetteurLora;
    stringTwo += "/";
    stringTwo += payloadLora;
    //Serial.println(stringTwo);
    //Serial.println(stringTwo.length()+1);
    char dataBuf[stringTwo.length()+1];
    stringTwo.toCharArray(dataBuf, stringTwo.length()+1);
    char dataCBuf[dataLora.length()+1];
    dataLora.toCharArray(dataCBuf, dataLora.length()+1);
  client.publish(dataBuf, dataCBuf);
}

void callback(char* topic, byte *payload, unsigned int length) {
    Serial.println("-------new message from broker-----");
    Serial.print("channel:");
    Serial.println(topic);
    //Serial.println("");
    Serial.print("data:");  
    //Serial.write(payload, length);
    LoRaSend = "";
    for (int i = 0; i < length; i++) {
      LoRaSend += ((char)payload[i]);
    }
    
    transmittion();

 /*       char temp[2];
        char c;
        int index;
        int e;
        uint8_t tx_buffer[128];
        uint8_t len_buffer=0;
      
        for (e = 0; e < length; e += 2) {
          temp[0] = payload[e];
          temp[1] = payload[e + 1];
          tx_buffer[len_buffer] = strtol(temp,NULL,16);
                  if (tx_buffer[len_buffer] <= 0x0F){
                    tft.print("0");
                  }
            tft.print(strtol(temp,NULL,16),HEX);
          len_buffer++;}

          //Serial2.println(len_buffer);

          for (e=0; e<len_buffer; e++){
            Serial2.write(tx_buffer[e]);
            //Serial.print(" ");
        
          }
    //Serial2.write(payload, length);
            tft.println("                                                                                                                                                                                                                                      ");
          //delay(1);*/
}

void publishSerialData(char *serialData){
  if (!client.connected()) {
    reconnect();
  }
  String stringOne = WiFi.macAddress();
    stringOne += "/gateway/tx";
    char charBuf[stringOne.length()+1];
    stringOne.toCharArray(charBuf, stringOne.length()+1);
  client.publish(charBuf, serialData);
}
