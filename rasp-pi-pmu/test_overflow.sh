#!/bin/sh

insmod ./rasp-pi-pmu.ko
./tenbillion
rmmod rasp-pi-pmu
