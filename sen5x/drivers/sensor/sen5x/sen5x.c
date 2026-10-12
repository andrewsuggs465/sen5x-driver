/*`
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sensirion_sen5x

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/sen5x.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(SEN5X, CONFIG_SENSOR_LOG_LEVEL);

struct sen5x_config {
	struct i2c_dt_spec bus;
};

struct sen5x_data {
	uint16_t raw[8];
};

static int sen5x_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	/* TODO */
	return -ENOTSUP;
}

static int sen5x_channel_get(const struct device *dev, enum sensor_channel chan,
			     struct sensor_value *val)
{
	/* TODO */
	return -ENOTSUP;
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

	/* TODO: send start measurement 0x0021 */
	return 0;
}

#define SEN5X_DEFINE(inst)                                                            \
	static struct sen5x_data sen5x_data_##inst;                                   \
	/* ROM config -- bus device plus target address*/			      \
	static const struct sen5x_config sen5x_config_##inst = {                      \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                    \
	};                                                                            \
	/* Macro for SENSOR_DEVICE_DT_DEFINE(DT_DRV_INST(inst), ... )*/               \
	SENSOR_DEVICE_DT_INST_DEFINE(inst,                          /*node_id*/       \
				     sen5x_init,                    /*init_fn*/       \
				     NULL,                          /*pm_device*/     \
				     &sen5x_data_##inst,            /*data_ptr*/      \
				     &sen5x_config_##inst,          /*cfg_ptr*/       \
				     POST_KERNEL,                   /*level*/	      \
				     CONFIG_SENSOR_INIT_PRIORITY,   /*Priority*/      \
				     &sen5x_api);                   /*api_ptr*/

DT_INST_FOREACH_STATUS_OKAY(SEN5X_DEFINE)
