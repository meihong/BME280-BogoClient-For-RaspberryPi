#include "bme280.h"

int BME280::readChunk(__u8 address, int size, unsigned short* buffer)
{
  __s32 readed;

  for (int i = 0; i < size; i++) {
    readed = i2c_smbus_read_byte_data(fd, address + i);
    if (readed < 0) {
      return -1;
    }
    buffer[i] = (unsigned short)(readed) & 0xff;
  }
  return 0;
}

int BME280::readCompensationData(void)
{
  int32_t readed;
  unsigned short compensation[26];
  int i;

  if (readChunk(0x88, 26, compensation) < 0) {
    return -1;
  }
  digT1 = (compensation[1]  << 8) | compensation[0];
  digT2 = (compensation[3]  << 8) | compensation[2];
  digT3 = (compensation[5]  << 8) | compensation[4];
  digP1 = (compensation[7]  << 8) | compensation[6];
  digP2 = (compensation[9]  << 8) | compensation[8];
  digP3 = (compensation[11] << 8) | compensation[10];
  digP4 = (compensation[13] << 8) | compensation[12];
  digP5 = (compensation[15] << 8) | compensation[14];
  digP6 = (compensation[17] << 8) | compensation[16];
  digP7 = (compensation[19] << 8) | compensation[18];
  digP8 = (compensation[21] << 8) | compensation[20];
  digP9 = (compensation[23] << 8) | compensation[22];
  digH1 = (unsigned char)compensation[24];

  if (readChunk(0xe1, 7, compensation) < 0) {
    return -1;
  }
  digH2 = (compensation[1] << 8) | compensation[0];
  digH3 = (unsigned char)compensation[2];
  digH4 = (compensation[3] << 4) | (compensation[4] & 0x0f);
  digH5 = (compensation[5] << 4) | ((compensation[4] >> 4) & 0x0f);
  digH6 = (unsigned char)compensation[6];

  return 0;
}

int BME280::readRawData(void)
{
  int32_t readed;
  unsigned char data[7];

  for (int i = 0; i < 8; i++) {
    readed = i2c_smbus_read_byte_data(fd, 0xf7 + i);
    if (readed < 0) {
      return -1;
    }
    data[i] = (unsigned char)(readed & 0xff);
  }

  rawPressure    = ((unsigned long)data[0] << 12)
    | ((unsigned long)data[1] << 4)
    | ((unsigned long)data[2] >> 4);
  rawTemperature = ((unsigned long)data[3] << 12)
    | ((unsigned long)data[4] << 4)
    | ((unsigned long)data[5] >> 4);
  rawHumidity    = ((unsigned short)data[6] << 8)
    | (unsigned short)data[7];

  return 0;
}

void BME280:: calcTemperature(void)
{
  t_fine =
    /* var1 */
    (((((rawTemperature >> 3) - ((int32_t)digT1 << 1)))
      * ((int32_t)digT2)) >> 11)
    +
    /* var2 */
    ((((
	((rawTemperature >> 4) - ((int32_t)digT1))
	*
	((rawTemperature >> 4) - ((int32_t)digT1))
	) >> 12
       )
      * ((int32_t)digT3)) >> 14);

  temperature = (double)((t_fine * 5 + 128) >> 8) / 100;
}

void BME280::calcPressure(void)
{
  int64_t var1, var2, p;

  var1 = (int64_t)t_fine - 128000;
  var2 = var1 * var1 * (int64_t)digP6;
  var2 = var2 + ((var1 * (int64_t)digP5) << 17);
  var2 = var2 + (((int64_t)digP4) << 35);
  var1 = ((var1 * var1 * (int64_t)digP3) >> 8)
    + ((var1 * (int64_t)digP2) << 12);
  var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)digP1) >> 33;

  if (var1 == 0) {
    pressure = 0;
  } else {
    p = 1048576 - (int64_t)rawPressure;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)digP9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)digP8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)digP7) << 4);

    pressure = (double)p / 25600;
    pressureAtSeaLevel = pressure
      * pow(
	    1 - (0.0065 * altitude)
	    / (temperature + 0.0065 * altitude + 273.15),
	    -5.257
	    );
  }
}

void BME280::calcHumidity(void)
{
  int32_t v_x1_u32r;

  v_x1_u32r = ((int32_t)t_fine - ((int32_t)76800));
  v_x1_u32r = (((((rawHumidity << 14) - ((int32_t)digH4 << 20) - (((int32_t)digH5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) * (((((((v_x1_u32r * ((int32_t)digH6)) >> 10) * (((v_x1_u32r * ((int32_t)digH3)) >> 11) + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)digH2) + 8192) >> 14));
  v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)digH1)) >> 4));

  v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
  v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);

  humidity = (double)(__u32)(v_x1_u32r >> 12) / 1024;
}

BME280::BME280(char* dev, unsigned int ad, double alt) {
  device   = dev;
  address  = ad;
  altitude = alt;
}

int BME280::setup (void)
{
  if ((fd = open((const char*)device, O_RDWR)) < 0) {
    return -1;
  }

  if (ioctl(fd, I2C_SLAVE, address) < 0) {
    return -1;
  }

  if (i2c_smbus_write_byte_data(fd, 0xf2, 0x03) < 0) {
    return -1;
  }
  if (i2c_smbus_write_byte_data(fd, 0xf4, 0x6f) < 0) {
    return -1;
  }
  if (i2c_smbus_write_byte_data(fd, 0xf5, 0xa8) < 0) {
    return -1;
  }

  if (readCompensationData() < 0) {
    return -1;
  }

  return 0;
}

int BME280::measure(void)
{
  if (readRawData() < 0) {
    return -1;
  } else {
    calcTemperature();
    calcPressure();
    calcHumidity();
  }
}
