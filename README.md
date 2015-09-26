Daphne: the First Ever Multiple Arcade Laserdisc Emulator
=========================================================

---

Daphne, the First Ever Multiple Arcade Laserdisc Emulator is a program 
that lets one play the original versions of many laserdisc arcade games 
on one's PC.

The official website for Daphne is http://www.daphne-emu.com.

As development for Daphne has stopped, this repository is intended as 
something of a continuation until the author decides to do something 
official.

**Compiling Daphne on Linux**
I'm not sure anymore how to get Daphne compiled for 32-bit Linux.  Here 
I'll assume we're working on an amd64 processor.

`cd src/vldp2`

`./configure --disable-accel-detect`

`make -f Makefile.linux_x64`

`cd ..`

`ln -s Makefile.vars.linux_x86 Makefile.vars`

`make`
