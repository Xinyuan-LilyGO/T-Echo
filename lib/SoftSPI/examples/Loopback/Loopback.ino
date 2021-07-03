#include <SPI.h>
#include <SoftSPI.h>

// Create a new SPI port with:
// Pin 2 = MOSI,
// Pin 3 = MISO,
// Pin 4 = SCK
SoftSPI mySPI(2, 3, 4);

void setup() {
	mySPI.begin();
	Serial.begin(9600);
}

void loop() {
	static uint8_t v = 0;

	Serial.print("Sending value: ");
	Serial.print(v, HEX);
	uint8_t in = mySPI.transfer(v);
	Serial.print(" Got value: ");
	Serial.print(in, HEX);
	Serial.println(v == in ? " PASS" : " FAIL");
	delay(1000);
	v++;
}
