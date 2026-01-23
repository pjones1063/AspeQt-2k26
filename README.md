#  Atari AspeQt-2k26

###  Atari 8-bit in 2026 - 45+ years after the 400/800 first release!



### Atari Serial Peripheral Emulator for Qt With 8bit Cartridge

### Summary

AspeQt emulates Atari SIO peripherals when connected to an Atari 8-bit computer with an SIO2PC cable.
In that respect it's similar to programs like APE and Atari810. The main difference is that it's free
(unlike APE) and it's cross-platform (unlike Atari810 and APE).

See readme.txt for more info


#### * AspeQt Client module MENU.COM. Runs on the Atari and is used to get/set Date/Time (SpartaDos) on the Atari plus a variety of other remote tasks (on any DOS). 

#### * Package builds for Windows, Linux, MacOS, and RasPi at:  https://sourceforge.net/projects/respeqt/files/

#### * Do it yourself Atari SIO2PC cable - see: SIO2PC__Build_Instructions.pdf (for under $20.00!)


#### Support and other inquiries can be made on our BBS at:   $ telnet 13leader.net 8023 or http://13leader.net

 
* NOTE: If you installed Raspbian Lite onto your Micro-SD card you will probably get an error saying
"unable to open x display", you will likely need to run the following commands to load ldxe, x11 & lightdm:


  $ sudo apt-get install lxde lxde-core lxterminal lxappearance
	(answer Y for y/n question... this takes a loooong time to install)
     
  $ sudo apt-get install lightdm
 	(this also takes a long time to load - be patient)
 
  $ sudo apt-get install xserver-xorg 
  
  $ sudo apt-get install xinit
  
  $ sudo apt-get install x11-xserver-utils
 	(answer Y for y/n question)
 
  $ sudo apt-get install xterm
 
  $ startx
 	(Opens X window) 
 
   
