#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/sen5x.h>

int main(void)
{
        const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(sen55));

        if(!device_is_ready(dev)){
            printk("SEN5x not ready\n");
            return 0;
        }

        printk("SEN5x ready: %s\n", dev->name);
        return 0;
}
