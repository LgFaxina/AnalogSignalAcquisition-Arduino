#include "BluetoothSerial.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include "string.h"

// BT based of BT examples library
// packaging based of multiprocessing code of Daniel C. and Thiago D.

/*definition of BT, and error prevention*/
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif

BluetoothSerial SerialBT; //define BT funcions header
Adafruit_MPU6050 mpu; //define MPU-6050 Sensor
/*---------------------------------------------*/

#define signal 15      //define the input pin for esp32
#define bufferSize 96  //define the amount of data stored inside de buffer

#define LED_BUILTIN 2
/*Global declaration for blinking led withoud delay*/
int ledState = LOW;             // ledState used to set the LED
unsigned long LEDpreviouMicros = 0;        // will store last time LED was updated
const long LEDinterval = 1000000;         //set the interval for blinking
/*---------------------------------------------*/

/*Global declaration for setting transfer speed*/
unsigned long BUFFERpreviousMicros = 0;       
const long BUFFERinterval = 1000;       //sets time interval (in microsec) between each data read
/*---------------------------------------------*/

int iteracoes = 0;  //counter for mesuring loop cicles in 1 second 

/*Global declaration for packaging data   (NEEDS REWORK)*/
float buffer [bufferSize]; //create buffer
const size_t CAPACITY = JSON_ARRAY_SIZE(bufferSize);
int bufferIndex = 0; //index for counting buffer readyness
/*---------------------------------------------*/

//float timestampSimu = 1680058002;
const char* name_device = "left"; // "left" or "right"

String DataPrep(){
  StaticJsonDocument<CAPACITY> doc; // allocate the memory for the document

  //JsonArray timestamp = doc.createNestedArray("timestamp");
  JsonArray data_Ac_X = doc.createNestedArray("left_data_Ac_X");
  JsonArray data_Ac_Y = doc.createNestedArray("left_data_Ac_Y");
  JsonArray data_Ac_Z = doc.createNestedArray("left_data_Ac_Z");
  JsonArray data_Gy_X = doc.createNestedArray("left_data_Gy_X");
  JsonArray data_Gy_Y = doc.createNestedArray("left_data_Gy_Y");
  JsonArray data_Gy_Z = doc.createNestedArray("left_data_Gy_Z");

  /*JsonArray data_Ac_X = doc.createNestedArray("right_data_Ac_X");
  JsonArray data_Ac_Y = doc.createNestedArray("right_data_Ac_Y");
  JsonArray data_Ac_Z = doc.createNestedArray("right_data_Ac_Z");
  JsonArray data_Gy_X = doc.createNestedArray("right_data_Gy_X");
  JsonArray data_Gy_Y = doc.createNestedArray("right_data_Gy_Y");
  JsonArray data_Gy_Z = doc.createNestedArray("right_data_Gy_Z");*/

  for (int i = 0; i < bufferSize; i=i+6){
      //timestamp.add(buffer[i]);
      data_Ac_X.add(buffer[i]);
      data_Ac_Y.add(buffer[i+1]);
      data_Ac_Z.add(buffer[i+2]);
      data_Gy_X.add(buffer[i+3]);
      data_Gy_Y.add(buffer[i+4]);
      data_Gy_Z.add(buffer[i+5]);
  }
  // serialize the array and sed the result to Serial
  String json;
  serializeJson(doc, json);
  //Serial.print("#debug - DataPrep ");Serial.println(json); 
  return json;
}

bool bufferBuild(float valueRead1,float valueRead2,float valueRead3,float valueRead4,float valueRead5,float valueRead6, unsigned long currentMicros){
  if(currentMicros - BUFFERpreviousMicros >= BUFFERinterval){
    //Serial.print("#debug - (currentMicros - BUFFERpreviousMicros ");Serial.println(currentMicros - BUFFERpreviousMicros);      //*debug*
    BUFFERpreviousMicros = currentMicros;  

    //buffer[bufferIndex] = timestampRead;
    buffer[bufferIndex] = valueRead1;
    buffer[bufferIndex+1] = valueRead2;
    buffer[bufferIndex+2] = valueRead3;
    buffer[bufferIndex+3] = valueRead4;
    buffer[bufferIndex+4] = valueRead5;
    buffer[bufferIndex+5] = valueRead6;
    /*Serial.print(" - "); Serial.print(bufferIndex); Serial.print(" - "); Serial.print(timestampRead);
    Serial.print(" - "); Serial.print(bufferIndex+1); Serial.print(" - "); Serial.print(valueRead1);
    Serial.print(" - "); Serial.print(bufferIndex+2); Serial.print(" - "); Serial.print(valueRead2);
    Serial.print(" - "); Serial.print(bufferIndex+3); Serial.print(" - "); Serial.print(valueRead3);
    Serial.print(" - "); Serial.print(bufferIndex+4); Serial.print(" - "); Serial.print(valueRead4);
    Serial.print(" - "); Serial.print(bufferIndex+5); Serial.print(" - "); Serial.print(valueRead5);
    Serial.print(" - "); Serial.print(bufferIndex+6); Serial.print(" - "); Serial.print(valueRead6);
         *///*debug*
    bufferIndex = bufferIndex + 6;

    if (bufferIndex >= bufferSize){
      //Serial.print("#debug - bufferIndex ");Serial.println(bufferIndex);    //*debug*
      bufferIndex = 0;
      return 1;
    }
  }
  return 0;
}


