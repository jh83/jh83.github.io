---
layout: single
#classes: wide
toc: true
title:  "Sending camera "
date:   2022-08-21 17:00:00 +0200
categories: blog
tags: 
- IoT
- LoRaWan
- MongoDB
- Atlas
- The Things Network
- TTN
- TTS
- TTI
---

In this blog post I will show how data can be sent between LoRaWAN devices connected thru The Things Network and a MongoDB database with the help of the components available in MongoDB Atlas cloud service.

This blog post will cover:

* Receive (Uplink) telemetry data sent from a LoRaWAN device and store it in a MongoDB timeseries database collection.
* Receive (Uplink) settings/config data sent from a LoRaWAN device and store it in a "device twin" database collection.
* Send changes which occurs on the "device twin" database documents as downlinks to the LoRaWAN device.
* Decode/Encode the LoraWAN device payload in a MongoDB Atlas Function.


### MongoDB Atlas

MongoDB Atlas is a cloud service which contains a number of services which (apart from the database) can be used to create a complete solution.

In this blog post, we will use the following components in MongoDB Atlas:

* MongoDB - The database service in which we will store our data.
* App Services:
  * HTTPS Endpoint - An HTTPS endpoint will be created to which we will tell TTN to send the data coming from the LoRaWAN device.
  * Triggers - An "OnChange" trigger will initiate a function when a change occurs.
  * Functions - Two functions will be used. One will be triggered by TTN as a Webhook and the other function will be triggered when there is a change on the database collection.

Let's get started!

### MongoDB

#### Create cluster

We need to create a cluster which will host our database:

![Create Cluster]({{ BASE_PATH }}/assets/images/mongodbatlas-ttn/CreateCluster.png)

#### Create database and collections

Create the database which will hold our data. During database creation, we also create the collection for our time series data

Time series collections requires us manually specify the timeField:

![Create Database]({{ BASE_PATH }}/assets/images/mongodbatlas-ttn/CreateDatabase.png)

We need one more collection to hold our "device twin" data. This collection will hold a *document* for each device, and the document will contain the *desired* and *reported* device settings:

![Additional Collection]({{ BASE_PATH }}/assets/images/mongodbatlas-ttn/AdditionalCollection.png)

Once the *deviceTwins* collection has been created, then we insert a new *document* into the database with a *_id* that matches the LoRaWAN device ID in The Things Network

The *document* contains a "*desired*" which controls how we WANT the device to be configured. The "*reported*" part of the document contains the configuration reported by the LoRaWAN device:

```json
{
    "_id": "eui-70b3d57ed0051cf5",
    "settings": {
        "desired": {
            "led_green": false,
            "led_red": false,
            "send_interval": {
                "$numberInt": "1"
            },
            "version": {
                "$numberInt": "1"
            }
        },
        "reported": {
        }
    }
}
```

![Insert device document]({{ BASE_PATH }}/assets/images/mongodbatlas-ttn/InsertDeviceTwinDocument.png)

### MongoDB App Services

First we need to create a new *"App Service"* in which we can create *HTTPS Endpoints*, *Triggers* and *Functions* later on.

#### Create App Service

Create a new App Service thru the web portal:

![Create App Service]({{ BASE_PATH }}/assets/images/mongodbatlas-ttn/CreateAppService.png)

#### Create Functions

We will need two functions. One which will be triggered by the webhook initiated from The Things Network, and a second one which will be triggered when there is a change on a document in the *deviceTwins* collection.

First we create the Function which will be executed by the TTN webhook. This function will take the input from TTN, decode the BASE64 payload and insert the data into the *telemetry* collection.

NOTE: In this example we save all the data that we receive. Usually you want to filter it and only store the data which is of importance.

![Create "fromTTN" function]({{ BASE_PATH }}/assets/images/mongodbatlas-ttn/CreateFunction1.png)

Navigate to "Function editor" and insert the code below:

