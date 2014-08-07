/*
 * EOF preprocessor bug prevent
*/
/// @dir FHEMduino (2013-11-07)
/// FHEMduino communicator
//
// authors: mick6300
// see http://forum.fhem.de/index.php/topic,17196.0.html


#include <Wire.h>
#define DS3231_I2C_ADDRESS 104
#define BMP085_ADDRESS 0x77  // I2C address of BMP085

#define PIN_LED 13
#define PIN_SEND 11

#if defined(__AVR_ATmega32U4__) //on the leonardo and other ATmega32U4 devices interrupt 0 is on dpin 3
#define PIN_RECEIVE 3
#else
#define PIN_RECEIVE 2
#endif

#define BAUDRATE    9600

byte tMSB, tLSB;
float temp3231,temp3231d;
float tempbmp085, tempbmp085d;

String cmdstring;
volatile bool available = false;
String message = "";

static uint32_t timer;
#define DHT11_PIN 1      // ADC0  Define the ANALOG Pin connected to DHT11 Sensor
int temp1[3];                //Temp1, temp2, hum1 & hum2 are the final integer values that you are going to use in your program. 
int temp2[3];                // They update every 2 seconds.
int hum1[3];
int hum2[3];

int sensor_mq2 = A2;    
int sensor_gas = A3;
char tmp[11];

//bmp085
const unsigned char OSS = 0;  // Oversampling Setting

// Calibration values
int ac1;
int ac2;
int ac3;
unsigned int ac4;
unsigned int ac5;
unsigned int ac6;
int b1;
int b2;
int mb;
int mc;
int md;
// b5 is calculated in bmp085GetTemperature(...), this variable is also used in bmp085GetPressure(...)
// so ...Temperature(...) must be called before ...Pressure(...).
long b5; 
int geraete = 2; 
int geraete_zahl = 1;
int fehler = 0;
void setup() {
  Wire.begin();
  // put your setup code here, to run once:
  Serial.begin(BAUDRATE);
  enableReceive();
  pinMode(PIN_RECEIVE,INPUT);
  pinMode(PIN_SEND,OUTPUT);
  DDRC |= _BV(DHT11_PIN);
  PORTC |= _BV(DHT11_PIN);
  get3231Temp_start();
//  bmp085Calibration(); //nur wenn bmp085 angeschlossen
}

void loop() {
  // put your main code here, to run repeatedly:
   if (millis() > timer) {
    // update uptime. Could be every where, but still put here
    uptime(millis(), false);

    timer = millis() + 100000;
    gas_sendData();
/*     switch(geraete_zahl){
        case 1:  mq2_sendData();
                 break;
        case 2:  gas_sendData();
                 break;
        case 3:  getdht11();
                 break;
        case 4:  getbmp085();
                 break;
     }
     if (geraete_zahl == geraete) {geraete_zahl = 0;}
     geraete_zahl = geraete_zahl +1;
//     Serial.println(fehler);*/
   }
 
  if (messageAvailable()) {
    Serial.println(message);
    resetAvailable();
  }
//serialEvent does not work on ATmega32U4 devices like the Leonardo, so we do the handling ourselves
#if defined(__AVR_ATmega32U4__)
  if (Serial.available()) {
    serialEvent();
  }
#endif
}
/*
 * Interrupt System
 */

void enableReceive() {
  attachInterrupt(0,handleInterrupt,CHANGE);
}

void disableReceive() {
  detachInterrupt(0);
}

void handleInterrupt() {
  static unsigned int duration;
//  static unsigned int changeCount;
  static unsigned long lastTime;
//  static unsigned int repeatCount;
  long time = micros();
  duration = time - lastTime;

  lastTime = time;  
}

/*
 * Serial Command Handling
 */
void serialEvent()
{
  while (Serial.available())
  {

    char inChar = (char)Serial.read();
    switch(inChar)
    {
    case '\n':
    case '\r':
    case '\0':
    case '#':
      HandleCommand(cmdstring);
      break;
    default:
      cmdstring = cmdstring + inChar;
    }
  }
}

void HandleCommand(String cmd)
{
  // Version Information
  if (cmd.equals("V"))
  {
    Serial.println(F("V 1.0b1 FHEMduino - compiled at " __DATE__ " " __TIME__));
  }

  // Print free Memory
  else if (cmd.equals("R")) {
    Serial.print(F("R"));
    Serial.println(freeRam());
  }

  else if (cmd.equals("XQ")) {
    disableReceive();
    Serial.flush();
    Serial.end();
  }
  else if (cmd.equals("t")) {
    uptime(millis(), true);
  }

  // Print Available Commands
  else if (cmd.equals("?"))
  {
    Serial.println(F("? Use one of V t R q"));
  }

  cmdstring = "";
}

