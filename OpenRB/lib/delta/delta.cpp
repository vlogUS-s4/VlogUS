#include <delta.h>

#include <Dynamixel2Arduino.h>

//This namespace is required to use Control table item names
using namespace ControlTableItem;


Dynamixel2Arduino dxl(DXL_SERIAL, DXL_DIR_PIN);
const float DXL_PROTOCOL_VERSION = 2.0;

#define SPEED 50 // Valeur/255       0 = valeur maximale


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

    detectServo();
}

void Delta::configureServo(uint8_t dxlID)
{


    // Get DYNAMIXEL information
        bool ping = dxl.ping(dxlID);
        if (!ping)
        {
            DEBUG_SERIAL.println("Could not ping motor!");
            DEBUG_SERIAL.print("Last error code: ");
            DEBUG_SERIAL.println(dxl.getLastLibErrCode());

            return;
        }
   
        // Turn off torque when configuring items in EEPROM area
        dxl.torqueOff(dxlID);
        dxl.setOperatingMode(dxlID, OP_POSITION);
        dxl.torqueOn(dxlID);
        
        // Limit the maximum velocity in Position Control Mode. Use 0 for Max speed
        dxl.writeControlTableItem(PROFILE_VELOCITY, dxlID, SPEED);
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
    float f_present_position = 0.0;
    int i;

    for (i = 0; i < 3; i++)
    {
        if (position[i] < 75) {
            position[i] = 75;
        }
        if (position[i] > 290) {
            position[i] = 290;
        }
        dxl.setGoalPosition(servoIDs[i], position[i], UNIT_DEGREE);
        f_present_position = 0.0;


    }
    i = 0;
    while (i < 3) {

        f_present_position = dxl.getPresentPosition(servoIDs[i], UNIT_DEGREE);

        if (abs(position[i] - f_present_position) < 8.0) {
            i++;
            Serial.println("Objectif atteint");

        }
            
    }

}

void Delta::readAngleCommand() {
    if (Serial.available() > 0) {
        double positions[] = {175.0, 175.0, 175.0};
    
        double angle1 = Serial.parseFloat();
        double angle2 = Serial.parseFloat();
        double angle3 = Serial.parseFloat();
    
        Serial.print("Reçu : ");
        Serial.print(angle1);
        Serial.print(", ");
        Serial.print(angle2);
        Serial.print(", ");
        Serial.println(angle3);
    
        positions[0] = angle1;
        positions[1] = angle2;
        positions[2] = angle3;
    
        setServoPositions(positions);
    
    }
}


void Delta::detectServo() {
    int i = 0;
    for (uint8_t id = 0; id < 254; id++) { // Tester les ID de 0 à 253
        if (dxl.ping(id)) {
            DEBUG_SERIAL.print("Moteur détecté avec ID : ");
            DEBUG_SERIAL.println(id);
            servoIDs[i] = id;
            i++;
            configureServo(id);
        }
    }
}
