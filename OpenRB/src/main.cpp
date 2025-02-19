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
const uint8_t DXL_ID1 = 101;  // Moteur XM430
const uint8_t DXL_ID2 = 1;  // Moteur XL430 avec le bout qui sort
const uint8_t DXL_ID3 = 6;  // Moteur XL430 avec le bout flat



Delta deltabot;



void setup() {
  // put your setup code here, to run once:
  delay(2000);    // Délai additionnel pour avoir le temps de lire les messages sur la console.
  DEBUG_SERIAL.println("Starting position control ...");
  
  // Use UART port of DYNAMIXEL Shield to debug.
  Serial.begin(9600);  // Port USB (débogage)  while(!DEBUG_SERIAL); // On attend que la communication série pour les messages soit prête.
  deltabot.setup();
  deltabot.detectDXL();

  // deltabot.configureDXL(DXL_ID1, DXL_ID2, DXL_ID3);
}

void loop() {
  double positions[] = {175.0, 175.0, 175.0};
  deltabot.setPositions(positions);
  // // put your main code here, to run repeatedly:
  // delay(1000);
  // positions[0] = 230.0;
  // positions[1] = 230.0;
  // positions[2] = 230.0;
  // deltabot.setPositions(positions);

  // delay(1000);
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n'); // Lire toute la ligne
    input.trim(); // Supprimer les espaces ou \r parasites

    // Découpage de la ligne en trois parties
    int firstSpace = input.indexOf(' ');
    int secondSpace = input.indexOf(' ', firstSpace + 1);

    if (firstSpace != -1 && secondSpace != -1) { // Vérifie qu'il y a bien 2 espaces
        double angle1 = input.substring(0, firstSpace).toFloat();
        double angle2 = input.substring(firstSpace + 1, secondSpace).toFloat();
        double angle3 = input.substring(secondSpace + 1).toFloat();

 
        Serial.print("Reçu : ");
        Serial.print(angle1);
        Serial.print(", ");

        
        Serial.print(angle2);
        Serial.print(", ");
        Serial.println(angle3);

        positions[0] = angle1;
        positions[1] = angle2;
        positions[2] = angle3;

        deltabot.setPositions(positions);
    } else {
        Serial.println("Erreur : format invalide !");
    }
}


}
