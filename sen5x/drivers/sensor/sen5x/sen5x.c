/*
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sensirion_sen5x

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/sen5x.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>

LOG_MODULE_REGISTER(SEN5X, CONFIG_SENSOR_LOG_LEVEL);

struct sen5x_config {
	struct i2c_dt_spec bus;
};

struct sen5x_data {
	uint16_t raw[8];
};

static int sen5x_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct sen5x_config *cfg = dev->config;
	struct sen5x_data *data = dev->data;
	uint8_t cmd[2] = { 0x03, 0xC4 }; /* Read measured values */
	uint8_t buf[24]; /* 8 x (MSB, LSB, CRC) */
	int ret;

	ret = i2c_write_dt(&cfg->bus, cmd, sizeof(cmd));
	if (ret < 0){
		LOG_ERR("Read command failed (%d)", ret);
		return ret;
	}

	k_msleep(20);

	ret = i2c_read_dt(&cfg->bus, buf, sizeof(buf));
	if (ret < 0){
		LOG_ERR("Read failed (%d)", ret);
		return ret;
	}

	for (int i = 0; i < 8; i++) {
		/* Byte 0: PM1.0, 3: PM2.5, 6: PM4.0,
		9: PM10, 12: %RH, 15: Temp, 18: VOC, 21: NOx */
		uint8_t *word = &buf[i * 3];

		if(crc8(word, 2, 0x31, 0xFF, false) != word[2]) {
			LOG_WRN("CRC mismatch on word %d", i);
			return -EIO;
		}
		data->raw[i] = sys_get_be16(word);
	}
	LOG_DBG("raw: %04x %04x %04x %04x %04x %04x %04x %04x",
		data->raw[0], data->raw[1], data->raw[2], data->raw[3],
		data->raw[4], data->raw[5], data->raw[6], data->raw[7]);
	return 0;
}

/* PM values: unsigned, scale x10, 0xFFFF = no data */
static int sen5x_get_pm(struct sensor_value *val, uint16_t raw)
{
	if (raw == 0xFFFF) {
		return -ENODATA;
	}
	/* x10 -> milli: 300 -> 30000 milli = 30.0 ug/m3 */
	return sensor_value_from_milli(val, raw * 100);
}

/* RH, temp, VOC, NOx: signed, 0x7FFF = no data */
static int sen5x_get_signed(struct sensor_value *val, uint16_t raw, int milli_per_lsb)
{
	if (raw == 0x7FFF) {
		return -ENODATA;
	}
	return sensor_value_from_milli(val, (int16_t)raw *milli_per_lsb);
}

static int sen5x_channel_get(const struct device *dev, enum sensor_channel chan,
			     struct sensor_value *val)
{
	struct sen5x_data *data = dev->data;

	switch ((int)chan) {
	case SENSOR_CHAN_PM_1_0:
		return sen5x_get_pm(val, data->raw[0]);
	case SENSOR_CHAN_PM_2_5:
		return sen5x_get_pm(val, data->raw[1]);
	case SENSOR_CHAN_SEN5X_PM_4_0:
		return sen5x_get_pm(val, data->raw[2]);
	case SENSOR_CHAN_PM_10:
		return sen5x_get_pm(val, data->raw[3]);
	case SENSOR_CHAN_HUMIDITY:
		/* x100 -> 1 LSB = 10 milli-%RH */
		return sen5x_get_signed(val, data->raw[4], 10);
	case SENSOR_CHAN_AMBIENT_TEMP:
		/* x200 -> 1 LSB = 5 milli-C */
		return sen5x_get_signed(val, data->raw[5], 5);
	case SENSOR_CHAN_SEN5X_VOC_INDEX:
		/* x10 -> 1 LSB = 100 milli */
		return sen5x_get_signed(val, data->raw[6], 100);
	case SENSOR_CHAN_SEN5X_NOX_INDEX:
		/* x10 -> 1 LSB = 100 milli */
		return sen5x_get_signed(val, data->raw[7], 100);
	default:
		return -ENOTSUP;
	}
}

static DEVICE_API(sensor, sen5x_api) = {
	.sample_fetch = sen5x_sample_fetch,
	.channel_get = sen5x_channel_get,
};

static int sen5x_init(const struct device *dev)
{
	const struct sen5x_config *cfg = dev->config;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("I2C bus %s not ready", cfg->bus.bus->name);
		return -ENODEV;
	}

	/* Send start measurement: 0x0021 */
	uint8_t cmd[2] = {0x00, 0x21};
	int ret;

	ret = i2c_write_dt(&cfg->bus, cmd, sizeof(cmd));
	if(ret < 0){
		LOG_ERR("Start Measurement failed! (%d)", ret);
		return ret;
	}

	return 0;
}

#define SEN5X_DEFINE(inst)                                                      \
	static struct sen5x_data sen5x_data_##inst;                                   \
	/* ROM config -- bus device plus target address */		                        \
	static const struct sen5x_config sen5x_config_##inst = {                      \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                          \
	};                                                                            \
	/* Macro for SENSOR_DEVICE_DT_DEFINE(DT_DRV_INST(inst), ... ) */              \
	SENSOR_DEVICE_DT_INST_DEFINE(inst,        /*node_id*/                         \
				     sen5x_init,                    /*init_fn*/                         \
				     NULL,                          /*pm_device*/                       \
				     &sen5x_data_##inst,            /*data_ptr*/                        \
				     &sen5x_config_##inst,          /*cfg_ptr*/                         \
				     POST_KERNEL,                   /*level*/	                          \
				     CONFIG_SENSOR_INIT_PRIORITY,   /*Priority*/                        \
				     &sen5x_api);                   /*api_ptr*/

DT_INST_FOREACH_STATUS_OKAY(SEN5X_DEFINE)
