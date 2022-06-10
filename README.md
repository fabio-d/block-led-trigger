# block-led-trigger
Linux kernel module to blink LEDs on block device activity

**Update**: Starting with version 4.8, Linux now has built-in support for
blinking LEDs (through the `disk-activity` trigger). However Linux's built-in
`disk-activity` trigger is not configurable, whereas this module enables the
user to specify what disks to monitor and a few other parameters.

## Description
My ThinkPad laptop has no hard disk activity LED, but just a single power LED
that can be controlled in software (through Linux's LED infrastructure). The
power LED's default always-on function is basically useless, and I have decided
to turn it into an hard disk activity LED.

This repository contains a Linux kernel module that implements a
`block-activity` LED trigger that can be assigned to LEDs as follows:

```
# echo block-activity > /sys/class/leds/tpacpi\:\:power/trigger
```

## Usage
This module has been tested on *Fedora 36 x86_64*, kernel version _5.17.13_.

Compile the kernel module (with a simple in-source `make`) **as root**, then
load it with:

```
# insmod block_led_trigger.ko devices=8:0,8:16
```

Note: This module has to be compiled as root because regular users cannot read
the actual contents of _/proc/kallsyms_. See the _Warnings_ section for an
explanation on why this file is needed at compile time.

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

`devices` (default: _8:0_) is a comma-separated list of devices to monitor. The
LED will blink whenever one of those devices is servicing an I/O operation.
Devices must be listed in `MAJOR:MINOR` format (e.g. _8:0_ for _sda_, _8:16_ for
_sdb_, _8:32_ for _sdc_, _8:48_ for _sdd_, and so on - if unsure, run
`ls -la /dev/sd?` to discover the values on your running system).

`invert_brightness` (default: _N_) lets you invert the LED output state. If _N_
is selected, the LED will stay switched off if there is no I/O activity; if _Y_
is selected, the LED will stay switched on.

## Warnings
This module uses the `block_rq_issue` tracepoint defined by the Linux kernel,
which is exposed to userland programs through the _blktrace_ API.

Unfortunately, it is not exposed to kernel modules (in particular, the
`__tracepoint_block_rq_issue` variable is not exported and therefore it cannot
be resolved by the module loader at runtime). For this reason, the Makefile
contains a `grep` command to extract its address from the running kernel's memory
map (_/proc/kallsyms_) and resolve its address at compile time.

The generated kernel module is therefore very dependent on the memory layout of
the kernel it was compiled against. If you try to load the compiled module on a
different kernel binary (even if it's the very same kernel version), the address
of that variable will likely be different and the module will refuse to load.

If the kernel was compiled with `CONFIG_RELOCATABLE`, it is even possible for the
address to change across reboots with the same kernel binary.

Please note that all addresses in _/proc/kallsyms_ might [appear as zero to
non-root users](https://stackoverflow.com/questions/55591542). Therefore, this
module must be compiled as root.

## RPM package and systemd service
The repository also contains a `make-rpm.sh` script that generates an RPM
package containing a systemd service that:
 1. compiles the module against the running kernel
 2. loads it and assigns the power LED (you may want to tweak the
    `block-led-trigger` script)

The RPM package can be installed and started as follows:

```
# ./make-rpm.sh
# dnf install block-led-trigger-0.5-1.fc36.noarch.rpm
# systemctl start block-led-trigger
```

To run the service automatically on boot:

```
# systemctl enable block-led-trigger
```

Note that the service script will rebuild the module on each boot. This
operation usually takes negligible time on a modern CPU, but it can be probably
be avoided by creating proper `akmods`/`dkms` packages, provided
`CONFIG_RELOCATABLE` does not interfere (see _Warnings_ above).