void uptime(unsigned long timepassed, bool Print)
{
  static unsigned long time_in_secs=0;
  static unsigned long temps=0;
  unsigned long secs=0;

  secs = timepassed/1000;
  
  if (secs < temps) {
     time_in_secs += secs;
  } else {
     time_in_secs = secs;
  }

  temps = timepassed/1000;
  
  if (Print) {
    //Display results
    String cPrint = "00000000";
    cPrint += String(time_in_secs,HEX);
    int StringStart = cPrint.length()-8;
    cPrint = cPrint.substring(StringStart);
    cPrint.toUpperCase();
    Serial.println(cPrint);
  }
}

// Get free RAM of UC
int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

/*
 * Message Handling
 */
bool messageAvailable() {
  return (available && (message.length() > 0));
}

void resetAvailable() {
  available = false;
  message = "";
}

bool mq2_sendData() {
//  int sensor_mq2 = A4;    
//  int sensor_gas = A5;    
  // Sensor ID & Channel
  byte id = 16;
  // (Propably) Battery State
  bool battery = 1;
  // Trend
  byte trend = 1;
  // Trigger
  bool forcedSend = 1;

  // Temperature & Humidity
    int mq2_value = analogRead(sensor_mq2);    
    int gas_value = analogRead(sensor_gas);
    int temperature = mq2_value;
    byte humidity = 1;
    sprintf(tmp,"G%02X%01d%01d%01d%+04d%02d", id, battery, trend, forcedSend, temperature, humidity);
//    Serial.println(tmp);
  message = tmp;
  available = true;
  return true;
}

bool gas_sendData() {
//  int sensor_mq2 = A4;    
//  int sensor_gas = A5;    
  // Sensor ID & Channel
  byte id = 11;
  // (Propably) Battery State
  bool stati = 0;

  // Temperature & Humidity
    int mq2_value = analogRead(sensor_mq2);    
    int gas_value = analogRead(sensor_gas);
    if (gas_value > 100 || mq2_value > 150) {stati = 1;}
    byte humidity = 1;
    sprintf(tmp,"G%02X%01d%+04d%+04d", id, stati, gas_value, mq2_value);
//    Serial.println(tmp);
  message = tmp;
  available = true;
  return true;
}


bool get3231Temp()
{
  //temp registers (11h-12h) get updated automatically every 64s
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0x11);
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 2);
 
  if(Wire.available()) {
    tMSB = Wire.read(); //2's complement int portion
    tLSB = Wire.read(); //fraction portion
   
    temp3231 = (tMSB & B01111111); //do 2's math on Tmsb
    temp3231 += ( (tLSB >> 6) * 0.25 ); //only care about bits 7 & 8
  }
  else {
    //oh noes, no data!
  }
//  temp3231 = (int)((temp3231 * 10) + .5); 
//  return temp3231;

temp3231d = ((temp3231d * 10) + temp3231) / 11;
byte trend = 0;

if (temp3231d > temp3231) {
  trend = 2;
}
  
if (temp3231d < temp3231) {
  trend = 1;
}  

  // Sensor ID & Channel
  byte id = 15;
  // (Propably) Battery State
  bool battery = 0;
  // Trend
//  byte trend = 0;
  // Trigger
  bool forcedSend = 1;

  // Temperature & Humidity
    int temperature = (int)((temp3231 * 10) + .5);
    byte humidity = 1;
    sprintf(tmp,"K%02X%01d%01d%01d%+04d%02d", id, battery, trend, forcedSend, temperature, humidity);
//    Serial.println(tmp);
  message = tmp;
  available = true;
  return true;
}

//bool receiveProtocolKW9010(unsigned int changeCount) {
bool getbmp085 () 
//int getbmp085()
{
tempbmp085d = ((tempbmp085d * 10) + tempbmp085) / 11;
byte trend = 0;

if (tempbmp085d > tempbmp085) {
  trend = 2;
}
  
if (tempbmp085d < tempbmp085) {
  trend = 1;
}  

  // Sensor ID & Channel
  byte id = 20;
  // (Propably) Battery State
  bool battery = 0;
  // Trend
//  byte trend = 0;
  // Trigger
  bool forcedSend = 1;

  // Temperature & Humidity
//    int temperature = (int)((temp3231 * 10) + .5);
    int temperature = int ((bmp085GetTemperature(bmp085ReadUT()) * 10)+ .5); //MUST be called first
    float pressure = bmp085GetPressure(bmp085ReadUP());

    byte humidity = 1;
    sprintf(tmp,"K%02X%01d%01d%01d%+04d%02d", id, battery, trend, forcedSend, temperature, humidity);
//    Serial.println(tmp);
  message = tmp;
  available = true;
  return true;
}

