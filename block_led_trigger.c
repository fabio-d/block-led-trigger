#include <linux/init.h>
#include <linux/kallsyms.h>
#include <linux/kernel.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/stringify.h>
#include <trace/events/block.h>

#define MAX_DEVICES	16

DEFINE_LED_TRIGGER(ledtrig_block);

static bool invert_brightness = false;
module_param(invert_brightness, bool, S_IRUGO | S_IWUSR);

static unsigned long blink_delay_ms = 30;
module_param(blink_delay_ms, ulong, S_IRUGO | S_IWUSR);

static unsigned int device_count = 1;
static dev_t device_dev[MAX_DEVICES];
static char *device_names[MAX_DEVICES] = { "sda" };
module_param_array_named(devices, device_names, charp, &device_count, 0);

// Hack to gain access to the "block_rq_issue" tracepoint (see Makefile)
static bool __tracepoint_block_rq_issue_address_found = false;
static bool __tracepoint_block_rq_issue_address_mismatch = false;
#ifdef __tracepoint_block_rq_issue_address
asm(".equ __tracepoint_block_rq_issue, " __stringify(__tracepoint_block_rq_issue_address));
#else
#error __tracepoint_block_rq_issue address could not be detected (see Makefile)
#endif

// Tracepoint callback: called every time an I/O request is sent to a block device
static void trace_rq_issue(void *ignore, struct request_queue *q, struct request *rq)
{
	bool device_matched = false;
	int i;

	// Sometimes we get null requests... :(
	if (rq == NULL || rq->rq_disk == NULL)
		return;

	// Filter out requests to devices that are not listed in the "devices" parameter
	for (i = 0; i < device_count && !device_matched; i++)
	{
		if (MKDEV(rq->rq_disk->major, rq->rq_disk->first_minor) == device_dev[i])
			device_matched = true;
	}

	if (!device_matched)
		return;

	// Let's blink!
	led_trigger_blink_oneshot(ledtrig_block, &blink_delay_ms, &blink_delay_ms, invert_brightness);
}

static int __init symbol_callback(void *data, const char *name, struct module *mod, unsigned long addr)
{
	if (mod == NULL && strcmp(name, "__tracepoint_block_rq_issue") == 0)
	{
		__tracepoint_block_rq_issue_address_found = true;

		if (addr != __tracepoint_block_rq_issue_address)
			__tracepoint_block_rq_issue_address_mismatch = true;
	}

	return 0;
}

static int __init block_led_trigger_init(void)
{
	int ret, i;

	printk(KERN_DEBUG "block_led_trigger: loading\n");

	// Sanity check: verify that the hardcoded __tracepoint_block_rq_issue
	// address is correct for the running kernel
	ret = kallsyms_on_each_symbol(symbol_callback, NULL);
	if (ret != 0)
	{
		printk(KERN_ERR "block_led_trigger: kallsyms_on_each_symbol failed, return code: %d\n",
			ret);
		return ret;
	}

	if (!__tracepoint_block_rq_issue_address_found)
	{
		printk(KERN_ERR "block_led_trigger: __tracepoint_block_rq_issue: symbol not found\n");
		return -EFAULT;
	}

	if (__tracepoint_block_rq_issue_address_mismatch)
	{
		printk(KERN_ERR "block_led_trigger: __tracepoint_block_rq_issue: symbol address mismatch\n");
		printk(KERN_ERR "block_led_trigger: please recompile this module against the running kernel\n");
		return -EFAULT;
	}

	// Translate device names (such as "sda") to device major/minor (such as 8:0)
	for (i = 0; i < device_count; i++)
	{
		device_dev[i] = blk_lookup_devt(device_names[i], 0);

		if (!device_dev[i])
		{
			printk(KERN_ERR "block_led_trigger: %s cannot be resolved\n",
				device_names[i]);
			return -ENODEV;
		}

		printk(KERN_DEBUG "block_led_trigger: %s resolved to device %u:%u\n",
		       device_names[i], MAJOR(device_dev[i]), MINOR(device_dev[i]));
	}

	// Initialize LED trigger
	led_trigger_register_simple("block-activity", &ledtrig_block);
	led_trigger_event(ledtrig_block, invert_brightness ? LED_FULL : LED_OFF);

	// Register our tracepoint
	ret = register_trace_block_rq_issue(trace_rq_issue, NULL);
	if (ret != 0)
	{
		printk(KERN_ERR "block_led_trigger: register_trace_block_rq_issue failed, return code: %d\n",
			ret);
		return ret;
	}

	printk(KERN_INFO "block_led_trigger: loaded successfully\n");
	return 0;
}

static void __exit block_led_trigger_exit(void)
{
	unregister_trace_block_rq_issue(trace_rq_issue, NULL);
	tracepoint_synchronize_unregister();

	led_trigger_unregister_simple(ledtrig_block);

	printk(KERN_INFO "block_led_trigger: unloaded\n");
}

module_init(block_led_trigger_init);
module_exit(block_led_trigger_exit);

MODULE_AUTHOR("Fabio D'Urso <fabiodurso@hotmail.it>");
MODULE_DESCRIPTION("Block device activity LED trigger");
MODULE_LICENSE("GPL");
