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