int get3231Temp_start()
{
  //temp registers (11h-12h) get updated automatically every 64s
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0x11);
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 2);
 
  if(Wire.available()) {
    tMSB = Wire.read(); //2's complement int portion
    tLSB = Wire.read(); //fraction portion
   
    temp3231 = (tMSB & B01111111); //do 2's math on Tmsb
    temp3231 += ( (tLSB >> 6) * 0.25 ); //only care about bits 7 & 8
  }
  else {
    //oh noes, no data!
  }
//  temp3231 = (int)((temp3231 * 10) + .5); 
//  return temp3231;
temp3231d = temp3231;
}

// Stores all of the bmp085's calibration values into global variables
// Calibration values are required to calculate temp and pressure
// This function should be called at the beginning of the program
void bmp085Calibration()
{
  ac1 = bmp085ReadInt(0xAA);
  ac2 = bmp085ReadInt(0xAC);
  ac3 = bmp085ReadInt(0xAE);
  ac4 = bmp085ReadInt(0xB0);
  ac5 = bmp085ReadInt(0xB2);
  ac6 = bmp085ReadInt(0xB4);
  b1 = bmp085ReadInt(0xB6);
  b2 = bmp085ReadInt(0xB8);
  mb = bmp085ReadInt(0xBA);
  mc = bmp085ReadInt(0xBC);
  md = bmp085ReadInt(0xBE);
}

// Calculate temperature in deg C
float bmp085GetTemperature(unsigned int ut){
  long x1, x2;

  x1 = (((long)ut - (long)ac6)*(long)ac5) >> 15;
  x2 = ((long)mc << 11)/(x1 + md);
  b5 = x1 + x2;

  float temp = ((b5 + 8)>>4);
  temp = temp /10;

  return temp;
}

// Calculate pressure given up
// calibration values must be known
// b5 is also required so bmp085GetTemperature(...) must be called first.
// Value returned will be pressure in units of Pa.
long bmp085GetPressure(unsigned long up){
  long x1, x2, x3, b3, b6, p;
  unsigned long b4, b7;

  b6 = b5 - 4000;
  // Calculate B3
  x1 = (b2 * (b6 * b6)>>12)>>11;
  x2 = (ac2 * b6)>>11;
  x3 = x1 + x2;
  b3 = (((((long)ac1)*4 + x3)<<OSS) + 2)>>2;

  // Calculate B4
  x1 = (ac3 * b6)>>13;
  x2 = (b1 * ((b6 * b6)>>12))>>16;
  x3 = ((x1 + x2) + 2)>>2;
  b4 = (ac4 * (unsigned long)(x3 + 32768))>>15;

  b7 = ((unsigned long)(up - b3) * (50000>>OSS));
  if (b7 < 0x80000000)
    p = (b7<<1)/b4;
  else
    p = (b7/b4)<<1;

  x1 = (p>>8) * (p>>8);
  x1 = (x1 * 3038)>>16;
  x2 = (-7357 * p)>>16;
  p += (x1 + x2 + 3791)>>4;

  long temp = p;
  return temp;
}

// Read 1 byte from the BMP085 at 'address'
char bmp085Read(unsigned char address)
{
  unsigned char data;

  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.write(address);
  Wire.endTransmission();

  Wire.requestFrom(BMP085_ADDRESS, 1);
  while(!Wire.available())
    ;

  return Wire.read();
}

// Read 2 bytes from the BMP085
// First byte will be from 'address'
// Second byte will be from 'address'+1
int bmp085ReadInt(unsigned char address)
{
  unsigned char msb, lsb;

  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.write(address);
  Wire.endTransmission();

  Wire.requestFrom(BMP085_ADDRESS, 2);
  while(Wire.available()<2)
    ;
  msb = Wire.read();
  lsb = Wire.read();

  return (int) msb<<8 | lsb;
}

// Read the uncompensated temperature value
unsigned int bmp085ReadUT(){
  unsigned int ut;

  // Write 0x2E into Register 0xF4
  // This requests a temperature reading
  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.write(0xF4);
  Wire.write(0x2E);
  Wire.endTransmission();

  // Wait at least 4.5ms
  delay(5);

  // Read two bytes from registers 0xF6 and 0xF7
  ut = bmp085ReadInt(0xF6);
  return ut;
}

