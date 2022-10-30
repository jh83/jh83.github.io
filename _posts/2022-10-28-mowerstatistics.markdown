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

How *inefficient* is a lawn mower that cuts on a randomized pattern? How many square meters does it actually mow before it successfully mowed the whole lawn? - Let's find out!

## Introduction

In this solution, I will use Real-Time Kinematic (RKT) GPS with centimeter level accuracy to track the movements our Husqvarna AutoMower 315 Mark II robotic mower and upload the data into a MongoDB Atlas database which in the end will provide me with the possibilities to visualize and graph how much time and area the robotic lawn mower need to cut before the whole lawn is cut at least once.

And the results? Look here:
[![Animation]({{ BASE_PATH }}/assets/images/mower-statistics/animation.gif)]({{ BASE_PATH }}/assets/images/mower-statistics/animation.gif)

## Solution overview

To solve this task, multiple components and services has been used.

First of all, RTK-GPS was a requirement from my side to provide the GPS/GNSS accuracy required for location data to be useful in this analysis. Standard GPS where meter-accuracy is achieved is not sufficient. RTK-GPS provides centimeter accuracy which is what I need in this case.

I also needed a way to transport the RTK reference signal from my base station to the Mower which is considered as a *rover*. It turned out that my WiFi covered almost all of my lawn so I decided to go for WiFi for simplicity.

