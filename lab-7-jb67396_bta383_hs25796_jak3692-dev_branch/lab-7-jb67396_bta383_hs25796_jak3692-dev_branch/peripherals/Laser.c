// Laser.c
#include <stdint.h>
#include "PLL.h"
#include "ST7735.h"
#include "VL53L0X.h"

VL53L0X_RangingMeasurementData_t measurement;

int LaserInit(void) {
	/*-- VL53L0X Init --*/
	if(!VL53L0X_Init(VL53L0X_I2C_ADDR)) {
		return 0;
	} else {
		return 1;
	}
}
    
uint16_t getMeasure(int index)
{
	VL53L0X_getSingleRangingMeasurement(&measurement);
	if (measurement.RangeStatus != 4)
	{
		return measurement.RangeMilliMeter;
	}
	else 
		return 65535;
}


