---
layout: single
#classes: wide
toc: true
title:  "Axis - ThingsBoard People Counter - Visualization"
date:   2021-05-29 17:00:00 +0200
categories: blog
tags: 
- IoT
- Axis
- ThingsBoard
---

In [part 3]({% post_url 2021-05-25-axis-thingsboard-peoplecounting-part3 %}) we configured the rule chain which creates the data we need in this part (4). Now we will focus on the visualization part and build two *dashboards* to visualize the data that we now have access to.

The dashboards will be as:

* One which is a simple real time STOP/GO dashboard which can be shown on a display in the office/floor/store it self.
* One which will provide more information and statistics which is intended to be used by other types of personnel like those who are planning staffing for a store or those who are planing and maintaining office spaces.  

### GO/STOP dashboard

The first dashboard that I have created is a simple GO/STOP dashboard that shows "GO" in green if *currentOccupancy* is below *maxOccupancy*. If the *currentOccupancy* is above *maxOccupancy* the background turns to red, and it says "STOP" instead. This dashboard fully event based and updates its data in realtime. It is intended to run in full screen on a display/monitor in the store/office.

[![GO](/assets/images/axis-thingsboard-peoplecounting-part4/go_stop.png)](/assets/images/axis-thingsboard-peoplecounting-part4/go_stop.png)

### Statistics dashboard

The statistics dashboard shows a map, in which all the buildings are mapped based on the longitude/latitude metadata added on the *building asset*. The number above the building icon is the current occupancy number for all floors combined in that building.

[![GO](/assets/images/axis-thingsboard-peoplecounting-part4/map.png)](/assets/images/axis-thingsboard-peoplecounting-part4/map.png)

When clicking a building (we only have one in this example), a new dashboard popups in which I have added some charts and cards to display statistics on the data we received from the devices, and data that we created thru the rule chains.

This dashboard also shows if devices are online/offline based on their LWT MQTT message. It shows Occupancy graphs as well as graphs for when occupancy was above the maximum allowed number.

We can also see how the *relation tree* looks like, all the way from the building > floor > entrance > device which helps us to verify that all relations are configured correctly.

[![GO](/assets/images/axis-thingsboard-peoplecounting-part4/details.png)](/assets/images/axis-thingsboard-peoplecounting-part4/details.png)

### Conclusion

This was the final part in this blog series.

My idea when creating this series was to show how easy it can be to build a *IoT* solution with components that are available *out of the box*, and that it doesn't need to be "built from scratch".

In this article we configured the Axis cameras to send it's data on MQTT as *Event's*. The Axis devices however has built in functionality to send its data on a interval instead of realtime. When using intervals, Axis will keep the history on the device it self until it know that it successfully was received by the recipient.

ThingsBoard as an IoT platform can handle a lot of different use-cases. This specific use-case can also be expanded to contain data from more sources/devices. - Lets say that you want to track Co2 or sound levels in the same Office, then it might be interesting to know how many people it was in the office/room when the Co2 went from good to bad for example.

I hope that you found this blog series interesting!
