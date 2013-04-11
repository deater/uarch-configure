#!/bin/sh

insmod ./rasp-pi-pmu.ko
./million
rmmod rasp-pi-pmu
insmod ./rasp-pi-pmu.ko
./fivemillion
rmmod rasp-pi-pmu
