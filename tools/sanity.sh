#!/bin/bash

RZCMD=./sd_report_zones/sd_report_zones
DEVICE=/dev/sdm
if [[ $# -eq 1 ]] ; then
	DEVICE=${1}
fi

sudo ./sd_identify/sd_identify ${DEVICE}  > /dev/null 2>&1
if [[ $? -eq 0 ]] ; then
	sudo ./sd_reset_wp/sd_reset_wp 0xFFFFFFFFFFFFFFFF ${DEVICE} > /dev/null 2>&1 
	sudo ${RZCMD} ${DEVICE} 2>/dev/null | head -40 | diff - ./gold/empty.txt
	if [[ $? -eq 0 ]] ; then
		echo "Reset WP success."
	fi
	echo "... filling zones .. this may take a minute."
	sudo dd if=/dev/zero of=${DEVICE} bs=1M count=2560
	sudo ${RZCMD} ${DEVICE} 2>/dev/null | head -40 | diff - ./gold/ten.txt
	if [[ $? -eq 0 ]] ; then
		echo "Fill 10 zones success."
	fi
	sudo ./sd_reset_wp/sd_reset_wp $((0x80000 * 3)) ${DEVICE}
	sudo ${RZCMD} ${DEVICE} 2>/dev/null | head -40 | diff - ./gold/clear-z3.txt
	if [[ $? -eq 0 ]] ; then
		echo "Reset Zone 3 success."
	fi
	sudo ./sd_reset_wp/sd_reset_wp $((0x80000 * 5)) ${DEVICE}
	sudo ${RZCMD} ${DEVICE} 2>/dev/null | head -40 | diff - ./gold/clear-z5.txt
	if [[ $? -eq 0 ]] ; then
		echo "Reset Zone 5 success."
	fi
else 
	echo "Not an SMR Device."
	sudo ${RZCMD} ${DEVICE} > /dev/null 2>&1 
	if [[ $? -eq 0 ]] ; then
		echo "Broken command."
	else
		echo "Kernel is good."
	fi
fi
