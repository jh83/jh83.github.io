---
layout: single
#classes: wide
toc: true
title:  "Sending camera "
date:   2021-10-10 17:00:00 +0200
categories: blog
tags: 
- IoT
- Axis
---

In a previous [previous]({% post_url 2021-05-25-axis-thingsboard-peoplecounting-part1 %}) blog series I showed how people count information could be sent from Axis camera devices to the Thingsboard IoT platform. - Now its time to utilize LPWAN as the communication layer between the camera device and the IoT Platform.

In this blog, I will show how to build a LoRaWAN bridge with the help of an ESP32 which will be configured to poll the Axis camera API for the people counting statistics and send the data over LoRaWAN over to an IoT Platform.

However, why would anyone want to utilize LoRaWan for sending this type of information when the actual camera is connected to a PoE network?

* A camera needs to be installed in a location in which *we* don't own the wired network.
* There is no existing wired network with internet connectivity.
* The amount of data the device produces is very small, so LoRaWan is fully capable of sending the data needed for the analytics purposes.
* Using LoRaWan combined with a micro controller such as the ESP32 is a very power-efficient solution which can be configured to *sleep* to reduce the power needed. As an addition, the ESP32 could also be extended so that it could power off the camera with a help of a MOSFET during nighttime, which would make it easier to run the whole solution on solar/battery power

### Requirements

We need some hardware to make this working. In this blog I have used the following:

* A Axis P8815-2 3D People Counter device, or any other device which has a HTTP API from which we want to collect data.
* An ESP32 with a ethernet port. I will be using the [Olimex ESP32-POE](https://www.olimex.com/Products/IoT/ESP32/ESP32-POE-ISO/open-source-hardware) which will be powered over ethernet.
* A GPS module for receiving accurate time. I will be using the [Olimex MOD-GPS](https://www.olimex.com/Products/Modules/GPS/MOD-GPS/) module in this blog. - The GPS module can be removed once LoRaWan modules running version 1.3 where the LoRaWAN *DeviceTimeReq* mac-command is available for time sync.
* A RTC module for keeping track of time. I will be using the [Olimex MOD-RTC2](https://www.olimex.com/Products/Modules/Time/MOD-RTC2/open-source-hardware) board.
* A LoRaWAN module. I will be using the [RAK811](https://store.rakwireless.com/products/rak811-lpwan-module?variant=39942880952518) unit in this blog.
* A PoE switch or PoE-injectors to power the Axis camera and the Olimex hardware. The Olimex hardware *can* be powered with 5v thru the USB connector or directly on the pins on the PCB.

### Configuration

The devices needs configuring and assembly before using.

* The Axis device needs to be configured with a static IP address as well as with a username and password. The username/password will make sure that only authenticated sessions can connect to its API and video stream to avoid inappropriate usage.
* The RAK811, MOD-RTC2 as well as the MOD-GPS all needs to be connected to the appropriate pins on the Olimex ESP32 board.
* The ESP32 and the Axis camera will be connected to each other on the Ethernet port and powered by the PoE switch used.
  
#### Axis device

In the Axis device, a username and password needs to be configured. All anonymous permissions will disabled. This will require all sessions to be authenticated with [Digest access authentication](https://en.wikipedia.org/wiki/Digest_access_authentication)

A static IP needs to be set. I will be using 10.42.0.10/24 on the camera.

#### ESP32

ESP32 will run a piece of code which will:

* Get the time from the GPS module and verify if the GPS time matches the RTC time. - If not, then it will write the GPS time to the RTC module.
* Poll the PeopleCounting API on the axis camera to get the current in/out count. - If the ESP32 notices that there is a time difference between the RTC time and the datetime which Axis returns in its API response, then the ESP32 will push the RTC time to the Axis device thru the Axis *time API*.
* Extract the In/Out count as well as the timestamp from the API response and reformat it to a LoRaWAN payload.
* Send the formatted payload over LoRaWAN to [The Things Stack](https://www.thethingsindustries.com/docs/).

---------
