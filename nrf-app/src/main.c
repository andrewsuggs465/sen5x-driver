#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/sen5x.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);
static const struct {
    int chan;
    const char *name;
    const char *unit;
} channels[] = {
	{ SENSOR_CHAN_PM_1_0,          "PM1.0", "ug/m3" },
	{ SENSOR_CHAN_PM_2_5,          "PM2.5", "ug/m3" },
	{ SENSOR_CHAN_SEN5X_PM_4_0,    "PM4.0", "ug/m3" },
	{ SENSOR_CHAN_PM_10,           "PM10",  "ug/m3" },
	{ SENSOR_CHAN_HUMIDITY,        "RH",    "%"     },
	{ SENSOR_CHAN_AMBIENT_TEMP,    "Temp",  "C"     },
	{ SENSOR_CHAN_SEN5X_VOC_INDEX, "VOC",   ""      },
	{ SENSOR_CHAN_SEN5X_NOX_INDEX, "NOx",   ""      },
};
int main(void)
{
        const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(sen54));

        int ret;

        if(!device_is_ready(dev)){
            printk("SEN5x not ready\n");
            return 0;
        }

        printk("SEN5x ready: %s\n", dev->name);

        while(1) {
            k_sleep(K_SECONDS(1));

            ret = sensor_sample_fetch(dev);
            if (ret) {
                printk("fetch failed (%d)", ret);
                continue;
            }
            
            for (size_t i = 0; i< ARRAY_SIZE(channels); i++) {
                struct sensor_value val;
                
                ret = sensor_channel_get(dev, channels[i].chan, &val);
                if (ret == -ENODATA) {
                    LOG_INF("%s: n/a", channels[i].name);
                } else if (ret) {
                    LOG_WRN("%s: get failed (%d)", channels[i].name, ret);
                } else {
                    LOG_INF("%s: %.2f %s", channels[i].name, sensor_value_to_double(&val), channels[i].unit);
                }
            }

            
        }
        return 0;
}