```json
// This function is the endpoint's request handler.
exports = function ({ query, headers, body }, response) {

    // Headers, e.g. {"Content-Type": ["application/json"]}
    const contentTypes = headers["Content-Type"];

    // Raw request body (if the client sent one).
    // This is a binary object that can be accessed as a string using .text()
    const reqBody = body;

    console.log("Content-Type:", JSON.stringify(contentTypes));
    console.log("Request body:", reqBody.text());

    // Result
    let res;

    const jsonBody = JSON.parse(reqBody.text())
    jsonBody.received_at = new Date(jsonBody.received_at)

    // Check that incoming message is a message from a device
    if (jsonBody.uplink_message.f_port) {

        let buff = Buffer.from(jsonBody.uplink_message.frm_payload, 'base64').toString("hex");
        let hexArray = parseHexString(buff)

        // lorawan port 1 contains telemetry data and should be inserted into telemetry collection
        if (jsonBody.uplink_message.f_port == 1) {
            jsonBody.telemetry = hexArray;

            context.services.get("mongodb-atlas").db("IoT").collection("telemetry").insertOne(jsonBody)
                .then(result => {
                    console.log(`Successfully inserted telemetry item with _id: ${result.insertedId}`);
                    res = result.insertedId;
                })
                .catch(err => {
                    console.error(`Failed to insert telemetry item: ${err}`)
                    res = err;
                });
        }

        // lorawan port 2 contains echoed settings and sould be inserted under "reported" in the "deviceTwins" collection
        else if (jsonBody.uplink_message.f_port == 2) {

            context.services.get("mongodb-atlas").db("IoT").collection("deviceTwins").updateOne(
                {
                    _id: jsonBody.end_device_ids.device_id
                },
                {
                    $set: {
                        "settings.reported": {
                            led_green: hexArray[0],
                            led_red: hexArray[1],
                            send_interval: hexArray[2],
                            version: hexArray[3],
                            updated_ts: jsonBody.received_at
                        }
                    }
                })
                .then(result => {
                    console.log(`Successfully inserted settings item with _id: ${JSON.stringify(result)}`);
                    res = result.insertedId;
                })
                .catch(err => {
                    console.error(`Failed to insert settings item: ${err}`)
                    res = err;
                });
        }
    }

    // The return value of the function is sent as the response back to the client
    // when the "Respond with Result" setting is set.
    return res;
};

// Helper function. Creates hex array from hex string
function parseHexString(str) {
    var result = [];
    while (str.length >= 2) {
        result.push(parseInt(str.substring(0, 2), 16));
        str = str.substring(2, str.length);
    }

    return result;
}
```

![Create "fromTTN" function]({{ BASE_PATH }}/assets/images/mongodbatlas-ttn/CreateFunction1-1.png)

Click "Review and deploy".

Next we create the function which will be executed when there is a change in the *deviceTwins* collection:

![Create "toTTN" function]({{ BASE_PATH }}/assets/images/mongodbatlas-ttn/CreateFunction2.png)

On the "Function editor" tab. Insert the following code which will:

* Verify that the *desired* version number differs from the *reported* one in the changed document avoid loops.
* Encode the *settings.desired* to a BASE64 string.
* Send the data thru a HTTP POST to The Things Network.

```json
exports = async function (changeEvent) {
  
  // Variables loaded from "values":
  const ttnBaseURL = context.values.get("ttnBaseURL");
  const ttnApplicationId = context.values.get("ttnApplicationId");
  const ttnWebhookName = context.values.get("ttnWebhookName");
  const ttnDownlinkApiKey = context.values.get("ttnDownlinkApiKey");
  
  const docId = changeEvent.documentKey._id;
  console.log("docId:", docId);

  const fullDocument = changeEvent.fullDocument;

  // verify that settings.desired.version doesnt match settings.reported.version to avoid loops
  if (fullDocument.settings.desired.version != fullDocument.settings.reported.version) {
      
      // Create an array which contains the settings to be sent
      let array = new Uint8Array(4);
      array[0] = fullDocument.settings.desired.led_green;
      array[1] = fullDocument.settings.desired.led_red;
      array[2] = fullDocument.settings.desired.send_interval;
      array[3] = fullDocument.settings.desired.version;

      // We need to encode the data from HEX > BASE64. This could be done in TTN aswell,
      // but for this demo we will do it in this function
      var base64String = Buffer.from(createHexString(array), 'hex').toString('base64')
      console.log("base64String", base64String);

      // Build the body object which will be sent to the TTN webhook.
      const body = {
          "downlinks": [{
              "frm_payload": base64String,
              "confirmed": true,
              "f_port": "15",
              "priority": "NORMAL"
          }]
      }

      // Build TTN API authorization Bearer
      const bearer = "Bearer " + ttnDownlinkApiKey;
      
      // Call the TTN webhook and tell it to send the data to the device.
      const response = await context.http.post({
          url: ttnBaseURL + "/api/v3/as/applications/" + ttnApplicationId + "/webhooks/" + ttnWebhookName + "/devices/" + fullDocument._id + "/down/replace",
          body: body,
          headers: { "Content-Type": ["application/json"], "Authorization": [bearer] },
          encodeBodyAsJSON: true
      })  

      console.log("response:", response.status);
  }
  else {
      console.log("Required and reported versions match. Nothing to do....:", docId)
  }
};

// Helper function to create a hexString from an array
function createHexString(arr) {
    var result = "";
    for (var i in arr) {
        var str = arr[i].toString(16);
        str = str.length == 0 ? "00" :
              str.length == 1 ? "0" + str : 
              str.length == 2 ? str :
              str.substring(str.length-2, str.length);
        result += str;
    }
    return result;
}

```

