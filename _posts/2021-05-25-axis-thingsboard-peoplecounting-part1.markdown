---
layout: single
#classes: wide
toc: true
title:  "Part 1 - Introduction - People Counter Axis/ThingsBoard"
date:   2021-05-29 13:00:00 +0200
categories: blog
tags: 
- IoT
- Axis
- ThingsBoard
---

This blog series aims at showing how easy it can be to set up a people/visitor counting solution with the help of Axis cameras and the IoT-Platform *ThingsBoard*.

## Background

In retail, Visitor/People counting statistics has been an important KPI for a long time. During the Covid-19 pandemic I have - in my professional work life - noticed a growing need for solutions that tracks the current occupancy to make sure that no violations for the maximum number of simultaneously visitors are made. People counting statistics are in many cases also important to justify the need of office space or for public locations which are granted funding based on the number of visitors.

## Introduction

In this blog series I will show how one can create a solution that can receive data from multiple people counting cameras, and have a *data flow* around it to be able to aggregate data and create statistics around the data for which the camera device it self doesn't report.

Functionality and statistics that I will cover in this blog series are:

* Current occupancy on a per-floor basis with support for multiple entrances/exits per floor.
* Max Occupancy warning if the current occupancy count exceeds the configured threshold.
* Total visitors per day/hour statistics.
* Per entrance/exit statistics. Percent in/outs between the entrances/exits on each floor.

## Pre-Req

### Axis - Cameras

