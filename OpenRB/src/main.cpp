// GRO400 - Exemple d'utilisation du OpenRB avec un moteur Dynamixel sous Platform.IO.
// Basé sur l'exemple de Position Control.
// Opère un moteur (à définir par la variable DXL_ID - 1 par défaut) en position en le faisant passer
// d'une position en pulsations (1000) à une autre en degrés (5.7) et vice-versa à chaque
// seconde.
// Écrit la position en cours en pulsations à la console série (accessible par DEBUG_SERIAL).
// N'oubliez-pas de configurer votre port série pour cette console à 115200 bauds.

#include <delta.h>
#include <Arduino.h>

 
// TODO: À changer selon l'ID de votre moteur :
const uint8_t steppa = 101;  // Moteur XM430


void setup() {
  // Use UART port of DYNAMIXEL Shield to debug.
  Serial.begin(9600);  // Port USB (débogage)  while(!DEBUG_SERIAL); // On attend que la communication série pour les messages soit prête.
  pinMode(7, OUTPUT);
  pinMode(6, OUTPUT);
}

void loop() {
  digitalWrite(6, HIGH);
  for(int i = 0; i <200;i++){
    digitalWrite(7, HIGH);
    delayMicroseconds(3000);
    digitalWrite(7, LOW);
    }
 /*  digitalWrite(6, LOW);
  for(int i = 0; i <200;i++){
    digitalWrite(7, HIGH);
    delayMicroseconds(1000);
    digitalWrite(7, LOW);
    } */

}
