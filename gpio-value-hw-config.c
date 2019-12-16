#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>       // for fs function like alloc_chrdev_region / operation file
#include <linux/types.h>
#include <linux/device.h>   // for device_create and class_create
#include <linux/uaccess.h>  // for copy to/from user function
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/of.h>       // access device tree file
#include <linux/delay.h>
#include <linux/slab.h>     // kmalloc, kcallloc, ....
#include <linux/string.h>
#include <linux/of_gpio.h>

#define DRIVER_NAME "gpio-hw-cfg"
#define FIRST_MINOR 0
#define BUFF_SIZE 100

#define PDEBUG(fmt,args...) printk(KERN_DEBUG"%s: "fmt,DRIVER_NAME, ##args)
#define PERR(fmt,args...) printk(KERN_ERR"%s: "fmt,DRIVER_NAME,##args)
#define PINFO(fmt,args...) printk(KERN_INFO"%s: "fmt,DRIVER_NAME, ##args)

typedef struct gpio_data{
    unsigned gpio;
} gpio_data_t;

typedef struct dev_private_data {
    struct device *dev;
    const char *name;
    gpio_data_t pin0;
    gpio_data_t pin1;
    gpio_data_t pin2;
    gpio_data_t pin3;
} dev_private_data_t;

typedef struct platform_private_data{
    struct class * dev_class;
    int num_pins;
    dev_private_data_t devices [];
} platform_private_data_t;


static inline int sizeof_platform_data(int num_pins)
{
	return sizeof(platform_private_data_t) +
		(sizeof(dev_private_data_t) * num_pins);
}

/***********************************/
/***** define device attribute *****/
/***********************************/

int getValue(dev_private_data_t *data)
{
    int result = -1;
    result = gpio_get_value(data->pin0.gpio)*1;
    result += gpio_get_value(data->pin1.gpio)*2;
    result += gpio_get_value(data->pin2.gpio)*4;
    result += gpio_get_value(data->pin3.gpio)*8;

    return result;
}

static ssize_t show_HW_CFG_show(struct device *dev, struct device_attribute *attr, char *buf)
{ 
    int res;
    dev_private_data_t *data = dev_get_drvdata(dev);
    if (!data)
        PERR("Can't get private data from device, pointer value: %p\n", data);

    res = scnprintf(buf, PAGE_SIZE, "%d\n", getValue(data));

    return res;
}

static struct device_attribute dev_class_attr[] = {
    __ATTR_RO(show_HW_CFG),
    __ATTR_NULL,
};

/***************************/
/*****module init + exit****/
/***************************/