In this blog series we will use the "3D" people counter camera P8815-2 from [Axis Solutions](https://www.axis.com/sv-se/products/axis-p8815-2-3d-people-counter){:target="_blank"}. This camera has two camera lenses which makes it less prone to shadows and other things that otherwise can cause miscounts.

This camera is shipped with the People Counter functionality built-in to the firmware itself, but People Counter and other [ACAP:s](https://www.axis.com/products/analytics/acap) can be installed on other Axis camera models as well if needed.

The People Counting ACAP provides an API for the People Count data, and the ACAP also holds the functionality to post this data to a remote server on a set time interval. More information on the Axis People Counter solution can be found [here](https://www.axis.com/products/axis-people-counter){:target="_blank"}.

### ThingsBoard

On the receiving end, we will use [ThingsBoard](https://ThingsBoard.io){:target="_blank"} as the IoT Platform. ThingsBoard is a versatile IoT platform that thru *integrations* and *data converters* makes it possible to receive and decode data from almost *all* IoT sources. It is a modern platform which can scale extremely well when it comes to message throughput and different sizing examples can be found on the ThingsBoard website.

Information on how to deploy ThingsBoard are all described in detail on the ThingsBoard web site, so I will not cover the installation of ThingsBoard in this blog. - You can either install ThingsBoard yourself, or try the online trial of the professional edition which I will be using in this blog series.

In this setup we will configure the Axis camera to send its data to a *MQTT Broker* every time it detects a person passing in or out over its virtual "detection line". ThingsBoard will be configured to connect to the same MQTT broker and subscribe to the topics to which the camera publishes its messages.

Axis can also be configured to send the people counting data on HTTPS, but in this blog series we will use MQTT instead since MQTT has something called "Last Will Message". The *LWT/LWM* is something that the MQTT client informs the MQTT broker about when the connection is established, and if the connectivity between the client (camera) and the MQTT broker is lost, then the MQTT broker will publish this LWT/LWM message. This makes it possible for us to detect in near realtime if a device goes offline by configuring ThingsBoard to listen to this topic.

Apart from creating an *integration* in ThingsBoard, we will also create a *data converter*. A data converter is a piece of javascript code that is executed by the *integration* for each incoming package that is received by the integration. The idea with the data converter is to transform the incoming JSON message to a format which the *rule chain* in ThingsBoard can handle and later write the attributes and/or telemetry to the database.

The *rule chain* in ThingsBoard is similar to *Node-RED* and can be used to apply all kinds of logic. In this blog series we will create a custom *rule-chain* that performs aggregation of the data to make it possible to have multiple entrances/exists/floors for a building and see the aggregated results from multiple people counting cameras on one object.

## Lets get started

First, in ThingsBoard we create a new *data converter*. We give it a name, enable debugging and replace the *Decoder* code with:
{% highlight javascript %}
// Decode an uplink message from a buffer
// payload - array of bytes
// metadata - key/value object

/**Decoder**/

// decode payload to string
var payloadStr = decodeToString(payload);

// decode payload to JSON
var data = decodeToJson(payload);

var deviceName = data.deviceId;
var deviceType = 'Axis People Counter';
var groupName = deviceType;

// Result object with device/asset attributes/telemetry data
if (typeof data.online != "undefined") {
    var act = data.online
    var result = {
        // Use deviceName and deviceType or assetName and assetType, but not both.
        deviceName: deviceName,
        deviceType: deviceType,
        groupName: groupName,
        attributes: {
            integrationName: metadata['integrationName'],
            active: act
        },
        telemetry: {
            online: data.online
        }
    };
} else {
    var result = {
        // Use deviceName and deviceType or assetName and assetType, but not both.
        deviceName: deviceName,
        deviceType: deviceType,
        groupName: groupName,
        attributes: {
            integrationName: metadata['integrationName']
        },
        telemetry: {
            count: data.count
        }
    };
}

/**Helper functions**/

function decodeToString(payload) {
    return String.fromCharCode.apply(String, payload);
}

function decodeToJson(payload) {
    // covert payload to string.
    var str = decodeToString(payload);

    // parse string to JSON
    var data = JSON.parse(str);
    return data;
}

return result;
{% endhighlight %}

Then save it:

![DataConverter]({{ BASE_PATH }}/assets/images/axis-thingsboard-peoplecounting-part1/dataconverter.png)

Next we crate a new *integration*. Give it a good name, select MQTT, select the *data converter* that we created earlier, enter the MQTT broker HOST, port, set a random *Client ID*, provide broker username/password and then under *Topic Filters* add *axis/+/+*. Enable debug and then save the integration:

![Mqtt_Integration]({{ BASE_PATH }}/assets/images/axis-thingsboard-peoplecounting-part1/mqtt_integration.png)

Now we have made the initial configuration in ThingsBoard and we are now ready to receive data from the Axis device. Now go to the Axis device and configure the MQTT settings on the camera. There are two parts that needs to be configured. One is the device level MQTT settings, and then *Events* need to be configured to trigger a MQTT message for each in/out count.

In the Axis web GUI under *Device View* > *MQTT* configure the host, username/password as well as the *Last will testament* and *connect message*:

![Device Mqtt Settings]({{ BASE_PATH }}/assets/images/axis-thingsboard-peoplecounting-part1/deviceMqttSettings.png)

Under *Events* configure two events. One for *In* and one for *out*:

![Device Mqtt Event]({{ BASE_PATH }}/assets/images/axis-thingsboard-peoplecounting-part1/deviceMqttEvent.png)

Once the camera is configured. Do some motion in front of it to make it perform an *in* or *out* count. After that, go to ThingsBoard and open the data converter and go to the Events tab. Since we enabled debugging earlier, we should now see that we have received a package and that the *data converter* transformed it as an *output*:

![DC In Event]({{ BASE_PATH }}/assets/images/axis-thingsboard-peoplecounting-part1/dcInEvent.png) | ![DC Out Event]({{ BASE_PATH }}/assets/images/axis-thingsboard-peoplecounting-part1/dcOutEvent.png)

ThingsBoard has now automatically provisioned a *device* for us since the *data converter* was able to execute successfully and since no device with this *deviceName* existed earlier.

In [part 2]({% post_url 2021-05-25-axis-thingsboard-peoplecounting-part2 %}) of this blog, we will focus on using *assets* to be able to support statistics and data aggregation when we have multiple entrances/exists/floors.
