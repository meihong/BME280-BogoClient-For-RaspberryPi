#include <stdio.h>
#include <stdlib.h>
#include "bme280.h"

#define ALTITUDE    68.0
#define I2C_DEVICE  "/dev/i2c-1"
#define I2C_ADDRESS 0x76

int main(void)
{
  BME280* sensor = new BME280((char*)I2C_DEVICE, I2C_ADDRESS, ALTITUDE);

  if (sensor->setup()) {
    fprintf(stderr, "Failed to read data from sensor.\n");
    exit(1);
  }

  if (sensor->measure()) {
    fprintf(stderr, "Failed to read data from sensor.\n");
    exit(1);
  } else {
    printf("Temperature: %.2f C\n", sensor->temperature);
    printf("Pressure:    %.2f hPa (@Curr. Alt. %.2f hPa)\n",
           sensor->pressureAtSeaLevel,
           sensor->pressure);
    printf("Humidity:    %.2f %%\n", sensor->humidity);
  }
}
