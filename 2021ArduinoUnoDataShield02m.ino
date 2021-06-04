//Arduino Uno with data logger shield with DS1307 RTC, SD card.  
//Analog sensors on pins A0-A5.  Also checks D3-D9.
//SD Card --> 8.3 filenames (not case-sensitive, ABCDEFGH.txt = abcdefgh.txt).
//  CS = pin 10 for Uno.  4 for MKRZero
//  MOSI = pin 11
//  MISO = pin 12
//  CLK = pin 13
//RTC DS1307. I2C--> SCL(clk)=D1, SDA(data)=D2 
//20210604 - by TSBrownie.  Non-commercial use approved.
#include <SD.h>                                   //SD card lib
#include <SPI.h>                                  //Serial lib
#include "Wire.h"                                 //I2C library
#define DS1307 0x68                               //I2C Addr 1307 (Default=0x68)
File diskFile;                                    //SD card file handle
String DoWList[]={"Null",",Sun,",",Mon,",",Tue,",",Wed,",",Thr,",",Fri,",",Sat,"}; //DOW from 1-7
byte second, minute, hour, DoW, Date, month, year; //Btye variables for BCD time
const int CSpin = 10;                             //Chip select pin (varies with model)
const int numSens = 5;                            //Number of analog sensors
const int readInterval = 5000;                    //**Millisecond delay between readings
int inputPin = 0;                                 //Input pins.  A0-A5, D3-D9.
int dataIn = 0;                                   //Data from sensors, temp
String dataBuff = "";                             //Output SD file buffer
String timeString;                                //Build date time data
String SDFileName = "SDfil01.txt";                //**SD card file name to create/write/read

//RTC FUNCTIONS =====================================
byte BCD2DEC(byte val){             //Ex: 51 = 01010001 BCD.  01010001/16-->0101=5 then x10-->50.  
  return(((val/16)*10)+(val%16));}  //         01010001%16-->0001.  50+0001 = 51 DEC

void GetRTCTime(){                               //Routine read real time clock, format data
  byte second;byte minute;byte hour;byte DoW;byte Date;byte month;byte year;
  Wire.beginTransmission(DS1307);                //Open I2C to RTC DS1307
  Wire.write(0x00);                              //Write reg pointer to 0x00 Hex
  Wire.endTransmission();                        //End xmit to I2C.  Send requested data.
  Wire.requestFrom(DS1307, 7);                   //Get 7 bytes from RTC buffer
  second = BCD2DEC(Wire.read() & 0x7f);          //Seconds.  Remove hi order bit
  minute = BCD2DEC(Wire.read());                 //Minutes
  hour = BCD2DEC(Wire.read() & 0x3f);            //Hour.  Remove 2 hi order bits
  DoW = BCD2DEC(Wire.read());                    //Day of week
  Date = BCD2DEC(Wire.read());                   //Date
  month = BCD2DEC(Wire.read());                  //Month
  year = BCD2DEC(Wire.read());                   //Year
  timeString = 2000+year;                        //Build Date-Time data to write to SD
  if (month<10){timeString = timeString + '0';}  //Pad leading 0 if needed
  timeString = timeString + month;               //Month (1-12)  
  if(Date<10){timeString = timeString + '0';}    //Pad leading 0 if needed
  timeString = timeString + Date;                //Date (1-30)
//  timeString = timeString + DoWList[DoW];        //1Sun-7Sat (0=null)
  if (hour<10){timeString = timeString + '0';}   //Pad leading 0 if needed
  timeString = timeString + hour;                //HH (0-24)
  if (minute<10){timeString = timeString + '0';} //Pad leading 0 if needed
  timeString = timeString + minute;              //MM (0-60)
  if (second<10){timeString = timeString + '0';} //Pad leading 0 if needed
  timeString = timeString + second;              //SS (0-60)
}
//SD CARD FUNCTIONS =================================
void openSD() {                                  //Routine to open SD card
  if (Serial){Serial.println(); Serial.println("Opening SD card");}//User message
  if (!SD.begin()) {                             //If not open, print message.  (CS=pin15)
    if (Serial){Serial.println("Open SD failed");}
    return;}                                     //If open fails
  if (Serial){Serial.println("SD Card open");}   //Else card opened
}

char openFile(char RW) {                     //Open SD file.  Only 1 open at a time
  diskFile.close();                          //Ensure file status, before re-opening
  diskFile = SD.open(SDFileName, RW);}       //If Read open at file start.  If write/append open at EOF

void print2File() {                          //Print data to SD file
  openFile(FILE_WRITE);                      //Open SD file for write
  if (diskFile) {                            //If file & opened --> write
    diskFile.println(dataBuff);              //Print string to SD file
    diskFile.close();                        //Close file, flush buffer (reliable but slower)
    if (Serial){Serial.println(dataBuff);}   //Print to COM
  } else {Serial.println("Error opening file");}  //File didn't open
}

void getRecordFile() {                       //Read from SD file
  openFile(FILE_READ);                       //Open SD file for write
  if (diskFile) {                            //Check if file open
    Serial.write(diskFile.read());           //Read SD file, then write to COM
  } else {Serial.println("Error opening file for read");}    //File didn't open
}

// SETUP AND LOOP =============================
void setup() {
  Wire.begin();                              //Open I2C
  Serial.begin(115200);                      //Open COM
  delay(300);                                //Wait for COM, but don't stop
  openSD();                                  //Open SD card
  GetRTCTime();                              //RTC "wake up"
  for (inputPin = 3; inputPin <=9; inputPin++){ //Init digital pins 3-9
    pinMode(inputPin,INPUT);}                //Set pinMode to input
}

void loop() {
  dataBuff = timeString + ',';
  for (inputPin = 0; inputPin < numSens; inputPin++) {  //Read sensors loop
    GetRTCTime();                                //Get time from real time clock
    dataIn = analogRead(inputPin);               //Read pin, temp store data
    dataBuff += String(dataIn);                  //Add data to string
//    if (inputPin < (numSens-1)){dataBuff += ",";} //Data divider if only analog
    dataBuff += ",";                             //Data divider if both dig/ana
  }
  for (inputPin = 3; inputPin <= 9; inputPin++){ //Read sensors loop
    dataIn = digitalRead(inputPin);              //Read pin, temp store data
    dataBuff += String(dataIn);                  //Add data to string
    if (inputPin < 9){dataBuff += ",";}          //Data divider
  }
  print2File();                                  //Write to SD card file
  dataBuff = "";                                 //Clear buffer for next round
  delay(readInterval);                           //Interval between measurements
}
