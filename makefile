# FILE: makefile
# A Bautista, B Franco, E Mora
# OS, Fall 2023, Transy U
#
# makefile for Group 3 Final

babyOS: babyOS.o FCFS.o pageReplacementSimulator.o pageTable.o Priority.o RoundRobin.o SJF.o
	g++ babyOS.o FCFS.o pageReplacementSimulator.o pageTable.o Priority.o RoundRobin.o SJF.o -o babyOS

babyOS.o: FCFS.h pageReplacementSimulator.h pageTable.h Priority.h procManagement.h RoundRobin.h SJF.h symConsts.h
	g++ -c babyOS.cpp
