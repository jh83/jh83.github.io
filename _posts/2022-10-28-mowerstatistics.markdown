---
layout: single
#classes: wide
toc: true
title:  "Robotic Mower Statistics"
date:   2022-10-28 17:00:00 +0200
categories: blog
tags: 
- IoT
- MongoDB
- Atlas
- ESP32
- Husqvarna
- AutoMower
- RTK-GPS
---

## Introduction

How *inefficient* is a lawn mower that cuts on a randomized pattern? - Let's find out!

In this article I will use Real-Time Kinematic (RKT) GPS with centimeter level accuracy to track the movements our Husqvarna AutoMower 315 Mark II robotic mower and upload the data into a MongoDB Atlas database for further analysis and visualization.

And the results? Look here:
[![Animation]({{ BASE_PATH }}/assets/images/mower-statistics/animation.gif)]({{ BASE_PATH }}/assets/images/mower-statistics/animation.gif)

## Solution overview

To solve this task, multiple components and services has been used.

First of all, RTK-GPS was a requirement from my side to provide the GPS/GNSS accuracy required for location data to be useful in this analysis. Standard GPS where meter-accuracy is achieved is not sufficient. RTK-GPS provides centimeter accuracy which is what I need in this case.

I also needed a way to transport the RTK reference signal from my base station to the Mower which is considered as a *rover*. It turned out that my WiFi covered almost all of my lawn so I decided to go for WiFi for simplicity.

My RTK-GPS base station was already running from previous projects and the reference signal has been sent to RTK2GO for a long time now. More information about the RTK-Base station I built earlier which runs on a ESP32 board powered thru PoE can be found here https://github.com/jh83/NTRIP-Server_ESP32-BaseStation

The data which is outputed once per second from the Ublox ZED-F9P GPS module is added to a *buffer* in the ESP32 chip running on the mower and then on every 10 seconds it send it's buffer to a HTTPS endpoint and function living in the MongoDB Atlas Cloud service.

If WiFi is lost, then GPS positions are buffered and sent once the WiFi connectivity is reestablished. RTK-GPS seem to maintain its "FIX" status for about 30-60 seconds without a reference signal.

The receiving *MongoDB Atlas Function* is responsible for parsing the raw NMEA string which was collected and sent by the ESP32 board to MongoDB. After parsing the NMEA String, the MongoDB function creates a geoJSON *point* feature of each measurement and inserts it into a time series collection in the database.

[![Architecture]({{ BASE_PATH }}/assets/images/mower-statistics/components.png)]({{ BASE_PATH }}/assets/images/mower-statistics/components.png)

### What is RTK-GPS?

This project relies heavily on the use of RTK-GPS to get the accuracy needed provide accurate location data. - So what is RTK-GPS and how does it compare with normal GPS?

RTK-GPS uses a reference signal which is transmitted from a fixed base-station to a mowing rover. In this setup the RTK Base station is mounted on our house and the rover is the hardware which is mounted on the AutoMower itself.

RTK-GPS explanation from Wikipedia:
*In practice, RTK systems use a single base-station receiver and a number of mobile units. The base station re-broadcasts the phase of the carrier that it observes, and the mobile units compare their own phase measurements with the one received from the base station. ... RTK provides accuracy enhancements up to about 20 km from the base station.*

*This allows the units to calculate their relative position to within millimeters, although their absolute position is accurate only to the same accuracy as the computed position of the base station. The typical nominal accuracy for these systems is 1 centimetre ± 2 parts-per-million (ppm) horizontally and 2 centimetres ± 2 ppm vertically.*

*Although these parameters limit the usefulness of the RTK technique for general navigation, the technique is perfectly suited to roles like surveying. In this case, the base station is located at a known surveyed location, often a benchmark, and the mobile units can then produce a highly accurate map by taking fixes relative to that point. RTK has also found uses in autodrive/autopilot systems, precision farming, machine control systems and similar roles.*

### Mower

[![Mower Hardware]({{ BASE_PATH }}/assets/images/mower-statistics/mower.png)]({{ BASE_PATH }}/assets/images/mower-statistics/mower.png)

#### Hardware

The hardware placed on the mower consists of:

* Ublox ZED-F9P GNSS module.
* DA910 Multi-band GNSS Antenna
* ESP32 as the "brain". It is responsible for:
  * Getting the RTK reference signal from RTK2GO cloud service and sending it thru I2C to the Ublox ZED-F9P GNSS module.
  * Receive the NMEA *GNGGA* message from the ZED-F9P at 1 HZ and buffer it until next *uplink* to MongoDB.
  * Send the NMEA *buffer* to the HTTPS endpoint/Function in MongoDB Atlas.

#### Software

### MognoDB Atlas Cloud Services

### Frontend
