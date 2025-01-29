#include <delta.h>


    void Delta::configureDXL(uint8_t dxl1, uint8_t dxl2, uint8_t dxl3) {
        servoIDs[0] = dxl1;
        servoIDs[1] = dxl2;
        servoIDs[2] = dxl3;

    }
