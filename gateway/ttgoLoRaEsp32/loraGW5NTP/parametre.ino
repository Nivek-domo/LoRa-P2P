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

void doTime() {
  timeClient.update();

  Serial.print(daysOfTheWeek[timeClient.getDay()]);
  Serial.print(", ");
  Serial.print(timeClient.getHours());
  if (timeClient.getMinutes() < 10)
  {
    Serial.print(":0");
  }
  else {
    Serial.print(":");
  }

  Serial.print(timeClient.getMinutes());

  if (timeClient.getSeconds() < 10)
  {
    Serial.print(":0");
  }
  else {
    Serial.print(":");
  }

  Serial.print(timeClient.getSeconds());
  Serial.print(" ");
  //Serial.println(MY_TZ_STRING);
  timeset = true;
  Serial.println(timeClient.getFormattedTime());
}
