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
