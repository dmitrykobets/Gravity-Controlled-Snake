#include <OrbitBoosterPackDefs.h>
#include <math.h>
#include "accelerometer.h"

float pitch = 0.0;
float roll = 0.0;
float pitchOffset = 0.0;
float rollOffset = 0.0;

// ALPHA and previous readings are used for the low-pass filter, which essentially filters out jittering in the accelerometer. ALPHA set to 0.9 seems to work well
const float ALPHA = 0.9; // <= 1
float prevX = 0.0;
float prevY = 0.0;
float prevZ = 0.0;

void initAccelerometer(void) {
  WireWriteRegister(ACCLADDR, 0x2D, 1 << 3); // set standby in the POWER_CTL register
  WireWriteRegister(ACCLADDR, 0x31, 1); // set 10-bit mode with 4g resolution in the DATA_FORMAT register
}

/* Function: zeroAccelerometer
 * ---------------------------
 * Reads the accelerometer tilt values and stores them as offsets
 * for future readings. The readings are taken with offsets of 0, regardless
 * of any existing offsets.
 * 
 * Parameters: none
 * Return values: none
 */
void zeroAccelerometer(void) {
  readAccelerometer(0,0);
}

/* Function: readAccelerometer
 * ---------------------------
 * Reads the accelerometer data registers (from X0 to Z1) and uses that data to compute pitch and roll.
 * Adds any existing offsets to the pitch and roll.
 *
 * Parameters: 
 *  - pitchOffset
 *  - rollOffset
 *
 * Return values: none
 */
void readAccelerometer(float pitchOffset, float rollOffset) {

  uint32_t data[6] = {0}; // instead of getting X0, X1, Y0, ... data separately, simply get all 6 bytes in one read. According to the specification, this removes the risk of the data in those registers changing between reads

  //for (int i = 0; i < 5; i ++) {
    WireWriteByte(ACCLADDR, 0x32); 
    WireRequestArray(ACCLADDR, data, 6);

		// X0 is the LSB and X1 is the MSB, and the data is stored right-shifted, so arrange the bytes accordingly
    uint16_t xi = (data[1] << 8) | data[0];
    uint16_t yi = (data[3] << 8) | data[2];
    uint16_t zi = (data[5] << 8) | data[4];
    
		// the acceleration data should be in signed form
    float x = *(int16_t*)(&xi); 
    float y = *(int16_t*)(&yi);
    float z = *(int16_t*)(&zi);

    x = x*ALPHA+(prevX*(1-ALPHA));
    y = y*ALPHA+(prevY*(1-ALPHA));
    z = z*ALPHA+(prevZ*(1-ALPHA));

    prevX = x;
    prevY = y;
    prevZ = z;

    pitch = (atan2(x,sqrt(y*y+z*z)) * 180.0) / M_PI;
    roll = (atan2(y,sqrt(x*x+z*z)) * 180.0) / M_PI;
  //}
  //pitch /= 5;
  //roll /= 5;

  pitch -= pitchOffset;
  roll -= rollOffset;
}
