---
layout: single
#classes: wide
toc: true
title:  "Assets and Relations - People Counter Axis/ThingsBoard"
date:   2021-05-29 15:00:00 +0200
categories: blog
tags: 
- IoT
- Axis
- ThingsBoard
---

In [Part 1]({% post_url 2021-05-25-axis-thingsboard-peoplecounting-part1 %}) of this blog series, we configured the Axis camera and the ThingsBoard platform so that ThingsBoard successfully received People Counter data from the Axis camera which then was saved on the *device* in ThingsBoard.

In this part 2 of this blog, I will show how we can utilize *Assets* as a *digital twin* to build a relation between the building, floors, entrance/exit and the actual camera device in ThingsBoard.

By using *assets* along with *relations* in ThingsBoard, we can control how the *rule chain* should aggregate data from/to each *device* and *asset*. *Assets* also removes the need of being bound to one specific device since the platform will copy all data from the device to the parent asset. This means that we can replace the device without loosing any historic data bound to that specific device.

Lets start with an example: We have a building named "HQ", in this building we have 2 floors. Each floor has one main entrance/exit as well as one secondary entrance/exit. To have full coverage here, we would need 4 cameras, one for each entrance/exit and each floor.

In the example above, since we have two entrances/exists for each floor, in order to determine occupancy, we need to perform the occupancy calculation for each floor and not on the camera object or entrance/exit object. This is were the *floor asset* will help us to aggregate the two entrances into one object - the floor. For this, we need to configure a *asset* that represents each floor and *assets* for each entrance/exit then link the camera device to the corresponding entrance/exit.

What we also want to do is to create a *asset* for the building itself, to which we can link all floors. With these assets and relations created, we now have a relation between the building > floors > entrance/exit > camera devices.

After all *Assets* has been created, we have this:

![Assets]({{ BASE_PATH }}/assets/images/axis-thingsboard-peoplecounting-part2/assets.png)

On each asset, a *To/From* relationship to create the hierarchy of Building > Floors > Entrance/Exit and then a relation was configured between each entrance/exit and the corresponding camera device.

![Asset Relations]({{ BASE_PATH }}/assets/images/axis-thingsboard-peoplecounting-part2/AssetsRelations.png)

Now we have all assets and relations in place we continue in [part 3]({% post_url 2021-05-25-axis-thingsboard-peoplecounting-part3 %}) with the work with the *rule chain*.
