/*
  SD card read/write
 
 This example shows how to read and write data to and from an SD card file 	
 The circuit:
 * SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 4
 
 created   Nov 2010
 by David A. Mellis
 modified 9 Apr 2012
 by Tom Igoe
 
 This example code is in the public domain.
 	 
 */
//SD CARD
#include <SD.h>
File myFile;

//MICROPHONE
const int sampleWindow = 500; // Sample window width in mS (500 mS = 20Hz)
unsigned int sample;

//HUMIDITY and TEMPERATURE Sensor
#include <idDHT11.h>
int idDHT11pin = 2; //Digital pin for comunications
int idDHT11intNumber = 0; //interrupt number (must be the one that use the previus defined pin (see table above)

//declaration
void dht11_wrapper(); // must be declared before the lib initialization

// Lib instantiate
idDHT11 DHT11(idDHT11pin,idDHT11intNumber,dht11_wrapper);


//CLOCK
#include <TimerOne.h>
#include "TM1637.h"
#define ON 1
#define OFF 0

int8_t TimeDisp[] = {
  0x00,0x00,0x00,0x00};
unsigned char ClockPoint = 1;
unsigned char Update;
unsigned char halfsecond = 0;
unsigned char second;
unsigned char minute = 00;
unsigned char hour = 00;


#define CLK 9//pins definitions for TM1637 and can be changed to other ports    
#define DIO 8
TM1637 tm1637(CLK,DIO);

//LUMINOSITY
int sensorLumPin = A2;    // select the input pin for the luminosity
int sensorLumValue = 0;  // variable to store the value coming from the sensor


int count=0;
void setup()
{
  // Open serial communications and wait for port to open:
  //Serial.begin(9600);
  //while (!Serial) {
  //  ; // wait for serial port to connect. Needed for Leonardo only
  //}


  //Serial.print("Initializing SD card...");
  // On the Ethernet Shield, CS is pin 4. It's set as an output by default.
  // Note that even if it's not used as the CS pin, the hardware SS pin 
  // (10 on most Arduino boards, 53 on the Mega) must be left as an output 
  // or the SD library functions will not work. 
  pinMode(10, OUTPUT);

  if (!SD.begin(4)) {
    //Serial.println("initialization failed!");
    return;
  }
  //Serial.println("initialization done.  OK");

  tm1637.set();
  tm1637.init();
  Timer1.initialize(500000);//timing for 500ms
  Timer1.attachInterrupt(TimingISR);//declare the interrupt serve routine:TimingISR  
}

// This wrapper is in charge of calling 
// mus be defined like this for the lib work
void dht11_wrapper() {
  DHT11.isrCallback();
}

