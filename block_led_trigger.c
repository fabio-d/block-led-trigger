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
static char *device_devstr[MAX_DEVICES] = { "8:0" };
module_param_array_named(devices, device_devstr, charp, &device_count, 0);

// Hack to gain access to the "block_rq_issue" tracepoint (see Makefile)
#ifdef __tracepoint_block_rq_issue_address
# if __tracepoint_block_rq_issue_address != 0
asm(".equ __tracepoint_block_rq_issue, " __stringify(__tracepoint_block_rq_issue_address));
# else
#  error __tracepoint_block_rq_issue address is zero (are you non-root?)
# endif
#else
# error __tracepoint_block_rq_issue address could not be detected (see Makefile)
#endif

// Tracepoint callback: called every time an I/O request is sent to a block device
static void trace_rq_issue(void *ignore, struct request *rq)
{
	bool device_matched = false;
	int i;

	// Do not attempt to dereference null requests
	if (rq == NULL || rq->q == NULL || rq->q->disk == NULL)
		return;

	// Filter out requests to devices that are not listed in the "devices" parameter
	for (i = 0; i < device_count && !device_matched; i++)
	{
		if (MKDEV(rq->q->disk->major, rq->q->disk->first_minor) == device_dev[i])
			device_matched = true;
	}

	if (!device_matched)
		return;

	// Let's blink!
	led_trigger_blink_oneshot(ledtrig_block, &blink_delay_ms, &blink_delay_ms, invert_brightness);
}

static bool __init validate_tracepoint_address(void)
{
	static char expected[] = "__tracepoint_block_rq_issue+0x0/";
	static char actual[sizeof(expected)];

	printk(KERN_INFO "block_led_trigger: hardcoded address %p resolves to %pS\n", &__tracepoint_block_rq_issue, &__tracepoint_block_rq_issue);
	snprintf(actual, sizeof(actual), "%pS", &__tracepoint_block_rq_issue);

	return memcmp(actual, expected, sizeof(expected)) == 0;
}

static int __init block_led_trigger_init(void)
{
	int ret, i;

	printk(KERN_DEBUG "block_led_trigger: loading\n");

	if (!validate_tracepoint_address())
	{
		printk(KERN_ERR "block_led_trigger: please recompile this module against the running kernel\n");
		return -EFAULT;
	}

	// Parse device strings (such as "8:0") into device major/minor numbers
	for (i = 0; i < device_count; i++)
	{
		unsigned int major, minor;
		int endpos;
		if (sscanf(device_devstr[i], "%u:%u%n", &major, &minor, &endpos) != 2 || device_devstr[i][endpos] != '\0')
		{
			printk(KERN_ERR "block_led_trigger: %s cannot be parsed as a device in MAJOR:MINOR format\n",
				device_devstr[i]);
			return -EINVAL;
		}

		device_dev[i] = MKDEV(major, minor);
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