My RTK-GPS base station was already in place and running from previous projects and the reference signal has been sent to RTK2GO for a long time. More information about the RTK-Base station I built earlier which runs on a ESP32 microcontroller powered thru PoE can be found here [NTRIP-Server_ESP32-PoE-BaseStation](https://github.com/jh83/NTRIP-Server_ESP32-BaseStation)

The GPS/GNSS location data which is produced once per second from on Ublox ZED-F9P GNSS module is added to a *buffer* in the ESP32 microcontroller running on the mower and then on every 10 seconds it send it's buffer to a HTTPS endpoint and function living in the MongoDB Atlas Cloud service.

If WiFi is lost, then GPS positions are buffered and sent once the WiFi connectivity is reestablished. RTK-GPS seem to maintain its "FIX" status for about 30-60 seconds without a reference signal.

The receiving *MongoDB Atlas Function* is responsible for parsing the raw NMEA string which was collected and sent by the ESP32 microcontroller to MongoDB. After parsing the NMEA String, the MongoDB function creates a geoJSON *point* feature of each measurement and inserts it into a time series collection in the database.

[![Architecture]({{ BASE_PATH }}/assets/images/mower-statistics/components.png)]({{ BASE_PATH }}/assets/images/mower-statistics/components.png)

The Lawn perimeter and the exclusions (staircase and trees) in it has all been mapped with the help of the same Ublox ZED-F9P, but connected to an Android phone running *SW MAPS*.

### What is RTK-GPS?

This project relies heavily on the use of RTK-GPS/GNSS to get the accuracy needed in this situation where want to know if the mower cutting disc has passed a specific location or not. - So what is RTK-GPS and how does it compare with normal GPS?

RTK-GPS uses a reference signal which is transmitted from a fixed base-station to a mowing rover. In this setup the RTK Base station is mounted on our house and the rover is the hardware which is mounted on the AutoMower itself.

RTK-GPS explanation from Wikipedia:
*In practice, RTK systems use a single base-station receiver and a number of mobile units. The base station re-broadcasts the phase of the carrier that it observes, and the mobile units compare their own phase measurements with the one received from the base station. ... RTK provides accuracy enhancements up to about 20 km from the base station.*

*This allows the units to calculate their relative position to within millimeters, although their absolute position is accurate only to the same accuracy as the computed position of the base station. The typical nominal accuracy for these systems is 1 centimeter ± 2 parts-per-million (ppm) horizontally and 2 centimeters ± 2 ppm vertically.*

*Although these parameters limit the usefulness of the RTK technique for general navigation, the technique is perfectly suited to roles like surveying. In this case, the base station is located at a known surveyed location, often a benchmark, and the mobile units can then produce a highly accurate map by taking fixes relative to that point. RTK has also found uses in autodrive/autopilot systems, precision farming, machine control systems and similar roles.*

### Mower

The hardware was placed in a waterproof food container. Food containers are cheap, watertight and available in different sizes which makes them as a good option for DIY projects.s

[![Mower Hardware Installation]({{ BASE_PATH }}/assets/images/mower-statistics/mower.png)]({{ BASE_PATH }}/assets/images/mower-statistics/mower.png)

#### Hardware

[![Mower Hardware]({{ BASE_PATH }}/assets/images/mower-statistics/mower_hardware.png)]({{ BASE_PATH }}/assets/images/mower-statistics/mower_hardware.png)

The hardware placed on the mower consists of:

* 10000 mAh powerbank. Is capable of powering the hardware for about 14-16 hours.
* A drotek Ublox ZED-F9P GNSS module.
* DA910 Multi-band GNSS Antenna
* Espressif ESP32 as the "brain". It is responsible for:
  * Getting the RTK reference signal from RTK2GO cloud service and sending it thru I2C to the Ublox ZED-F9P GNSS module.
  * Receive the NMEA *GNGGA* message from the ZED-F9P at 1 HZ and buffer it until next *uplink* to MongoDB.
  * Send the NMEA *buffer* to the HTTPS endpoint/Function in MongoDB Atlas.

#### Software

In this project, both CPU cores of the ESP32 are used to be able to execute the tasks in parallel.

* First core is responsible for:
  * Connect and subscribe to the RTK-GNSS reference signal from the RTK2GO *NTRIP Caster* cloud service and push it to the ZED-F9P module thru I2C.
  * Listen to the NMEA string output from the ZED-F9P which occurs at 1HZ and push it to the *NMEA Buffer*.
* Second core is responsible for:
  * On a 10 second interval, get the *NMEA Buffer* and push it to the MongoDB HTTPS endpoint.
  * If WiFi or MongoDB is unreachable for whatever reason (poor WiFi coverage), then retry the *NMEA Buffer* at an interval and then bulk upload it once WiFi/MongoDB becomes available.

More details on the software can be found in the github repo <LÄNK>.

### MognoDB Atlas Cloud Services

MongoDB Atlas cloud services has been used heavily in this project. All that has been used here is provides for free in the cloud service. I have not used any paid MongoDB Services.

The Web frontend - described later - interacts with the MongoDB thru their [*Web SDK*](https://www.mongodb.com/docs/realm/web/)

#### HTTPS Endpoint

In MongoDB Atlas, an HTTPS Endpoint has been created. This HTTPS endpoint initiates a *Function* when something is sent to its URL.

#### Function

The MongoDB Atlas function that has been coded (and triggered by the HTTPS Endpoint) is responsible for:

* Loop thru the incoming array of NMEA Strings and:
  * Parse the NMEA string to:
    * Timestamp that can be handled by MongoDB.
    * Longitude/Latitude as a *geoJSON Point* format.
  * Write to database with *insertMany()*

The *Function* code:

```js
// This function is the endpoint's request handler.
exports = function ({ query, headers, body }, response) {

    // This is a binary object that can be accessed as a string using .text()
    const jsonBody = JSON.parse(body.text());
    console.log("Data Array Length: ", jsonBody.length);

    // Array containing all documents that should be written to DB later
    let docs = [];

    // Verify that data exists
    if (jsonBody.length > 1) {

        // Loop thru all items in jsonBody array
        for (var row in jsonBody) {

            // Parse NMEA string
            const nmeaParsedBody = parseNMEA(jsonBody[row]);

            // Build obj
            const obj = {
                "device_id": "esp32RtkGPS",
                "gps_ts": nmeaParsedBody.gps_ts,
                "fix_type": parseInt(nmeaParsedBody.fix_type),
                "num_satellites": parseInt(nmeaParsedBody.num_satellites),
                "deviation_lat": parseFloat(nmeaParsedBody.deviation_lat),
                "deviation_lon": parseFloat(nmeaParsedBody.deviation_lon),
                "deviation_alt": parseFloat(nmeaParsedBody.deviation_alt),
                "location": {
                    "coordinates": [nmeaParsedBody.longitude, nmeaParsedBody.latitude],
                    "type": "Point"
                }
            }

            // Push object to docs array
            docs.push(obj)
        }

        // Write docs to database collection
        context.services.get("mongodb-atlas").db("geospatial").collection("telemetry").insertMany(docs)
            .then(result => {
                response.setStatusCode(200);
            })
            .catch(err => {
                console.error(`Failed to insert telemetry item: ${err}`)
                response.setStatusCode(400);
            });
    }
    return;
};


// Function that parses NMEA GxGGA string
function parseNMEA(nmeaString) {

    // Transform NMEA string to Array
    const nmeaArray = nmeaString.split(",");

    const local_ts = new Date();
    // NMEA string only contains HH:MM:SS so we need to add YYYY:mm:DD it
    const gps_ts = new Date(local_ts.getFullYear(), local_ts.getMonth(), local_ts.getDate(), nmeaArray[1].substr(0, 2), nmeaArray[1].substr(2, 2), nmeaArray[1].substr(4, 2));

    const lat = nmeaToWGS84(nmeaArray[2], nmeaArray[3]);
    const long = nmeaToWGS84(nmeaArray[4], nmeaArray[5]);

    const obj = {
        gps_ts: gps_ts,
        latitude: lat,
        longitude: long,
        fix_type: nmeaArray[6],
        num_satellites: nmeaArray[7],
        deviation_lat: nmeaArray[21],
        deviation_lon: nmeaArray[22],
        deviation_alt: nmeaArray[23]
    };

    return obj;
}

// Convert NMEA (d)ddmm.mmmm format to WGS84 
function nmeaToWGS84(deg, latorlon) {

    // Get the whole degree part
    const degWhole = parseFloat(parseInt((deg) / 100));

    // Get the fractional part
    const degDec = (deg - degWhole * 100) / 60;

    // Complete
    let degr = degWhole + degDec; //Gives complete correct decimal form of Longitude degrees

    // Check if value should be negative
    if (latorlon == 'W' || latorlon == 'S') {  //If you are in Western Hemisphere, longitude degrees should be negative
        degr = (-1) * degr;
    }

    return degr;
}
```

#### Database

The database consists of two collections and one view:

* A time-series collection which stores all the GPS/GNSS data points sent by the mower. All *documents* stored in this collection has a unique timestamp and all contains the long/lat in a *geoJSON Point* format. - The reason why I use *geoJSON* format is to utilize the built-in capabilities in MongoDB to perform geospatial queries.
* A collection that holds all *assets*, such as the *mower* as well as the *lawn* geoJSON features.
* A view that groups the time-series collection data on a per-day view.

### Web Frontend

As stated earlier, the MongoDB *Web SDK* is used to handle the authentication to the MongoDB database and for all the reads/writes from/to the database.

MongoDB has support for *geospatial query's*, meaning that we can perform queries based on Latitude/Longitude information. This functionality is used in a *geoWithin* combined with a *not* to exclude all data points for when the mower is parked in the charger.

At first, if the user isn't logged in then a login page is shown:

[![Login Page]({{ BASE_PATH }}/assets/images/mower-statistics/loginpage.png)]({{ BASE_PATH }}/assets/images/mower-statistics/loginpage.png)

After login, the user is prompted with the default page:

[![Default Page]({{ BASE_PATH }}/assets/images/mower-statistics/defaultpage.jpeg)]({{ BASE_PATH }}/assets/images/mower-statistics/defaultpage.jpeg)

#### Settings

Mower settings can be managed:

* *Fabricator* and *Model Name* are both descriptor fields.
* *Cutting Width* is in centimeters and controls how wide the *path* will be drawn later. In this solution each pixel on the *canvas* is a 1x1 square centimeter.
* *Start time* and *End time* is used as from/to time when querying the database.

[![Mower Settings Page]({{ BASE_PATH }}/assets/images/mower-statistics/mowerpage.png)]({{ BASE_PATH }}/assets/images/mower-statistics/mowerpage.png)

Lawn settings can be managed:

* Takes an *geoJSON features* object as input. A geoJSON features object can contain multiple *features* and the perimeter of the lawn must be the first object in the array. Additional *features* can be added to exclude areas for trees and stairs etc from the Lawn area. If "name: charging_station" is set on a geoJSON feature then this "point" will also be used as an *not geoWithin* input in the DB Query.

[![Lawn Settings Page]({{ BASE_PATH }}/assets/images/mower-statistics/lawnpage.png)]({{ BASE_PATH }}/assets/images/mower-statistics/lawnpage.png)

#### Per day Statistics

Once the desired days has been selected in the table, then it will loop thru each selected day and render it on a canvas (which by default is hidden) where the resolution is set so that each pixel represents 1x1 square centimeter.

After the "path/track" has been drawn (red pixels) based on the Latitude/Longitude data points, then the number of *green pixels* on the canvas are counted and compared to the number of green pixels that existed before the path/track was drawn. - In this way we can determine how many square centimeters that are mowed (the pixels that now are red, and no longer green). The square cm represented by each pixel are then translated to square meters.

When the hidden canvas has been fully drawn for one date, then a snapshot is taken from it and displayed in the *Per Day Statistics* section, and then it clears the canvas and continues with the next date:

[![Per day Statistics]({{ BASE_PATH }}/assets/images/mower-statistics/perdaypage.png)]({{ BASE_PATH }}/assets/images/mower-statistics/perdaypage.png)

#### Cut Percentage Chart

This line chart displays how many square meters that has been cut in total, and how many "unique" square meters that has been cut. - As expected, this graph rises quickly in the beginning and then it flattens more and more as it gets closer to 100%:

[![Cut Percentage Chart]({{ BASE_PATH }}/assets/images/mower-statistics/linegraphpage.png)]({{ BASE_PATH }}/assets/images/mower-statistics/linegraphpage.png)
