#include <delta.h>
#include <AccelStepper.h>
#include <Dynamixel2Arduino.h>

// This namespace is required to use Control table item names
using namespace ControlTableItem;

Dynamixel2Arduino dxl(DXL_SERIAL, DXL_DIR_PIN);
const float DXL_PROTOCOL_VERSION = 2.0;

#define SPEED 50 // Valeur/255       0 = valeur maximale

#define DIRPIN 5
#define STEPPIN 4
#define step_delay 1 // Délai entre les impulsions de pas
// Define motor interface type (DRIVER = STEP/DIR mode)
#define MotorInterfaceType 1
// Initialize stepper (STEP pin 3, DIR pin 2)
AccelStepper stepper(MotorInterfaceType, STEPPIN, DIRPIN);

void Delta::setup()
{

    // Set Port baudrate to 57600bps. This has to match with DYNAMIXEL baudrate.
    dxl.begin(57600);

    if (dxl.getLastLibErrCode())
    {
        // DEBUG_SERIAL.println("Could not init serial port!");
        // DEBUG_SERIAL.print("Last error code: ");
        // DEBUG_SERIAL.println(dxl.getLastLibErrCode());
    }
    // Set Port Protocol Version. This has to match with DYNAMIXEL protocol version.
    if (!dxl.setPortProtocolVersion(DXL_PROTOCOL_VERSION))
    {
        // DEBUG_SERIAL.println("Could not set protocol version!");
        // DEBUG_SERIAL.print("Last error code: ");
        // DEBUG_SERIAL.println(dxl.getLastLibErrCode());
    }

    // DEBUG_SERIAL.println("Setup done.");
    // DEBUG_SERIAL.print("Last error code: ");
    // DEBUG_SERIAL.println(dxl.getLastLibErrCode());

    detectServo();
    // Configure motor settings
    stepper.setMaxSpeed(1000);    // Max steps/sec (adjust for your motor)
    stepper.setAcceleration(500); // Steps/sec² (smoother = lower value)
}

void Delta::configureServo(uint8_t dxlID)
{

    // Get DYNAMIXEL information
    bool ping = dxl.ping(dxlID);
    if (!ping)
    {
        // DEBUG_SERIAL.println("Could not ping motor!");
        // DEBUG_SERIAL.print("Last error code: ");
        // DEBUG_SERIAL.println(dxl.getLastLibErrCode());

        return;
    }

    dxl.reboot(dxlID);

    // Turn off torque when configuring items in EEPROM area
    dxl.torqueOff(dxlID);
    dxl.setOperatingMode(dxlID, OP_POSITION);
    dxl.torqueOn(dxlID);

    // Limit the maximum velocity in Position Control Mode. Use 0 for Max speed
    dxl.writeControlTableItem(PROFILE_VELOCITY, dxlID, SPEED);
    // Serial.println("Servos configurées");
}

void Delta::setServoPositions(double position[])
{
    float f_present_position = 0.0;
    int i;

    for (i = 0; i < 3; i++)
    {
        if (position[i] < 75)
        {
            position[i] = 75;
        }
        if (position[i] > 290)
        {
            position[i] = 290;
        }
        dxl.setGoalPosition(servoIDs[i], position[i], UNIT_DEGREE);
        f_present_position = 0.0;
    }
}

void Delta::readAngleCommand()
{

    double positions[3];
    if (Serial.available() > 0)
    {
        String input = Serial.readStringUntil('\n'); // Lire toute la ligne
        input.trim();                                // Supprimer les espaces ou \r parasites

        // Découpage de la ligne en trois parties
        int firstSpace = input.indexOf(' ');
        int secondSpace = input.indexOf(' ', firstSpace + 1);
        int thirdSpace = input.indexOf(' ', secondSpace + 1);

        if (firstSpace != -1 && secondSpace != -1)
        { // Vérifie qu'il y a bien 2 espaces
            double angle1 = input.substring(0, firstSpace).toFloat();
            double angle2 = input.substring(firstSpace + 1, secondSpace).toFloat();
            double angle3 = input.substring(secondSpace + 1).toFloat();
            double angle_stepper = input.substring(thirdSpace + 1).toFloat();

            positions[0] = angle1;
            positions[1] = angle2;
            positions[2] = angle3;

            setServoPositions(positions);
            stepper.moveTo(stepper.currentPosition() + angle_stepper);
        }
        else
        {
            // Serial.println("Erreur : format invalide !");
        }
    }
    // Serial.print("POS:");
    Serial.println(stepper.currentPosition());
}

void Delta::updateStepper()
{

    stepper.run(); // Must be called repeatedly
}

void Delta::detectServo()
{
    int i = 0;
    for (uint8_t id = 0; id < 254; id++)
    { // Tester les ID de 0 à 253
        if (dxl.ping(id))
        {
            // DEBUG_SERIAL.print("Moteur détecté avec ID : ");
            // DEBUG_SERIAL.println(id);
            servoIDs[i] = id;
            i++;
            configureServo(id);
        }
    }
}

void Delta::setStepperPosition(double position)
{
    bool dirValue = 0;
    if (position >= 0)
    {
        digitalWrite(DIRPIN, HIGH);
        bool dirValue = 1;
    }
    else
    {
        digitalWrite(DIRPIN, LOW);
        bool dirValue = 0;
    }
    position = abs(position);
    for (int i = 0; i < position; i++)
    {
        digitalWrite(STEPPIN, HIGH);
        delay(step_delay);
        digitalWrite(STEPPIN, LOW);
        delay(step_delay);
        if (dirValue)
        {
            stepperPosition += 1;
        }
        else
        {
            stepperPosition -= 1;
        }
    }
}