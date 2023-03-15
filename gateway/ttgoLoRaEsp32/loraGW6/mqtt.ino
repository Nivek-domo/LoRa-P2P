

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
    String stringOne = NodeId + "/rx";
    char charBuf[stringOne.length()+1];
    stringOne.toCharArray(charBuf, stringOne.length()+1);
    client.subscribe(charBuf);
    
      //Inscription au topic des commandes
      String stringTwo = NodeId + "/cmd";
      char charBufCmd[stringTwo.length()+1];
      stringTwo.toCharArray(charBufCmd, stringTwo.length()+1);
      client.subscribe(charBufCmd);
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
      String STopic = String(topic);        //conversion en string du topic pour traitement
  //String Tbase = WiFi.macAddress();   //topic de base de la passerelle
  String Tbase = NodeId;                //topic de base de la passerelle
  Tbase += "/";

  Serial.println("- - - New message from broker");
  Serial.print("- Topic:");
  Serial.println(topic);

  //topic pour transmission Lora
  if(STopic==Tbase + "rx"){           
    Serial.println("- - Topic MQTT pour transmission");
    Serial.print("- Data:");  
    Serial.write(payload, length);
    Serial.println("");
    LoRaSend = "";                     //vidange de la variable
    for (int i = 0; i < length; i++) {
      LoRaSend += ((char)payload[i]);
    }

    transmittion();
    Serial.println("- - Topic MQTT pour transmission end");
  }

  //topic pour cmd
  if(STopic==Tbase + "cmd"){
    /*
    * Le Topic NodeId
    * json qui à la forme ;
    * {"cal":true} //pour lancer une synchronisation depuis la Gateway
    * {"calall":true} //pour lancer une synchronisation avce une moyenne du decallage entre la passerelle et les node à son premier niveau de porté
    */
    Serial.println("- - - Topic pour cmd");
    Serial.write(payload, length);
    DynamicJsonDocument doc(512);        //penser a ajuster la taille de la memoire alloué
    deserializeJson(doc, payload);
    boolean cal = doc["cal"];
    boolean calAll = doc["calall"];

    // Commande de calibrage
    if(cal == true){
      Serial.println("- - - Commande de calibration simple par rapport à la passerelle des nodes du reseau");
      SynchroAllNode();
      /*
      LoRaDest = LoRaBr;                //le destinataire est broadcast
      LoRaSend = "cal";
      LoRaNumMes = "00";
      transmittion();
      Serial.println("-Interrogation des nodes à portée lancée");
      */
      Serial.println("- - - Commande de calibration simple par rapport à la passerelle des nodes du reseau end");
    }
    //Commande de calibrage entre chaque node à porté de la passerelle
    if(calAll == true){
      Serial.println("- - Commande de calibration complete avec tout les node a portée directe de la passerelle");
      LoRaDest = LoRaBr;                //le destinataire est broadcast
      LoRaNumMes = "04";                //pas de répétion de message on aligne par rapport au premier niveau de node
      LoRaSend = "calall";
      transmittion();
      Serial.println("- Interrogation des nodes à portée lancée");
      LoFlSyAl = true;
      Serial.print("- Flag LoFlSyAl: ");
      Serial.println(LoFlSyAl);
      LoRaSyTemp = millis();            //On fait partir la tempo de limitation d etemps pour la réponse des nodes
      NodeCpt = 0;                      //mise à zéro du compteur de node qui répondent
      Serial.println("- - Commande de calibration complet avec tout les node a portée directe de la passerelle end");
      }

  }
  Serial.println("-------new message from broker end-----");
}

void publishSerialData(char *serialData){
  if (!client.connected()) {
    reconnect();
  }
  String stringOne = NodeId;
    stringOne += "/tx";
    char charBuf[stringOne.length()+1];
    stringOne.toCharArray(charBuf, stringOne.length()+1);
  client.publish(charBuf, serialData,true);
}

void publishStatus(char *serialData){                                             //Publication sur NodeId/status
  if (!client.connected()) {
    reconnect();
  }
  String topic = NodeId;
  topic += "/gateway/status";
  char charBuf[topic.length()+1];
  topic.toCharArray(charBuf, topic.length()+1);//-
  client.publish(charBuf, serialData,true);
}

/*void pubDataLora(String dataLora, String payloadLora, String emetteurLora){       //Permet de publier les informations de reception LoRa vers MQTT
  //String stringTwo = WiFi.macAddress();
  String stringTwo = NodeId;
  stringTwo += "/";
  stringTwo += emetteurLora;
  stringTwo += "/";
  stringTwo += payloadLora;
  char dataBuf[stringTwo.length()+1];
  stringTwo.toCharArray(dataBuf, stringTwo.length()+1);
  char dataCBuf[dataLora.length()+1];
  dataLora.toCharArray(dataCBuf, dataLora.length()+1);
  client.publish(dataBuf, dataCBuf,true);
}
*/


void pubDataLora(String dataLora, String payloadLora, String emetteurLora){
    /*String stringTwo = WiFi.macAddress();
    stringTwo += "/";
    stringTwo += emetteurLora;*/
    String stringTwo = emetteurLora;
    stringTwo += "/";
    stringTwo += payloadLora;
    //Serial.println(stringTwo);
    //Serial.println(stringTwo.length()+1);
    char dataBuf[stringTwo.length()+1];
    stringTwo.toCharArray(dataBuf, stringTwo.length()+1);
    char dataCBuf[dataLora.length()+1];
    dataLora.toCharArray(dataCBuf, dataLora.length()+1);
  client.publish(dataBuf, dataCBuf,true);
}
