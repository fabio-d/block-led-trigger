#!/bin/sh 
TMP="$PWD/$(mktemp -d rpmbuild.XXXXXXXXX)"
mkdir -p "$TMP/SOURCES"

tar --transform 's|^|block-led-trigger/|' -cvf "$TMP/SOURCES/block-led-trigger.tar" \
	Makefile \
	block_led_trigger.c \
	block-led-trigger \
	block-led-trigger.service

rpmbuild --define "_topdir $TMP" -bb block-led-trigger.spec
cp "$TMP/RPMS"/*/* .
rm -rf "$TMP"
