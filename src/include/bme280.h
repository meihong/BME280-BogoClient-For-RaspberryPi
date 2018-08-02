#include <math.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

class BME280
{
private:
  unsigned short digT1;
  signed short   digT2;
  signed short   digT3;
  unsigned short digP1;
  signed short   digP2;
  signed short   digP3;
  signed short   digP4;
  signed short   digP5;
  signed short   digP6;
  signed short   digP7;
  signed short   digP8;
  signed short   digP9;
  unsigned char  digH1;
  signed short   digH2;
  unsigned char  digH3;
  signed short   digH4;
  signed short   digH5;
  signed char    digH6;

  unsigned short rawHumidity;
  unsigned long  rawTemperature;
  unsigned long  rawPressure;

  int32_t  t_fine;

  const char* device;
  unsigned int fd;

  int readChunk(__u8, int, unsigned short*);
  int readCompensationData(void);
  int readRawData(void);
  void calcTemperature(void);
  void calcPressure(void);
  void calcHumidity(void);

 public:
  unsigned char address;

  double altitude;

  double temperature;
  double pressure;
  double pressureAtSeaLevel;
  double humidity;

  BME280(char* device, unsigned int address, double altitude);
  int setup(void);
  int measure(void);
};
