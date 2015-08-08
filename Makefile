obj-m := block_led_trigger.o

KDIR  := /lib/modules/$(shell uname -r)/build
PWD   := $(shell pwd)

# This is a very ugly hack to gain access to the "block_rq_issue" tracepoint,
# which is unfortunately not exported by the kernel (as of kernel 4.1.3).
# Here we grab its address from the kernel's memory map file and we hardcode it
# in our module. Another approach could have been to just query the symbol
# table at runtime, but that would not give us the benefits of type checking.
EXTRA_CFLAGS += $(shell grep __tracepoint_block_rq_issue $(KDIR)/System.map | \
	sed 's/\([^ ]*\).*/-D__tracepoint_block_rq_issue_address=0x\1/')

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
