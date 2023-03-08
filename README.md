@ -0,0 +1,24 @@
# Lora-p2p-arduino
réseau LoRa en p2p résilient

![image](https://user-images.githubusercontent.com/123359286/223655576-a041355b-3199-430c-aea4-ddaf2be827e6.png)
L'idée est de créer un protocole longue distance et faible consommation sur des fréquences libres.
avec les copains nous avons déjà avancé en terme de matériel et début de programmation.
les premier tests sont très interessants en terme de distance et répétition
nous avonc notre serveur Mqtt en commun pour mettre au point le système.
les accès au serveur ne se feront qu'en pv

les transmission sont sous la forme
synchro + numéro de répétition + mac destinataire + mac emetteur + type d'emetteur + niveau batterie + nombre de byte de data + Data
exemple 5501FFFFFFFFFFFF0AAFFC52681701520100
donc on décompose 55-01-FFFFFFFFFFFF-0AAFFC526817-01-52-01-00 
55 synchro sorte de clef pour savoir ou non si on décode la trame
01 la trame est répété par un node, on limite la répétition à 4 se qui permet déjà avec 4 nodes répéteur de transmettre les info d'un capteur sur plus d'1 km
FFFFFFFFFFFF pour qui? ici FF.. veut dire public adresse donc c'est pour tout le monde
0AAFFC526817 c'est l'émetteur de la trame
01 on a ici un capteur binaire basic
52 soit le pourcentage dse la batterie
01 nombre de byte de data pour faciliter le décodage on sait que l'on a 1 byte de data à traiter
00 donc valeur binaire à "0"

les différents type reste à définir