static int driver_probe (struct platform_device *pdev)
{
    int num_pins;
    struct device_node *np = pdev->dev.of_node;
    struct device_node *child ;
    platform_private_data_t *data;

    PINFO ("Driver module hwcrg init\n");
    PINFO ("Node name %s\n",pdev->dev.of_node->name );

    // create private data
    num_pins = of_get_child_count(np);
    if (!num_pins)
		return -ENODEV;

    data = (platform_private_data_t*)kcalloc(1, sizeof_platform_data(num_pins), GFP_KERNEL);
    data->num_pins = 0;

    // // create class 
    data->dev_class = class_create(THIS_MODULE, DRIVER_NAME);
    if (IS_ERR(data->dev_class))
    {
        PERR("Class create fail, error code: %d\n", (int)data->dev_class);

        goto error_class;
    }
    data->dev_class->dev_attrs = dev_class_attr;
    
    for_each_child_of_node(np, child) {
        dev_private_data_t *device = &data->devices[data->num_pins++];
        
		device->name = of_get_property(child, "label", NULL) ? : child->name;

        // get gpio properties and create device

        device->dev = device_create(data->dev_class, &pdev->dev, 0, device, "%s", device->name);
        if (IS_ERR(device->dev))
        {
            PERR("device for %s create fall, error code: %d\n", device->name, (int)device->dev);

            goto error_device;
        }

        // get gpio number from device tree
        device->pin0.gpio = of_get_named_gpio(child, "pin0", 0);
        if (!device->pin0.gpio)
        {
            PERR ("can't get gpio PIN_0 from %s, error code: %d\n", device->name, device->pin0.gpio);

            goto error_pin0;
        } 

        device->pin1.gpio = of_get_named_gpio(child, "pin1", 0);
        if (!device->pin1.gpio)
        {
            PERR ("can't get gpio PIN_1 from %s, error code: %d\n", device->name, device->pin1.gpio);

            goto error_pin1;
        } 

        device->pin2.gpio = of_get_named_gpio(child, "pin2", 0);
        if (!device->pin2.gpio)
        {
            PERR ("can't get gpio PIN_2 from %s, error code: %d\n", device->name, device->pin2.gpio);

            goto error_pin2;
        } 

        device->pin3.gpio = of_get_named_gpio(child, "pin3", 0);
        if (!device->pin3.gpio)
        {
            PERR ("can't get gpio PIN_3 from %s, error code: %d\n", device->name, device->pin3.gpio);

            goto error_pin3;
        } 

        // request gpio and init
        if (!gpio_is_valid(device->pin0.gpio))
        {
            PINFO ("Gpio number 0 is invalid \n");

            goto error_gpio_init;
        }
        else devm_gpio_request_one(device->dev, device->pin0.gpio, GPIOF_IN, "pin0");

        if (!gpio_is_valid(device->pin1.gpio))
        {
            PINFO ("Gpio number 1 is invalid \n");

            goto error_gpio_init;
        }
        else devm_gpio_request_one(device->dev, device->pin1.gpio, GPIOF_IN, "pin1");

        if (!gpio_is_valid(device->pin2.gpio))
        {
            PINFO ("Gpio number 2 is invalid \n");

            goto error_gpio_init;
        }
        else devm_gpio_request_one(device->dev, device->pin2.gpio, GPIOF_IN, "pin2");

        if (!gpio_is_valid(device->pin3.gpio))
        {
            PINFO ("Gpio number 3 is invalid \n");

            goto error_gpio_init;
        }
        else devm_gpio_request_one(device->dev, device->pin3.gpio, GPIOF_IN, "pin3");

	    PINFO("device %s configuration : \n", device->name);
        PINFO("\tgpio_number_0: %d\n", device->pin0.gpio);
        PINFO("\tgpio_number_1: %d\n", device->pin1.gpio);
        PINFO("\tgpio_number_2: %d\n", device->pin2.gpio);
        PINFO("\tgpio_number_3: %d\n", device->pin3.gpio);

        continue;

        error_gpio_init:
            gpio_free(device->pin3.gpio);
        error_pin3:
            gpio_free(device->pin2.gpio);
        error_pin2:
            gpio_free(device->pin1.gpio);
        error_pin1:
            gpio_free(device->pin0.gpio);
        error_pin0:
            device_unregister(device->dev);
        error_device:
            continue;
    }

    platform_set_drvdata(pdev, data);

    return 0;

    //error handle
error_class:
    return -1;

}

static int driver_remove(struct platform_device *pdev)
{
    platform_private_data_t *data = platform_get_drvdata(pdev);
    PINFO("Driver module remove from kernel\n");
    class_destroy(data->dev_class);
    kfree(data);
    platform_set_drvdata(pdev, NULL);

    return 0;
}

static const struct of_device_id hwcfg_dst[]={
    { .compatible = "orca7,v2,hwconfig", },
    {}
};

MODULE_DEVICE_TABLE(of, hwcfg_dst);	

static struct platform_driver gpio_hwconfig = {
    .driver = {
        .name = DRIVER_NAME,
        .owner = THIS_MODULE,   
        .of_match_table = of_match_ptr (hwcfg_dst),
    },
    .probe = driver_probe,
    .remove = driver_remove,
};

module_platform_driver(gpio_hwconfig);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Le Phuong Nam <le.phuong.nam@styl.solutions>");