#ifndef DELTA_H
#define DELTA_H

#include <Dynamixel2Arduino.h>


class Delta {
public:

    Delta();

    void configureDXL(uint8_t dxl1, uint8_t dxl2, uint8_t dxl3);
    void setPositions(uint8_t dxl1, uint8_t dxl2, uint8_t dxl3);

private:
    uint8_t servoIDs[3];




};





#endif // DELTA_H
