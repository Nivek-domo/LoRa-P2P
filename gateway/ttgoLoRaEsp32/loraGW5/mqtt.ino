

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
    String stringTwo = NodeId;
    stringTwo += "/rx";
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

}