// Read the uncompensated pressure value
unsigned long bmp085ReadUP(){

  unsigned char msb, lsb, xlsb;
  unsigned long up = 0;

  // Write 0x34+(OSS<<6) into register 0xF4
  // Request a pressure reading w/ oversampling setting
  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.write(0xF4);
  Wire.write(0x34 + (OSS<<6));
  Wire.endTransmission();

  // Wait for conversion, delay time dependent on OSS
  delay(2 + (3<<OSS));

  // Read register 0xF6 (MSB), 0xF7 (LSB), and 0xF8 (XLSB)
  msb = bmp085Read(0xF6);
  lsb = bmp085Read(0xF7);
  xlsb = bmp085Read(0xF8);

  up = (((unsigned long) msb << 16) | ((unsigned long) lsb << 8) | (unsigned long) xlsb) >> (8-OSS);

  return up;
}

void writeRegister(int deviceAddress, byte address, byte val) {
  Wire.beginTransmission(deviceAddress); // start transmission to device 
  Wire.write(address);       // send register address
  Wire.write(val);         // send value to write
  Wire.endTransmission();     // end transmission
}

int readRegister(int deviceAddress, byte address){

  int v;
  Wire.beginTransmission(deviceAddress);
  Wire.write(address); // register to read
  Wire.endTransmission();

  Wire.requestFrom(deviceAddress, 1); // read a byte

  while(!Wire.available()) {
    // waiting
  }

  v = Wire.read();
  return v;
}

float calcAltitude(float pressure){

  float A = pressure/101325;
  float B = 1/5.25588;
  float C = pow(A,B);
  C = 1 - C;
  C = C /0.0000225577;

  return C;
}

bool getdht11()
{
    byte dht11_dat[5];
    byte dht11_in;
    byte i;
    // start condition
    // 1. pull-down i/o pin from 18ms
    PORTC &= ~_BV(DHT11_PIN);
    delay(18);
    PORTC |= _BV(DHT11_PIN);
    delayMicroseconds(40);

    DDRC &= ~_BV(DHT11_PIN);
    delayMicroseconds(40);

    dht11_in = PINC & _BV(DHT11_PIN);

    if(dht11_in){
      Serial.println("dht11 start condition 1 not met");
      fehler = fehler +1;
      return false;
    }
    delayMicroseconds(80);

    dht11_in = PINC & _BV(DHT11_PIN);

    if(!dht11_in){
      Serial.println("dht11 start condition 2 not met");
      fehler = fehler +1;
      return false;
    }
    delayMicroseconds(80);
    // now ready for data reception
    for (i=0; i<5; i++)
      dht11_dat[i] = read_dht11_dat();

    DDRC |= _BV(DHT11_PIN);
    PORTC |= _BV(DHT11_PIN);

    byte dht11_check_sum = dht11_dat[0]+dht11_dat[1]+dht11_dat[2]+dht11_dat[3];
    // check check_sum
    if(dht11_dat[4]!= dht11_check_sum)
    {
//      Serial.println("DHT11 checksum error");
      return false;
    }
    temp1[0]=dht11_dat[2];
    temp2[0]=dht11_dat[3];
    hum1[0]=dht11_dat[0];
    hum2[0]=dht11_dat[1];
/*    Serial.print("Temperature: ");
    Serial.print(temp1[0]);
    Serial.print(".");
    Serial.print(temp2[0]);
    Serial.print(" C");
    Serial.print("    ");
    Serial.print("Humidity: ");
    Serial.print(hum1[0]);
    Serial.print(".");
    Serial.print(hum2[0]);
    Serial.println("%");
*/
  // Sensor ID & Channel
  byte id = 14;
  // (Propably) Battery State
  bool battery = 0;
  // Trend
//  byte trend = 0;
  // Trigger
  bool forcedSend = 1;
byte trend = 0;

  // Temperature & Humidity
//    int temperature = (int)((temp3231 * 10) + .5);
    int temperature = (int) temp1[0]*10; 
    byte humidity = hum1[0];
    sprintf(tmp,"K%02X%01d%01d%01d%+04d%02d", id, battery, trend, forcedSend, temperature, humidity);
//    Serial.println(tmp);
  message = tmp;
  available = true;
  return true;
}

byte read_dht11_dat()
{
  byte i = 0;
  byte result=0;
  for(i=0; i< 8; i++){


    while(!(PINC & _BV(DHT11_PIN)));  // wait for 50us
    delayMicroseconds(30);

    if(PINC & _BV(DHT11_PIN)) 
      result |=(1<<(7-i));
    while((PINC & _BV(DHT11_PIN)));  // wait '1' finish


  }
  return result;
}
