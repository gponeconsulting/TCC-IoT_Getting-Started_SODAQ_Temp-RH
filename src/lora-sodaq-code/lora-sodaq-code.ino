#include <TheThingsNetwork.h>
#include <Wire.h>
I2CSoilMoistureSensor sensor;

// Set your AppEUI and AppKey
const char *appEui = "0000000000000000";
const char *appKey = "00000000000000000000000000000000";

#define loraSerial Serial2
#define debugSerial SerialUSB

// Replace REPLACE_ME with TTN_FP_EU868 or TTN_FP_US915
#define freqPlan TTN_FP_AU915

TheThingsNetwork ttn(loraSerial, debugSerial, freqPlan);

void setup()
{
  Wire.begin();
  loraSerial.begin(57600);
  debugSerial.begin(9600);
 
  // Wait a maximum of 10s for Serial Monitor
  while (!debugSerial && millis() < 10000)
    ;
 
  sensor.begin(); // reset sensor
  delay(1000); // give some time to boot up
  debugSerial.print("I2C Soil Moisture Sensor Address: ");
  debugSerial.println(sensor.getAddress(),HEX);
  debugSerial.print("Sensor Firmware version: ");
  debugSerial.println(sensor.getVersion(),HEX);
  debugSerial.println();
 
  debugSerial.println("-- STATUS");
  ttn.showStatus();
 
  debugSerial.println("-- JOIN");
  ttn.join(appEui, appKey);
}
 
void loop()
{
  debugSerial.println("-- LOOP");
 
  while (sensor.isBusy()) delay(50);
  int soilMoistureCapacitance = sensor.getCapacitance();
  int temperature = sensor.getTemperature();
  int light = sensor.getLight(true);
  sensor.sleep(); //sleep the sensor to save power
  
  debugSerial.print("Soil Moisture Capacitance: ");
  debugSerial.print(soilMoistureCapacitance); //read capacitance register
  debugSerial.print(", Temperature: ");
  debugSerial.print(temperature /(float)10); //temperature register
  debugSerial.print(", Light: ");
  debugSerial.println(light); //request light measurement, wait and read light register

  //Break the soilMoistureCapacitance, temperature and light into Bytes in individual buffer arrays
  byte payloadA[2];
  payloadA[0] = highByte(soilMoistureCapacitance);
  payloadA[1] = lowByte(soilMoistureCapacitance);
  byte payloadB[2]; 
  payloadB[0] = highByte(temperature);
  payloadB[1] = lowByte(temperature);
  byte payloadC[2]; 
  payloadC[0] = highByte(light);
  payloadC[1] = lowByte(light);

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
 
  // Send it off
  ttn.sendBytes(payload, sizeof(payload));
 
  delay(10000);
}
