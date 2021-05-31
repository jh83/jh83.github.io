---
layout: single
#classes: wide
toc: true
title:  "Axis - ThingsBoard People Counter - Rule Chains"
date:   2021-05-29 16:00:00 +0200
categories: blog
tags: 
- IoT
- Axis
- ThingsBoard
---

In [part 2]({% post_url 2021-05-25-axis-thingsboard-peoplecounting-part2 %}) of this blog series, we configured the assets and relation between those which we in this part 3 will work with *rule chains* to create the data flow and data aggregation needed.

With the help of rule chains, we will create the functionality needed to create the floor/building data aggregation as well as calculate other statistics that we want the system to provide us with.

The rule chain will also calculate realtime per-floor and building occupancy numbers for us, which makes it possible to display it in different colors on a dashboard depending on the configured warning/maximum occupancy thresholds.

The rule chain which I have created for this consists of multiple parts. The main part is *event based*, and this part is executed each time a device sends a data package to ThingsBoard. Then there also are some parts in the rule chain which is based on time intervals that execute data aggregation, resets count at 00:00 local time etc which are needed to maintain the correct counter data.

## Event based flow

Let's start with the rule chain which is event based.

The first part of this event based rule chain is a filter that validates that the data that triggers this rule cain is of type *telemetry*. The second node is to validate that the device type is what we expect, so that we don't try to run the next steps on other devices:
[![Event part1]({{ BASE_PATH }}/assets/images/axis-thingsboard-peoplecounting-part3/EventPart1.png)]({{ BASE_PATH }}/assets/images/axis-thingsboard-peoplecounting-part3/EventPart1.png)

In the second part of this event based rule chain we start to add some data. What we do here is to create the data needed to later be able to calculate how many percent of total floor In/Outs that uses this particular Entrance:

* Change the *originator* from the device to the parent entrance asset.
* From the entrance asset, get the previous *totalInOutCount* time series data.
* Run a script that adds the current *totalInOutCount* to the previous *totalInOutCount*
* Save this to the database.

[![Event part2]({{ BASE_PATH }}/assets/images/axis-thingsboard-peoplecounting-part3/EventPart2.png)]({{ BASE_PATH }}/assets/images/axis-thingsboard-peoplecounting-part3/EventPart2.png)

In the third part we do:

* we start by changing the originator from the entrance we changed to in step 2, to the floor based on the asset relations we have created earlier.
* From the floor, we gather both attributes and previous telemetry. We collect *maxOccupancy* which is configured on the floor level that tells the maximum number of allowed visitors for each floor. We also collect *currentOccupancy*, *totalVisitors* and *totalInOutCount* from the floor asset.
* In the script node, we adjust the output by taking the previous telemetry and incrementing/decrementing those depending on if the camera counted In (1) or Out (-1). We also make sure that if the *currentOccupancy* is 0 and the camera sends "-1", the currentOccupancy should remain 0, and not be -1 on the floor level.
* Next we save this to the database

[![Event part3]({{ BASE_PATH }}/assets/images/axis-thingsboard-peoplecounting-part3/EventPart3.png)]({{ BASE_PATH }}/assets/images/axis-thingsboard-peoplecounting-part3/EventPart3.png)

The last step in this rule chain is used to perform an action of the *currentOccupancy* exceeds the *maxOccupancy* which is configured on each floor asset. If the occupancy exceeds the maximum allowed number then we can trigger an event like sending an email or REST API call to an external system to create a incident or other things.

* In the first node, we check if the *currentOccupancy* is below or above *maxOccupancy*
* If the *currentOccupancy* is above the configured threshold then we create a alarm within ThingsBoard, we then have a script node which formats the body to be used in the REST API Call against the external service, and then perform the actual REST API call.
* If the *currentOccupancy* is below the configured threshold, then we will clear the existing alarm in ThingsBoard. We also format the REST body in the script node so that it thru the REST API call can notify the external system that the *currentOccupancy* now is below *maxOccupancy*.

[![Event part4]({{ BASE_PATH }}/assets/images/axis-thingsboard-peoplecounting-part3/EventPart4.png)]({{ BASE_PATH }}/assets/images/axis-thingsboard-peoplecounting-part3/EventPart4.png)

## Interval based flows

Apart from the event based activities above, we also have some things that we need to trigger to run on a interval to make the solution work.

### Floor > Building aggregation

The event based rule chain describes above updates the data on the entrance level and on the floor level in realtime. However, we also want the floor occupancy and totalVisitors to be aggregated to the building level, so that we can see on the building level what the total occupancy and visitor numbers are when combing the data from all floors.

To do this, I have created a *aggregate latest* which runs every 60 seconds that gets all floors related to a building and then sums the *currentOccupancy* and *totalVisitors* for all floors and then saves it to the building asset:

[![Building aggregation]({{ BASE_PATH }}/assets/images/axis-thingsboard-peoplecounting-part3/floorBuildingAggregation.png)]({{ BASE_PATH }}/assets/images/axis-thingsboard-peoplecounting-part3/floorBuildingAggregation.png)

### Calculate entrance utilization in percent

In this blog post we have one main and one secondary entrance for each floor. We know that people are entering/leaving the floor using any of those two. But how many percent in/outs do we have on the main entrance VS the secondary entrance? - These nodes are executed every 60 seconds to calculate the *entranceUtilization* percent for us.

* First we split this job to run on all floor assets in the building.
* Then we split this job to run on all entrance assets in each floor.
* For each entrance asset, we then get the corresponding *totalInOutCount* from the floor.
* In the "originator attributes" we get the *totalInOutCount* for this particular entrance asset.
* And in the script node, we now can calculate the each entrance *entranceUtilizationPercent* by taking the entrance *totalInOutCount* and device it with the floor *totalInOutCount*.

[![Entrance Utilization]({{ BASE_PATH }}/assets/images/axis-thingsboard-peoplecounting-part3/entranceUtilization.png)]({{ BASE_PATH }}/assets/images/axis-thingsboard-peoplecounting-part3/entranceUtilization.png)

### Reset all counters to zero

If you remember from part one of this series, the cameras in the MQTT configuration that we are using sends either *count: 1* or *count: -1*. This means that we now want ThingsBoard to set all counters that we have incremented back to 0 on a specific time, which in this case is configured on the building asset to be 00:00.

The nodes from left to right:

* Every 300 seconds the generator node is executed.
* *Originator attributes* is read from the building asset to get the *clearPeopleCountTimeUTC* attribute, which in our case is set to "2200" which translates to 00:00 in my local time.
* The script node then checks if the current time is within 10 minutes from what was specified in the *clearPeopleCountTimeUTC* and if not, then it stops here.
* We then duplicates this message from the building to the floors and all entrances related to this building.
* Next we verify if *totalVisitors* exists.
* And lastly we write *0* for *currentOccupancy, totalVisitors* and *totalInOutCount* as time series data into the database.

[![Reset counters]({{ BASE_PATH }}/assets/images/axis-thingsboard-peoplecounting-part3/resetBuilding.png)]({{ BASE_PATH }}/assets/images/axis-thingsboard-peoplecounting-part3/resetBuilding.png)

Note: If we were using the builtin functionality in the camera to send the data on a 15 minutes interval, then the camera would reset it's counters at 00:00 automatically, but in this blog post we are using *Events* instead since we wanted a *real time* solution.

### Conclusion

In this part we added all the *intelligence* to the solution to add the functionality and calculate the data which we need in [part 4]({% post_url 2021-05-25-axis-thingsboard-peoplecounting-part4 %}) where we will build a dashboard for monitoring.
