#include <delta.h>

#include <Dynamixel2Arduino.h>

//This namespace is required to use Control table item names
using namespace ControlTableItem;

const float DXL_PROTOCOL_VERSION = 2.0;

Dynamixel2Arduino dxl(DXL_SERIAL, DXL_DIR_PIN);

void Delta::configureDXL(uint8_t dxl1, uint8_t dxl2, uint8_t dxl3)
{
    servoIDs[0] = dxl1;
    servoIDs[1] = dxl2;
    servoIDs[2] = dxl3;


    // Get DYNAMIXEL information
    for (int i = 0; i < 3; i++) {
        bool ping = dxl.ping(servoIDs[i]);
        if (!ping)
        {
            DEBUG_SERIAL.println("Could not ping motor!");
            DEBUG_SERIAL.print("Last error code: ");
            DEBUG_SERIAL.println(dxl.getLastLibErrCode());

            return;
        }
   
        // Turn off torque when configuring items in EEPROM area
        dxl.torqueOff(servoIDs[i]);
        dxl.setOperatingMode(servoIDs[i], OP_POSITION);
        dxl.torqueOn(servoIDs[i]);
        
        // Limit the maximum velocity in Position Control Mode. Use 0 for Max speed
        dxl.writeControlTableItem(PROFILE_VELOCITY, servoIDs[i], 10);
    }
}

void Delta::setPositions()
{
    for (int i = 0; i < 3; i++)
    {
        dxl.setGoalPosition(servoIDs[i], pos[i], UNIT_DEGREE);
    }
    delay(1000);
}
void Delta::setPositions(double position[])
{
    for (int i = 0; i < 3; i++)
    {
        dxl.setGoalPosition(servoIDs[i], position[i], UNIT_DEGREE);
    }
    delay(1000);
}

void Delta::readSerial() {
    int count = 0;  // Nombre de valeurs lues
    unsigned long startTime = millis();  // Temps de départ

    while (count < 3) {  // Lire jusqu'à 3 valeurs
        if (Serial.available() >= sizeof(double)) {  // Attente de 8 octets
            double valeur;
            Serial.readBytes((char*)&valeur, sizeof(double));  // Lecture binaire
            Serial.print("Valeur reçue : ");
            Serial.println(valeur, 6);
            pos[count] = valeur;
            count++;  // Incrémenter le compteur

        }
        // Sortir si le temps d'attente dépasse 5 secondes
        if (millis() - startTime > 5000) {
            Serial.println("Timeout: pas assez de valeurs reçues.");
            break;
        }
    }
}


void Delta::setup()
{

    // Set Port baudrate to 57600bps. This has to match with DYNAMIXEL baudrate.
    dxl.begin(57600);
    if (dxl.getLastLibErrCode())
    {
        DEBUG_SERIAL.println("Could not init serial port!");
        DEBUG_SERIAL.print("Last error code: ");
        DEBUG_SERIAL.println(dxl.getLastLibErrCode());
    }
    // Set Port Protocol Version. This has to match with DYNAMIXEL protocol version.
    if (!dxl.setPortProtocolVersion(DXL_PROTOCOL_VERSION))
    {
        DEBUG_SERIAL.println("Could not set protocol version!");
        DEBUG_SERIAL.print("Last error code: ");
        DEBUG_SERIAL.println(dxl.getLastLibErrCode());
    }

    DEBUG_SERIAL.println("Setup done.");
    DEBUG_SERIAL.print("Last error code: ");
    DEBUG_SERIAL.println(dxl.getLastLibErrCode());
}

void Delta::detectDXL() {
    int i = 0;
    for (uint8_t id = 0; id < 254; id++) { // Tester les ID de 0 à 253
        if (dxl.ping(id)) {
            DEBUG_SERIAL.print("Moteur détecté avec ID : ");
            DEBUG_SERIAL.println(id);
            servoIDs[i] = id;
            i++;
        }
    }
}
