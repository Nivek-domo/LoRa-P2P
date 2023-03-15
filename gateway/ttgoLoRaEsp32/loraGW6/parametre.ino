void wifiTask(){

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

void LoRaFix(){
    LoRa.setTxPower(14);//Supported values are 2 to 20 for PA_OUTPUT_PA_BOOST_PIN, and 0 to 14 for PA_OUTPUT_RFO_PIN.  LoRa.setTxPower(txPower, outputPin);
    LoRa.setSpreadingFactor(7);//Supported values are between 6 and 12. If a spreading factor of 6 is set, implicit header mode must be used to transmit and receive packets.
    LoRa.setSignalBandwidth(125E3);//signalBandwidth - signal bandwidth in Hz, defaults to 125E3.
//Supported values are 7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3, 41.7E3, 62.5E3, 125E3, 250E3, and 500E3.
    LoRa.enableCrc();//LoRa.disableCrc();
    LoRa.setGain(6);//Supported values are between 0 and 6. If gain is 0, AGC will be enabled and LNA gain will not be used. Else if gain is from 1 to 6, AGC will be disabled and LNA gain will be used.
  Serial.println("LoRa Initializing OK!");
  display.setCursor(0,10);
  display.println("LoRa Initializing OK!");
  display.display();
}  


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

void calibration(long correction){ //Calibration interne en fonction de l'oscillateur                     
  Serial.print("- - Calibration demandée ");   
  NBAND = NBAND - correction;
  Serial.print("- Nouvelle fréquence programmée: ");     
  Serial.print(String(NBAND));
  Serial.println(" MHZ ");    
  if (!LoRa.begin(NBAND)) {
    Serial.println("- Starting LoRa failed!");
    while (1);
  }
  preferences.putString("frequence", String(NBAND));
  delay(10);
  String NBANDstr = preferences.getString("frequence", "");
  NBAND = atol(NBANDstr.c_str());
  Serial.print("- Nouvelle fréquence corrigée lue en EEPROM: ");
  Serial.println(NBAND); 
  LoRaFix();
  delay(100);
  Serial.println("- Modification fréquence prise en compte.");
  Serial.print("- - Calibration demandée end ");
}

void SynchroAllNode (){
  LoRaDest = LoRaBr;                //le destinataire est broadcast
  LoRaSend = "cal";
  LoRaNumMes = "00";
  transmittion();
  Serial.println("-Interrogation des nodes à portée lancée");
}

/*
 * -------------
 * Les fonctions
 * -------------
 */
long stringToLong(String s)           //convertion de String en Long
{
    char arr[12];
    s.toCharArray(arr, sizeof(arr));
    return atol(arr);
}
