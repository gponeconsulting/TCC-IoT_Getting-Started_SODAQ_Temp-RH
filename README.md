# ThingSpeak Guide + temperature / humidity sensor guide (sodaq explorer)
This guide is a continuation of the `SODAQ ExpLoRer Guide` and will make use of the application and device set up during the guide.

## What you will need
To Follow along with this guide you will need the following things:
- A SODAQ ExpLoRer which can be bought [here](https://shop.sodaq.com/explorerrn2903a-us.html)
- A micro usb connector which is included with the SODAQ ExpLoRer
- A Keyestudio DHT22 Temperature and Humidity Sensor Module
- A breadboard and 3 wires OR some other way to connect the sensor to the SODAQ ExpLoRer
- A computer to connect to the SODAQ ExpLoRer and write the code


## Step 1 - Set up ThingSpeak
To follow this guide you will need the following:
- First create a ThingSpeak account [here](https://thingspeak.com/)
- After creating an account and signing in, go to the My Channels page by clicking `Channels > My Channels` on the top nav bar
- Click the New Channel button
- In the Name textbox enter an appropriate name like `Temperature / Humidity Sensor`
- Ensure that the first 3 fields have a tick in their checkbox
- In field 1 enter `Humidity`
- In field 2 enter `temperature`
- In field 3 enter `Heat Index`
- The rest of the options can be left blank for now so scroll to the bottom of the page and press the `Save Channel` button
- This will take you to the private view of your new channel where you will have a graph for each of the fields, we entered earlier

## Step 2 - Connect ThingSpeak to The Things Network
Now a ThingSpeak account has been created, and channel has been set up, we need to get our The Things Network Application to send any data it receives to our new channel.
- Sign into The Things Network
- Go to `Console > Applications` and select the application that you want to connect to ThingSpeak 
- Click on the `Integrations` heading
- Click on the `add integration` button
- Click on the `ThingSpeak` option
- Enter a unique name for the integration process in the `Process ID` field
- Copy the write API key from the `API Keys` section of the ThingSpeak channel created in Step 1 into the `Authorization` field
- Copy the `Channel ID` from the top of the ThingSpeak Channel created in Step 1 into the `Channel ID` field
- Click `Add Integration`

## Step 3 - Setup the board
To connect the sensor to the Arduino we will be using a breadboard.
A breadboard has a number of rows typically labeled with numbers and a number of columns typically labeled with letters.
- Insert the pins from the DHT22 Sensor into the breadboard so that they all enter a column of the same letter.
- In the same numbered row that the sensors GND pin is connected to insert a wire into the breadboard and connect it to one of the GND sockets on the Sodaq Explorer.
- In the same numbered row that the sensors VCC pin is connected to insert a wire into the breadboard and connect it to the 5V socket on the Sodaq Explorer.
- In the same numbered row that the sensors S (or DAT) pin is connected to insert a wire into the breadboard and connect it to the D2 socket on the Sodaq Explorer.

## Step 4 - Setup the Code
Now that the sensor is connected to the Sodaq Explorer the code needs to be modified from the `Sodaq Explorer Guide` to send the sensor data instead of the static letters in the myData variable.
- Open a copy of the `SendOTAA` file you created in the `SODAQ ExpLoRer Guide`
- Goto `Tools > Manage Libraries`
- Search for and install the `DHT sensor library` and `Adafruit Unified Sensor` libraries
- Click close
- Enter `#include "DHT.h"` at the top of the file under the `#include <TheThingsNetwork.h>` line
- After the `#define freqPlan TTN_FP_AU915` line include the following
```
#define DHTPIN 2 // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

DHT dht(DHTPIN, DHTTYPE);
```
- At the bottom of the `setup()` function add `dht.begin();`
- Replace the loop function with the following code
```
void loop()
{
  debugSerial.println("-- LOOP");

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  
  // Check if any reads failed and exit early (to try again).
  if (isnan(humidity) || isnan(temperature)) {
    debugSerial.println(F("Failed to read from DHT sensor!"));
    return;
  }
 
  // Multiply by 100 to convert to remove decimal places
  int humidityInt = humidity * 100;
  int temperatureInt = temperature * 100;
  int heatIndexInt = dht.computeHeatIndex(temperature, humidity, false) * 100;

  
  //Break the humidity, temperature and heat index into Bytes in individual buffer arrays
  byte payloadA[2];
  payloadA[0] = highByte(humidityInt);
  payloadA[1] = lowByte(humidityInt);
  byte payloadB[2]; 
  payloadB[0] = highByte(temperatureInt);
  payloadB[1] = lowByte(temperatureInt);
  byte payloadC[2]; 
  payloadC[0] = highByte(heatIndexInt);
  payloadC[1] = lowByte(heatIndexInt);

  //Get the size of each buffer array (in this case it will always be 2, but this could be used if they were variable)
  int sizeofPayloadA = sizeof(payloadA);
  int sizeofPayloadB = sizeof(payloadB);
  int sizeofPayloadC = sizeof(payloadC);

  //Make a buffer array big enough for all the values
  byte payload[sizeofPayloadA + sizeofPayloadB + sizeofPayloadC];

  //Add each of the individual buffer arrays the single large buffer array
  memcpy(payload, payloadA, sizeofPayloadA);
  memcpy(payload + sizeofPayloadA, payloadB, sizeofPayloadB);
  memcpy(payload + sizeofPayloadA + sizeofPayloadB, payloadC, sizeofPayloadC);

  //Print some outputs for debugging
  debugSerial.print(F("Humidity: "));
  debugSerial.print(humidityInt);
  debugSerial.print(F("%  Temperature: "));
  debugSerial.print(F("°C "));
  debugSerial.print(temperatureInt);
  debugSerial.print(F(" Heat index °C "));
  debugSerial.println(heatIndexInt);
  
  // Send it off
  ttn.sendBytes(payload, sizeof(payload));

  //Wait 60 second between readings
  delay(60000);
}
```
### What does this do?
Every 60 seconds this will take the Humidity and Temperature from the sensor.
Then it will calculate the heat index and multiply the humidity, temperature, and heat index by 100 so that the integers now have 2 decimal points of precision and we can convert them back later on.
It then goes through a process of converting the numbers into a buffer array of bytes that can be sent to The Things Network. You can learn more about this process [here](https://www.thethingsnetwork.org/docs/devices/bytes.html).

## Step 5 - Decoding the message
Now that we have encoded the message has been encoded and sent it to The Things Network, it needs to be told what to do with it.

- In your application on The Things Network, go to the tab named `Payload Formats`

- In here the code can be written to decrypt the data from our device.
- Enter the following into the decoder
```
function Decoder(bytes, port) {
  // Decode an uplink message from a buffer
  var decoded = {};
  //Separate the individual numbers into individual buffers
  var Val1 = bytes.slice(0, 2);
  var Val2 = bytes.slice(2, 4);
  var Val3 = bytes.slice(4, 6);
  
  //Convert the buffers into numbers and divide by 100 to return them to 2 decimal places
  //Then save them to the decoded array
  decoded.myValA = ((Val1[0] << 8) + Val1[1]) / 100;
  decoded.myValB = ((Val2[0] << 8) + Val2[1]) / 100;
  decoded.myValC = ((Val3[0] << 8) + Val3[1]) / 100;

  //return the values
  return {
    field1: decoded.myValA,
    field2: decoded.myValB,
    field3: decoded.myValC,
  };
}
```
The Code first breaks the long buffer array we created in step 4 into the buffers that make up the Humidity, temperature, and heat index. 
It then decodes each of the buffers into numbers, divides the numbers by 100 to turn them back into their original values and saves them into the decoded array.
Finally the numbers are returned as field 1, field 2 and field 3 so they can be sent to ThingSpeak.

- Click the `Save payload functions` button

## Step 6 - Testing everything
If everything up to this point has gone well, all that’s left to do is start the program.
- Connect the Sodaq Explorer to your computer using the micro usb cable.
- In the Arduino IDE ensure that the correct port is selected by going to `Tools > Port:` and check that the Sodaq port is selected
- Ensure that the correct board is selected by going to `Tools > Board: > SODAQ SAMD (32-bits ARM Cortex-M0+) Boards > SODAQ ExpLoRer`
- Finally click the arrow button in the top left to upload your code to the Sodaq explorer

After it has finished uploading you can check the monitor at `Tools > Serial Monitor` to see if it is working.
You can also now go to the Private view of the ThingSpeak channel you set up earlier and see that a new data entry will be entered every 60 seconds