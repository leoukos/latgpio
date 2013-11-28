#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/gpio.h>


#define FT_GPIO_OUT 22
#define FT_GPIO_IN 23

// Interruput handler 
// Reverse gpio out when an interrupt occurs on gpio in
static irqreturn_t irq_ft_gpio_handler(int irq, void* ident)
{
	static int output_value;

	gpio_set_value(FT_GPIO_OUT, output_value);

	output_value = 1 - output_value;

	return IRQ_HANDLED;
}

// Module loading 
static int __init module_loading(void)
{
	int err;

	printk(KERN_INFO "Loading ft_gpio_handler\n");

	// Request GPIO in
	if((err=gpio_request(FT_GPIO_IN, THIS_MODULE->name)) != 0){
		return err;
	}

	// Request GPIO out
	if((err=gpio_request(FT_GPIO_IN, THIS_MODULE->name)) != 0){
		gpio_free(FT_GPIO_IN);
		return err;
	}

	// Configure GPIO in
	if((err=gpio_direction_input(FT_GPIO_OUT)) != 0){
		gpio_free(FT_GPIO_IN);
		gpio_free(FT_GPIO_OUT);
		return err;
	}

	// Configure GPIO out
	if((err=gpio_direction_output(FT_GPIO_IN, 1)) != 0){
		gpio_free(FT_GPIO_IN);
		gpio_free(FT_GPIO_OUT);
		return err;
	}

	// Install interrupt handler on GPIO in
	if((err=request_irq(gpio_to_irq(FT_GPIO_IN),
								irq_ft_gpio_handler,
								IRQF_SHARED | IRQF_TRIGGER_RISING,
								THIS_MODULE->name,
								THIS_MODULE->name)) != 0){
		gpio_free(FT_GPIO_IN);
		gpio_free(FT_GPIO_OUT);
		return err;
	}

	return 0;
}

// Module unloading 
static void __exit module_unloading(void)
{
	printk(KERN_INFO "Unloading gpio_ft_irq_handler\n");

	// Free irq
	free_irq(gpio_to_irq(FT_GPIO_IN), THIS_MODULE->name);

	// Free GPIO
	gpio_free(FT_GPIO_IN);
	gpio_free(FT_GPIO_OUT);
}

module_init(module_loading);
module_exit(module_unloading);
MODULE_LICENSE("GPL");

