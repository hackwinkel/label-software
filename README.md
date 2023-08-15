# label-software

The "label" is a cheap badge with a PADAUK PFS154-S16 microcontroller, 40 leds and IR transmitter & receiver for inter-badge communications.
The software/hardware *may* also work with a PMS150C-S16 and pin compatible controllers from other manufacturers (e.g. Nyquest tech), but this is untested

This software goes with the hardware described here: https://github.com/hackwinkel/label-hardware

## Background

This badge was originally designed for SMD hotplate soldering workshops at TBD (https://tbd.camp/) and later used for workshops at bornhack 2023 () and private workshops

According to internet rule 663829, "Every hacker conference shall have a badge". Being hackers, of course, leads to hypotheses 1-3:
"1: Any hacker conference badge will be overly complex",
"2: Any hacker conference badge will be released with unfinished software" and
"3: Any hacker conference  badge will function poorly for its namesake purpose".
This badge disproves these 3 hypotheses.

## Prerequisites & instructions

You will need a PADAUK programmer to program the device. PADAUK microcontrollers use a proprietary programming algorithm.

Hardware and software for an open source programmer can be found here: https://github.com/free-pdk/easy-pdk-programmer-hardware and https://github.com/free-pdk/easy-pdk-programmer-software.

Follow the instructions provided on these pages.


To compile the software for the label you need:

1. Small Device C Compiler (SDCC) (https://sdcc.sourceforge.net/) version >=4

2. pdk-includes (https://github.com/free-pdk/pdk-includes)

3. easy-pdk-includes (https://github.com/free-pdk/pdk-includes)

3. This repository

Install SDCC as per instructions for your system


Make a containing directory to hold both this repository and the free-pdk  pdk-includes and easy-pdk-includes repositories and:

cd <your_dir>

git clone https://github.com/free-pdk/pdk-includes

git clone https://github.com/free-pdk/easy-pdk-includes

git clone https://github.com/free-pdk/pdk-includes

git clone https://github.com/hackwinkel/label-software

cd label-software

cd cooperative

make

make burn

This last command requires easypdkprog to be found in your $PATH. You can also run this command manually

Other make targets are: sizes (displays the sizes of varios segments in the binary), clean and all (the default).


Each version of the software for the label is located in a separate sub-directory:

1. cooperative: the normal software that will sync all labels by transmitting a 25ms IR pulse every ~ minute, while continuously listening for incoming IR pulses.

2. antisocial: continuously transmitting IR while NOT listening for incoming IR signals.


Of course, installing the "antisocial" software is a trivial/stupid way to mess with all labels around you, and will probably result in you being found out quite quickly and being called a "dumb fricktard". And rightfully so. If you want to do something special with your label (a badge-battle with secret codes? A TV-B-gone clone (https://en.wikipedia.org/wiki/TV-B-Gone?), please do so in an intelligent way, in an IR transmitting envelope that will NOT annoyingly interfere with other (cooperative) badges in your neighborhood:

1. Transmit IR codes once per minute at most

2. Transmit IR codes for one second at most

3. While not transmitting, listen for incoming IR signals, and reset your timing cycle accordingly


If you want to quickly get started programming your label yourself, simply copy the complete "cooperative" folder with its contents to a new subdirectory of the label-software folder, make the necessary changes to main.c, then run "make; make burn" from this new folder.


Note: reprogramming through the 5 pin programming connection may not work because of failure to calibrate the clock speed. You CAN reprogram the badge without clock-speed calibration by using easypdkprog with the --nocalibrate option.
