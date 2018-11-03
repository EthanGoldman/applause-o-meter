#include "LPD8806.h"
#include "SPI.h"

/* Example to control LPD8806-based RGB LED Modules in a strip
 *  Creates an applause-o-meter from a sound meter 
 *  with high-water-mark record level and 
 *  blinking alert upon setting new record.
 *  Designed to work with sound meter like the Extech 407736
 *  that outputs analog 0 - 2.5 VDC (10mv / dB).
 */


/*****************************************************************************/

// Number of RGB LEDs in strand{}Y
int nLEDs = 160;

// Chose 2 pins for output; can be any valid output pins:
int dataPin  = 2;
int clockPin = 3;
int sensorPin = 0;
int sensorValue = 0;
int sensorDbValue = 0;
float voltage = 0;
float percentOfMaxDb = 0.0;

// for keeping track of the max since last plugged in
// TODO: add a reset button that sets this back to zero
float recordHighestPercentOfDb = 0.0;
// set this to the minimum dB that will trigger blinking when it sets a record
int minRecordDbBlinkThresh = 70;
// set for how many times to blink
int recordSetBlinkCount = 3;
int remainingBlinkCount = 0; // counter for blinker
bool blinkStatus = false;

// meter's low range starts at 35
// set higher to "zoom in" on active range in loud room; sets strip min dB to register
float sensorDbMin = 55.0;
// meter's low range maxes out at 90, high at 130; 
// set this to dB that maxes out strip display
float sensorDbMax = 120.0;

// NOTE: samplingDuration must be an integer multiple of samplingInterval
int samplingDuration = 500; // milliseconds to gather samples before updating display
int samplingInterval = 50; // milliseconds between individual samples (averaged over duration)

// First parameter is the number of LEDs in the strand.  The LED strips
// are 32 LEDs per meter but you can extend or cut the strip.  Next two
// parameters are SPI data and clock pins:
LPD8806 strip = LPD8806(nLEDs, dataPin, clockPin);

// You can optionally use hardware SPI for faster writes, just leave out
// the data and clock pin parameters.  But this does limit use to very
// specific pins on the Arduino.  For "classic" Arduinos (Uno, Duemilanove,
// etc.), data = pin 11, clock = pin 13.  For Arduino Mega, data = pin 51,
// clock = pin 52.  For 32u4 Breakout Board+ and Teensy, data = pin B2,
// clock = pin B1.  For Leonardo, this can ONLY be done on the ICSP pins.
//LPD8806 strip = LPD8806(nLEDs);

void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);

  // Start up the LED strip
  strip.begin();

  // Update the strip, to start they are all 'off'
  strip.show();
}


void loop() {
  // measure the sensor input in percent of max dB
  sensorDbValue = sensorReadDb(sensorPin, samplingDuration);
  // convert to percent
  percentOfMaxDb = (sensorDbValue - sensorDbMin) / (sensorDbMax - sensorDbMin);
  Serial.print("reading: ");
  Serial.print(percentOfMaxDb);

  // check for a new record
  if (percentOfMaxDb > recordHighestPercentOfDb) {
    recordHighestPercentOfDb = percentOfMaxDb;
    if (sensorDbValue >= minRecordDbBlinkThresh) {
      remainingBlinkCount = recordSetBlinkCount;
    }
  }
  
  if (remainingBlinkCount > 0) {
    if (blinkStatus == true) {
      // wipe it all out to turn off
      colorFillPercent(strip.Color(0,   0,   0), recordHighestPercentOfDb);
      blinkStatus = false;
      remainingBlinkCount -= 1; // reduce the count
    } else {
      rainbowFillPercent(percentOfMaxDb, 0);
      blinkStatus = true;
    }
  } else {
    rainbowFillPercent(percentOfMaxDb, 0);
  }
  
  Serial.print(", record: ");
  Serial.println(recordHighestPercentOfDb);
  colorMarkPercent(strip.Color(255,   0,   0), recordHighestPercentOfDb);

  strip.show();
}

///////////////////////////////////////////
// FUNCTIONS 
///////////////////////////////////////////

float sensorReadDb(int sensorPin, int samplingDuration) {
  int sampleCount = samplingDuration / samplingInterval;
  int sampleSum = 0;
  for (int t=0; t<sampleCount; t++) {
    // read the sound meter;
    sensorValue = analogRead(sensorPin);
    sampleSum += sensorValue; // add to sum of samples so far
    delay(samplingInterval);
  } 
  int sampleAverage = sampleSum / sampleCount;
  voltage = sampleAverage * (5.0 / 1023.0);
  sensorDbValue = voltage * 100; // 10 mV per dB
  return sensorDbValue;
}


// Fill the dots up to a percent along the strip.
void rainbowFillPercent(float fillPercent, int colorOffset) {
  int i;
  int fillHeight = strip.numPixels() * fillPercent;

  for (i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0,   0,   0)); // fill with "off"
  }

  for (i=0; i < fillHeight; i++) {
      strip.setPixelColor(i, Wheel( ((i * 384 / strip.numPixels()) + colorOffset) % 384) );
  }
}

// Fill the dots up to a percent along the strip.
void colorFillPercent(uint32_t c, float fillPercent) {
  int i;
  int fillHeight = strip.numPixels() * fillPercent;

  for (i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0,   0,   0)); // fill with "off"
  }

  for (i=0; i < fillHeight; i++) {
      strip.setPixelColor(i, c); // fill to height with color
  }
}

// Mark the dot at a given percent along the strip.
void colorMarkPercent(uint32_t c, float markPercent) {
//  Serial.println(markPercent);
  int markHeight = int(strip.numPixels() * markPercent);
//  Serial.println(markHeight);
  for (int i=0; i<3; i++) { // add three dots as athe marker
    if (markHeight + i < strip.numPixels()) { // don't fill past the end
      strip.setPixelColor(markHeight + i, c);
    }
  }
}

/* Helper functions */

//Input a value 0 to 384 to get a color value.
//The colours are a transition r - g -b - back to r

uint32_t Wheel(uint16_t WheelPos)
{
  byte r, g, b;
  switch(WheelPos / 128)
  {
    case 0:
      r = 127 - WheelPos % 128;   //Red down
      g = WheelPos % 128;      // Green up
      b = 0;                  //blue off
      break; 
    case 1:
      g = 127 - WheelPos % 128;  //green down
      b = WheelPos % 128;      //blue up
      r = 0;                  //red off
      break; 
    case 2:
      b = 127 - WheelPos % 128;  //blue down 
      r = WheelPos % 128;      //red up
      g = 0;                  //green off
      break; 
  }
  return(strip.Color(r,g,b));
}
