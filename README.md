# gpio-value-hw-config

Project ORCA7 v2 - Android 7.1.2

## Add to source code

1. <b>Directory</b>
```
{android_build}/kernel/drivers/gpio/gpio-value-hw-config.c
```

2. <b>DEVICE TREE</b>
```
	gpio-value-hw-cfg{
		compatible = "orca7,v2,hwconfig";
		status = "okay";

		interrupt-controller;

		hw-cfg {
				pin0 = <&msm_gpio 68 1>;
				pin1 = <&msm_gpio 69 1>;
				pin2 = <&msm_gpio 88 1>;
				pin3 = <&msm_gpio 89 1>;
		};

	};
```

3. <b>Defconfig</b>
```
#8. get value of hardware configuration
CONFIG_GPIO_VALUE_HW_CONFIG=y
```

4. <b>Kconfig</b>
```
config GPIO_VALUE_HW_CONFIG
	bool "gpio_value_hw_config"
	help
	  gpio get value of Hardware Configuration
```

5. <b>Makefile</b>
```
obj-$(CONFIG_GPIO_VALUE_HW_CONFIG)   += gpio-value-hw-config.o
```

## Define pin number of gpio

* Pin 0: GPIO_68
* Pin 1: GPIO_69
* Pin 2: GPIO_88
* Pin 3: GPIO_89

## How to use

### Makefile

Build-in kernel with boot image

```
make -j4 bootimage
```

or build a full image

```
make -j4
```

Then boot/fastbot to board

### Get value

```
cat /sys/class/gpio-hw-cfg/hw-cfg/show_HW_CFG
```

<b>Expect Result</b>

0:0:0:0 => Pin3:Pin2:Pin1:Pin0 => Decimal Number

Number should be in range [0:15] <=> [0000:1111], and if Number = -1, it failed.

<i>Example</i>

Number = 3 => 0011 => P3=0; P2=0; P1=1; P0=1