![Create "toTTN" function]({{ BASE_PATH }}/assets/images/mongodbatlas-ttn/CreateFunction2-2.png)

#### Create HTTPS Endpoint

We need a HTTPS Endpoint. We will tell TTN to send its data to this endpoint later on. Here we select the "fromTTN" function created earlier:

![Create HTTPS Endpoint]({{ BASE_PATH }}/assets/images/mongodbatlas-ttn/CreateHttpsEndpoint.png)

NOTE: Copy the URL generated. We need it when we configure TTN later.

#### Create API Key

A *API-KEY* required for authorization of TTN is needed. We create one thru "App Users".

NOTE: Copy the key, you will need it later.

![Create API key]({{ BASE_PATH }}/assets/images/mongodbatlas-ttn/CreateApiKey.png)

Copy the uniq ID of the *User*. We need this ID later when we create the permissions under *rules*:

![API key]({{ BASE_PATH }}/assets/images/mongodbatlas-ttn/Apikey.png)

#### Create Rules

Rules are *permissions* that we need to configure to allow our functions - initiated with the "API-KEY" *user* - to write to the database collections.

We will delegate *insert* permissions onto the *deviceTwins* and *telemetry* collection with the help of the *id* on the API-KEY user we created earlier:

![Rule - Telemetry]({{ BASE_PATH }}/assets/images/mongodbatlas-ttn/RuleTelemetry.png)

![Rule - deviceTwins]({{ BASE_PATH }}/assets/images/mongodbatlas-ttn/RuleDeviceTwin.png)

#### Add Trigger

A *trigger* will enable us to run a function when something occurs in the database. We want it to trigger our "toTTN" function when a document in the *deviceTwins* collection is changed:

![Create Trigger]({{ BASE_PATH }}/assets/images/mongodbatlas-ttn/CreateTrigger.png)

#### Add Values

The "toTTN" function requires a set of *values* to be declared to work as expected. These are values that we receive from the TTN console.

The TTN webhook needs to be configured on the TTN console before we can add any *values*.

In TTN, add a new webhook:

* "Base URL" should point to the URL we got earlier when creating the "HTTPS Endpoint".
* "Additional headers" needs to be configured since MongoDB HTTPS Endpoint expects a header named "API-KEY" containing the key.
* In this demo, we only want "Uplink message"

![Add webhook]({{ BASE_PATH }}/assets/images/mongodbatlas-ttn/ttnAddWebhook.png)

In TTN, add a new API key. Take note of the key generated since we need to insert it into MongoDB *values* later:

![Add webhook]({{ BASE_PATH }}/assets/images/mongodbatlas-ttn/ttnAddApiKey.png)

Now we can add all data into *values*:

![Value TTN URL]({{ BASE_PATH }}/assets/images/mongodbatlas-ttn/ValueTtnUrl.png)

![Value TTN AppID]({{ BASE_PATH }}/assets/images/mongodbatlas-ttn/ValueTttnAppId.png)

![Value TTN Webhook name]({{ BASE_PATH }}/assets/images/mongodbatlas-ttn/ValuesTtnWebhook.png)

![Add TTN secret]({{ BASE_PATH }}/assets/images/mongodbatlas-ttn/ValueSecret.png)


### Testing

All done. Time for testing.

The uplink messages from the LoRaWAN device should now start to populate the *telemetry* collection:

![Received telemetry]({{ BASE_PATH }}/assets/images/mongodbatlas-ttn/receivedTelemetry.png)

If any change is made in the document, the *trigger* that we have created is executed. The trigger function checks if the version number under *desired* vs *reported* matches or not. If the version numbers doesn't match then a downlink to the device is scheduled thru the TTN webhook. The LoRaWAN device is working as a Class-A device, downlink will occur after the next uplink:

![Pending twin update]({{ BASE_PATH }}/assets/images/mongodbatlas-ttn/deviceTwinPending.png)

The LoRaWAN device runs a custom firmware which is configured to echo back the settings that it receives, And after the device receives the downlink and echos it back, we see that the same version number under *reported*:

![Twin updated]({{ BASE_PATH }}/assets/images/mongodbatlas-ttn/deviceTwinUpdated.png)
