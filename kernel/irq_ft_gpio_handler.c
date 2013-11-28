#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/gpio.h>

// Module parameters
static int ft_gpio_out = 22;
static int ft_gpio_in = 23;

module_param(ft_gpio_out, int, 0);
MODULE_PARM_DESC(ft_gpio_out, "Output GPIO number");
module_param(ft_gpio_in, int, 0);
MODULE_PARM_DESC(ft_gpio_in, "Input GPIO number");


// Interruput handler 
// Reverse gpio out when an interrupt occurs on gpio in
static irqreturn_t irq_ft_gpio_handler(int irq, void* ident)
{
	static int output_value;

	gpio_set_value(ft_gpio_out, output_value);

	output_value = 1 - output_value;

	return IRQ_HANDLED;
}

// Module loading 
static int __init module_loading(void)
{
	int err;

	printk(KERN_INFO "Loading ft_gpio_handler\n");

	// Request GPIO in
	if((err=gpio_request(ft_gpio_in, THIS_MODULE->name)) != 0){
		return err;
	}

	// Request GPIO out
	if((err=gpio_request(ft_gpio_in, THIS_MODULE->name)) != 0){
		gpio_free(ft_gpio_in);
		return err;
	}

	// Configure GPIO in
	if((err=gpio_direction_input(ft_gpio_out)) != 0){
		gpio_free(ft_gpio_in);
		gpio_free(ft_gpio_out);
		return err;
	}

	// Configure GPIO out
	if((err=gpio_direction_output(ft_gpio_in, 1)) != 0){
		gpio_free(ft_gpio_in);
		gpio_free(ft_gpio_out);
		return err;
	}

	// Install interrupt handler on GPIO in
	if((err=request_irq(gpio_to_irq(ft_gpio_in),
								irq_ft_gpio_handler,
								IRQF_SHARED | IRQF_TRIGGER_RISING,
								THIS_MODULE->name,
								THIS_MODULE->name)) != 0){
		gpio_free(ft_gpio_in);
		gpio_free(ft_gpio_out);
		return err;
	}

	return 0;
}

// Module unloading 
static void __exit module_unloading(void)
{
	printk(KERN_INFO "Unloading gpio_ft_irq_handler\n");

	// Free irq
	free_irq(gpio_to_irq(ft_gpio_in), THIS_MODULE->name);

	// Free GPIO
	gpio_free(ft_gpio_in);
	gpio_free(ft_gpio_out);
}

module_init(module_loading);
module_exit(module_unloading);
MODULE_LICENSE("GPL");

