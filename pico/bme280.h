#include "pico/binary_info.h"
#include "hardware/spi.h"

int32_t compensate_temp(int32_t adc_T);
uint32_t compensate_pressure(int32_t adc_P);
uint32_t compensate_humidity(int32_t adc_H);
static inline void cs_select();
static void cs_deselect();
void write_register(uint8_t reg, uint8_t data);
void read_registers(uint8_t reg, uint8_t *buf, uint16_t len);
void bme280_read_raw(int32_t *humidity, int32_t *pressure, int32_t *temperature);
void read_compensation_parameters();
