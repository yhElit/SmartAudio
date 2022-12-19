#!/bin/bash
rm -f ./*.o
gcc -c ini.c -o ini.o
gcc -c communication.c -o comm.o
gcc -D_GNU_SOURCE -c DetectionOfHumanFlowDirection.c -o dohfd.o
gcc -o smartaudioclient ini.o dohfd.o comm.o -lwiringPi -lmosquitto -lpthread
