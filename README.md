#DNS Name Resolution Engine
##CS3753 (Operating Systems)
###Spring 2012
###University of Colorado Boulder
###Programming Assignment 2
###Public Code

####Assignment By Andy Sayler - 2012
http://www.andysayler.com
With help from:
Junho Ahn - 2012

####Adopted from previous code by
Chris Wailes <chris.wailes@gmail.com> - 2010
Wei-Te Chen <weite.chen@colorado.edu> - 2011
Blaise Barney - pthread-hello.c

###Folders
input - names*.txt input files

##Run Instructions
Build:
 `make`

Clean:
 `make clean`

Lookup DNS info for all names files in input folder:
 `./mulit-lookup input/names*.txt report.txt`

Check multi-lookup for memory leaks:
 `valgrind ./mulit-lookup input/names*.txt report.txt`

Run pthread-hello
 `./pthread-hello`

##Included Files

###My Executable
multi-lookup - A threaded DNS query-er

###Original Executables
lookup - A basic non-threaded DNS query-er
queueTest - Unit test program for queue
pthread-hello ; A simple threaded "Hello World" program
