# Jirachi: self-balancing Reuleaux triangle

Jirachi is an embedded device in the shape of a Reuleaux triangle which is able to balance itself in the positions where the system is unstable. This is done through active control in the form of a PID controller.

> [!NOTE]
> This project is still active and being worked on, the firmware for the device and the software for the control GUI is mostly done, the hardware is completely done (no plan for future revisions), and the mechanical side of the project is currently the main focus, as well as documentation.

![Jirachi Device](mechanical/assets/og-irl2.jpg)

This repository keeps track of all the work that's needed to be done in order to create a product from scratch. The solutions provided aren't necessarily the best ones out there, but the ones that made sense to me as a solo developer given the nature of the project (which is a personal project if it wasn't obvious).

## Overview

As you would be able to tell from the four directories at the root of the repository, each of them aims to solve a specific problem that would ultimately solve the "i want a balancing-robot-thingy" issue. Here's an brief explanation of each folder in the same chronology as this was being developed:

- `hardware/`: This folder holds all the files related to the electronics of the device. This is essentially the device itself and the schematic design and PCB layout was done on Altium Designer. You will be able to find each of the schematics, the PCB project, and the outputs generated by the program to manufacture and assemble the PCB. There are other miscellaneous files used by the program to correctly understand the relation between the mechanical constrains of the device and the actual electronics design needed to be done. [You can read more about this section over here.](hardware/README.md)
- `mechanical/`: This folder holds all the files and data related to the mechanical design of the device. This means any part that is not the PCB itself or soldered directly to it. Technically this aspect of the device was being developed alongside the hardware, as I was jumping back and forth between programs so I could generate mechanical data to add to the hardware program, and viceversa so I could generate a plausible assembly from the PCB data. The mechanical design was worked on CATIA V5, the most interesting thing is the Reuleaux triangle, although it is pretty trivial to implement. Other than that, here is where I defined the assembly, the shape of the motor mount, the flywheel, enclosure, etc. [You can read more about this section over here.](mechanical/README.md)
- `fimrware/`: This folder holds all the source code running on the device. The code implements the RGB LED controller, intertial measurement unit driver, motor driver, Bluetooth Low Energy driver, the PID controller and the sensor-fusion algorithm. The source code is written in C, using the ESP-IDF framework. [You can read more about this section over here.](firmware/README.md)
- `gui/`: This folder holds the source code of a GUI application used to tune the PID controller and enable the control algorithm over BLE. It's implemented in Rust just because I wanted to learn it in a more complex project hehe. It is supposed to be cross-platform but I've just tested on Windows cause my linux distro on my dual booted machine is broken at the moment so whatever (i would just use linux only but i wanted to use some specific CAD programs, so cry about it :P). [You can read more about this section over here.](gui/README.md) ALSO!!! the only reason i created a GUI app to control the device was because stupid windows couldn't connect to my device without pairing first, and to do the pairing i need to define some super specific bloated BLE characteristics so whatever i just did my own solution >:))). You can read more about that particular issue [over here.](https://github.com/espressif/esp-idf/issues/10653#issuecomment-1751914245)

## Build one yourself!!! (Bill of Materials)

If, by some weird reason, you want to build this device from scratch, then let me tell you i would love to see pictures of this lol. Send 'em my way!!! Here's a high level overview of what you'd need to build this device:

- 1 x Assembled PCB: you can just order it from JLCPCB at a pretty cheap price (~20 usd per board), everything is already done for you over at the hardware directory!!
- 1 x DC motor: I used [this one](https://www.amazon.com/dp/B07JYM8H18)
- 1 x Motor coupler: I used [this one](https://www.amazon.com/dp/B0BZS347G5)
- 1 x 3D prints of the models under the `mechanical/outputs/` folder
- 3 x M4x50mm screws
- 2 x M3x10mm screws
- 1 x Li-Ion/LiPo Battery (can be any kind as long as voltage is between 5 and 3.6V, an it can deliver ~1.5A). I used a 18650 cell with a capacity of 2600 mAh
- 2 x zip ties ~2 or 3 mm in width
- 1 x sick soldering and assembly skillz

## Repository Structure
```
├───firmware   -> actual code running on the embedded device
│   ├───docs   -> extra documentation regarding algorithms
│   ├───main   -> source code, built with the esp-idf framework
│   └───tests
├───gui        -> cross-platform gui to connect and control the device
│   ├───assets -> images used by the gui README.md
│   └───src    -> rust source code
├───hardware   -> altium designer files with the schematics and PCB
│   ├───assets -> images used by the hardware README.md
│   ├───libraries -> component libraries
│   │   ├───Phil's Lab
│   │   └───Terreneitor
│   └───Project Outputs for Lucido -> gerbers, BOM and CPL files for manufactury/assembly of the boards
└───mechanical -> 3D models/designs for everything else, done in CATIA
    └───assets -> images used by the mechanical README.md
```