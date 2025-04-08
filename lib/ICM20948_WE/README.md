# ICM20948_WE
An Arduino library for the ICM-20948 9-axis accelerometer, gyroscope and magnetometer. It contains many example sketches with lots of comments to make it easy to use. It works with I2C and SPI.

I have not implemented DMP features and most probably I won't do that in future. That would exceed the time I can invest. 

You can find a documentation in my blog:

https://wolles-elektronikkiste.de/icm-20948-9-achsensensor-teil-i (German)

https://wolles-elektronikkiste.de/en/icm-20948-9-axis-sensor-part-i (English)

If you find bugs please inform me. If you like the library it would be great if you could give it a star.

If you are not familiar with the ICM20948 I recommend to work through the example sketches.

When you wire the ICM-20948 you need to consider that VDD is 3.3 volts, but VDDIO is only 1.71-1.95 volts (see data sheet). For a 5V MCU board, I used a level shifter and additional resistors to GND which form a voltage divider together with the internal resistors of the level shifter.

<h3>Important note on release 1.2.2</h3>
Version 1.2.2 is not fully compatible with former versions. Many functions like getGValues() returned xyzFloat structures. To be exact, the functions did return pointers to the structures which were created by library functions. But after returning from the functions the memory space where the structures were located can be overwritten. I have changed that by passing the xyzFloat variables by reference. Here is an example:  


````
xyzFloat gValue = myIMU.getGValues(); // for versions < 1.2.2
````
changed to:

````
xyzFloat gValue;  // for versions >= 1.2.2
myIMU.getGValues( &gValue )
````

I am sorry for the inconvience. But the changes to be made to existing sketches to make them work with version 1.2.2 are really limited. All example sketches are changed accordingly. 

<h3>(Formerly) Known issue</h3>
Before version 1.2.0, using my library caused sporadic connection issues after re-powering. This has been solved by setting up the magnetometer as SLV4. The magnetometer data reading is still done using the magnetometer as SLV0. Please inform me if you should still have issues.      
