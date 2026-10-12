#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/sen5x.h>

int main(void)
{
        const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(sen55));
        int ret;

        if(!device_is_ready(dev)){
            printk("SEN5x not ready\n");
            return 0;
        }

        printk("SEN5x ready: %s\n", dev->name);

        while(1) {
            ret = sen5x_sample_fetch(dev);
            if (ret) {
                printk("sensor_sample_fetch failed ret %d\n", ret);
                return 0;
            }
            k_sleep(K_MSEC(500));
        }
        return 0;
}