void loop()
{



  if(Update == ON)
  {


    //HUMIDITY AND TEMPERATURE
    //Serial.print("\nRetrieving information from sensor: ");
    //Serial.print("Read sensor: ");
    //delay(100);

    int result = DHT11.acquireAndWait();
    switch (result)
    {
    case IDDHTLIB_OK: 
      Serial.println("OK"); 
      break;
    case IDDHTLIB_ERROR_CHECKSUM: 
      Serial.println("Error\n\r\tChecksum error"); 
      break;
    case IDDHTLIB_ERROR_ISR_TIMEOUT: 
      Serial.println("Error\n\r\tISR time out error"); 
      break;
    case IDDHTLIB_ERROR_RESPONSE_TIMEOUT: 
      Serial.println("Error\n\r\tResponse time out error"); 
      break;
    case IDDHTLIB_ERROR_DATA_TIMEOUT: 
      Serial.println("Error\n\r\tData time out error"); 
      break;
    case IDDHTLIB_ERROR_ACQUIRING: 
      Serial.println("Error\n\r\tAcquiring"); 
      break;
    case IDDHTLIB_ERROR_DELTA: 
      Serial.println("Error\n\r\tDelta time to small"); 
      break;
    case IDDHTLIB_ERROR_NOTSTARTED: 
      Serial.println("Error\n\r\tNot started"); 
      break;
    default: 
      Serial.println("Unknown error"); 
      break;
    }
    /*
  // re-open the file for reading:
     myFile = SD.open("meteorologicalstation.txt");
     if (myFile) {
     Serial.println("meteorologicalstation.txt:");
     
     // read from the file until there's nothing else in it:
     while (myFile.available()) {
     	Serial.write(myFile.read());
     }
     // close the file:
     myFile.close();
     
     } else {
     	// if the file didn't open, print an error:
     Serial.println("error opening meteorologicalstation.txt");
     }   
     */

    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    myFile = SD.open("station.txt", FILE_WRITE);

    // if the file opened okay, write to it:
    if (myFile && (count%5==0)) {
      TimeUpdate();
      tm1637.display(TimeDisp);
      myFile.print("Time: ");
      myFile.print(TimeDisp[0]);
      myFile.print(TimeDisp[1]);
      myFile.print(":");      
      myFile.print(TimeDisp[2]);
      myFile.println(TimeDisp[3]);

      myFile.print("Humidity (%): ");
      myFile.println(DHT11.getHumidity(), 2);

      myFile.print("Temperature (oC): ");
      myFile.println(DHT11.getCelsius(), 2);

      myFile.print("Temperature (oF): ");
      myFile.println(DHT11.getFahrenheit(), 2);

      myFile.print("Temperature (K): ");  
      myFile.println(DHT11.getKelvin(), 2);

      myFile.print("Dew Point (oC): ");
      myFile.println(DHT11.getDewPoint());

      myFile.print("Dew Point Slow (oC): ");
      myFile.println(DHT11.getDewPointSlow());


      //MICROPHONE
      unsigned long startMillis= millis();  // Start of sample window
      unsigned int peakToPeak = 0;   // peak-to-peak level

      unsigned int signalMax = 0;
      unsigned int signalMin = 1024;

      // collect data for 50 mS
      while (millis() - startMillis < sampleWindow)
      {
        sample = analogRead(A0);
        if (sample < 1024)  // toss out spurious readings
        {
          if (sample > signalMax)
          {
            signalMax = sample;  // save just the max levels
          }
          else if (sample < signalMin)
          {
            signalMin = sample;  // save just the min levels
          }
        }
      }
      peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude
      double volts = (peakToPeak * 3.3) / 1024;  // convert to volts
      myFile.print("Microphone: ");  
      myFile.println(volts);

      //LUMINOSIY
      sensorLumValue = analogRead(sensorLumPin);  
      myFile.print("Luminosity: ");  
      myFile.println(sensorLumValue);
      myFile.println("==============================");

      //DATABASE FORMAT
      myFile.print("Database Format:");
      myFile.print("Time: ");
      myFile.print(TimeDisp[0]);
      myFile.print(TimeDisp[1]);
      myFile.print(":");      
      myFile.print(TimeDisp[2]);
      myFile.print(TimeDisp[3]);                  
      myFile.print("#");
      myFile.print(DHT11.getHumidity(), 2);
      myFile.print("#");
      myFile.print(DHT11.getCelsius(), 2);
      myFile.print("#");
      myFile.print(DHT11.getFahrenheit(), 2);
      myFile.print("#");  
      myFile.print(DHT11.getKelvin(), 2);
      myFile.print("#");
      myFile.print(DHT11.getDewPoint());
      myFile.print("#");
      myFile.print(DHT11.getDewPointSlow());
      myFile.print("#");
      myFile.print(volts);
      myFile.print("#");
      myFile.println(sensorLumValue);      
      myFile.println("============================================================");
    } 
    else {
      // if the file didn't open, print an error:
      Serial.println("error opening station.txt");
    }
    // close the file:
    myFile.close();

  }
  delay(60000);
  count++;
}
void TimingISR()
{
  halfsecond ++;
  Update = ON;
  if(halfsecond == 2){
    second ++;
    if(second == 60)
    {
      minute ++;
      if(minute == 60)
      {
        hour ++;
        if(hour == 24)hour = 0;
        minute = 0;
      }
      second = 0;
    }
    halfsecond = 0;  
  }
  // Serial.println(second);
  ClockPoint = (~ClockPoint) & 0x01;
}
void TimeUpdate(void)
{
  if(ClockPoint)tm1637.point(POINT_ON);
  else tm1637.point(POINT_OFF); 
  TimeDisp[0] = hour / 10;
  TimeDisp[1] = hour % 10;
  TimeDisp[2] = minute / 10;
  TimeDisp[3] = minute % 10;
  Update = OFF;
}