void setup() {
  pinMode(LED_BUILTIN, OUTPUT); //setup builtin led
  pinMode(signal, OUTPUT); //setup input pin

  Serial.begin(115200);
  if(name_device == "left"){
    SerialBT.begin("ESP32_BT_LEFT");
  } else{
    SerialBT.begin("ESP32_BT_RIGHT");
  }  
  Serial.println("The device started, now you can pair it with bluetooth!");

  // Try to initialize MPU6050
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.print("Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
  case MPU6050_RANGE_2_G:
    Serial.println("+-2G");
    break;
  case MPU6050_RANGE_4_G:
    Serial.println("+-4G");
    break;
  case MPU6050_RANGE_8_G:
    Serial.println("+-8G");
    break;
  case MPU6050_RANGE_16_G:
    Serial.println("+-16G");
    break;
  }
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.print("Gyro range set to: ");
  switch (mpu.getGyroRange()) {
  case MPU6050_RANGE_250_DEG:
    Serial.println("+- 250 deg/s");
    break;
  case MPU6050_RANGE_500_DEG:
    Serial.println("+- 500 deg/s");
    break;
  case MPU6050_RANGE_1000_DEG:
    Serial.println("+- 1000 deg/s");
    break;
  case MPU6050_RANGE_2000_DEG:
    Serial.println("+- 2000 deg/s");
    break;
  }

  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
  Serial.print("Filter bandwidth set to: ");
  switch (mpu.getFilterBandwidth()) {
  case MPU6050_BAND_260_HZ:
    Serial.println("260 Hz");
    break;
  case MPU6050_BAND_184_HZ:
    Serial.println("184 Hz");
    break;
  case MPU6050_BAND_94_HZ:
    Serial.println("94 Hz");
    break;
  case MPU6050_BAND_44_HZ:
    Serial.println("44 Hz");
    break;
  case MPU6050_BAND_21_HZ:
    Serial.println("21 Hz");
    break;
  case MPU6050_BAND_10_HZ:
    Serial.println("10 Hz");
    break;
  case MPU6050_BAND_5_HZ:
    Serial.println("5 Hz");
    break;
  }

  Serial.println("");
  delay(100);
}

void loop() {
  if (Serial.available()) {
    SerialBT.write(Serial.read());
  }
  if (SerialBT.available()) {
    Serial.write(SerialBT.read());
  }
  
  /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  unsigned long currentMicros = micros(); //using microsec precision reduces the code velocity to ~=10380 cicles per sec

  //this if section is only for blinking the onboard LED
  if (currentMicros - LEDpreviouMicros >= LEDinterval) {
    //Serial.print(iteracoes);  Serial.print(" --- "); Serial.println(currentMicros - LEDpreviouMicros); //*debug*    //print elapsed loop time in milliseconds and the count of iterations

    LEDpreviouMicros = currentMicros;        //saves the last time you blinked the LED
    iteracoes = 0;

    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }
    // set the LED with the ledState of the variable:
    digitalWrite(LED_BUILTIN, ledState);
  }

  bool bufferFlag = 0;
  //strcpy(name_device_sensor,name_device);
  //strcat(name_device_sensor, name_sensor);
  
  bufferFlag = bufferBuild(a.acceleration.x, a.acceleration.y, a.acceleration.z, g.gyro.x, g.gyro.y, g.gyro.z, currentMicros);   //call the buffer builder function every loop cicle, now using raw analog input and microsec precision
  //Serial.print("------------"); Serial.println(bufferFlag);     //*debug*
  
  if(bufferFlag == 1){
  SerialBT.println(DataPrep());           //print data to bluetooth serial
  Serial.print("DATA- ");Serial.println(DataPrep());         //*debug*
  }
  
  iteracoes++;         //increases the loop cicles counter
    
}