# block-led-trigger
Linux kernel module to blink LEDs on block device activity

## Description
My new ThinkPad W541 laptop has no hard disk activity LED, but just a single
power LED that can thankfully be controlled in software (through Linux's LED
infrastructure). I have been missing the HD LED since the moment I unboxed
the laptop and, since the power LED's default always-on function is basically
useless, I have decided to turn it into an hard disk activity LED.

Linux has a great LED infrastructure and the feature I wanted was already
available for IDE disks (see Linux's `drivers/leds/trigger/ledtrig-ide-disk.c`).
Unfortunately that code only works if you are using the old `ide` driver (that
is, if your hard disk shows up as a `/dev/hdX` device). In virtually all modern
distributions (including my Fedora 22) hard disks are now `/dev/sdX` because
they are controlled by different drivers and, to my knowledge, there is no LED
support in these drivers.

This repository contains a Linux kernel module that implements a
`block-activity` LED trigger that can be assigned to LEDs as follows:

```
# echo block-activity > /sys/class/leds/tpacpi\:\:power/trigger
```

## Usage
This module has only been tested on *Fedora 22 x86_64*, kernel version _4.1.3_.

After compiling the kernel module (with a simple in-source `make`), load it
with:

```
# insmod block_led_trigger.ko devices=sda,sdb
```

_(see the next paragraph for the available command-line arguments)_

Choose the LED you want to control (you can browse available
LEDs in the `/sys/class/leds/` directory) and check that it can actually be
controlled by `echo`'ing a few `0` or `1` values to its `brightness` file.
Finally, assign the `block-activity` LED trigger with:

```
# echo block-activity > /sys/class/leds/$YOUR_LED_HERE/trigger
```

## Module parameters
`blink_delay_ms` (default: _30_) is the blink interval in milliseconds.

`devices` (default: _sda_) is a comma-separated list of devices to monitor. The
LED will blink whenever one of those devices is servicing an I/O operation.

`invert_brightness` (default: _N_) lets you invert the LED output state. If _N_
is selected, the LED will stay switched off if there is no I/O activity; if _Y_
is selected, the LED will stay switched on.

## Warnings
This module uses the `block_rq_issue` tracepoint defined by the Linux kernel,
which is exposed to userland programs through the _blktrace_ API.

Unfortunately, it is not exposed to kernel modules (in particular, the
`__tracepoint_block_rq_issue` variable is not exported and therefore it cannot
be resolved by the module loader at runtime). For this reason, the Makefile
contains a `grep` command to extract its address from the kernel memory map
(_System.map_) and resolve its address at compile time.

The generated kernel module is therefore very dependent on the memory layout of
the kernel it was compiled against. If you try to load the compiled module on a
different kernel binary (even if it's the very same kernel version), the address
of that variable will likely be different and the module will refuse to load.


## RPM package and systemd service
The repository also contains a `make-rpm.sh` script that generates an RPM
package containing a systemd service that:
 1. compiles the module against the running kernel
 2. loads it and assigns the power LED (you may want to tweak the
    `block-led-trigger` script)

The RPM package can be installed and started as follows:

```
$ ./make-rpm.sh
# dnf install block-led-trigger-0.1-1.fc22.noarch.rpm
# systemctl start block-led-trigger
```

To run the service automatically on boot:

```
# systemctl enable block-led-trigger
```

Note that the service script will rebuild the module on each boot. This
operation takes very little time on my ThinkPad, but it could be probably
be avoided by creating proper `akmods`/`dkms` packages.
