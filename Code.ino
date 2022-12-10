#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
#include <Wire.h>
// pulse sensor
int PulseSensorPurplePin = 36;
int Signal;                // holds the incoming raw data. Signal value can range from 0-1024
int Threshold = 2000;            // Determine which Signal to "count as a beat", and which to ingore.

//Temp Sensor 
#include "esp_adc_cal.h"
#define LM35_Sensor1 35
int LM35_Raw_Sensor1 = 0;
float LM35_TempC_Sensor1 = 0.0;
float LM35_TempF_Sensor1 = 0.0;
float Voltage = 0.0;

//Accelerometer
const int xpin = 32;                  // x-axis of the accelerometer
const int ypin = 33;                  // y-axis
const int zpin = 34;                  // z-axis (only on 3-axis models)


// gps module
#include <TinyGPSPlus.h>
TinyGPSPlus gps;
#define RXD2 16
#define TXD2 17
HardwareSerial neogps(1);

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "OnePlus Nord"
#define WIFI_PASSWORD "sainanda321"

// Insert Firebase project API Key
#define API_KEY "AIzaSyAYg_OduC4oNaLHg7MnNq51qicKuDot6Wk"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://oldiegoldie1-default-rtdb.firebaseio.com/" 

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

void setup(){
  Serial.begin(9600);
    //Begin serial communication Neo6mGPS
  neogps.begin(9600, SERIAL_8N1, RXD2, TXD2);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print("*");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop(){

  // Read LM35_Sensor1 ADC Pin
  LM35_Raw_Sensor1 = analogRead(LM35_Sensor1);  
  // Calibrate ADC & Get Voltage (in mV)
  Voltage = readADC_Cal(LM35_Raw_Sensor1);
  // TempC = Voltage(mV) / 10
  LM35_TempC_Sensor1 = ((Voltage / 10)+7);
  // Print The Readings
  Serial.print("Temperature = ");
  Serial.print(LM35_TempC_Sensor1);
  Serial.print(" °C , ");
  //read pulse sensor
  Signal = analogRead(PulseSensorPurplePin);
  Serial.print(Signal);
  
  //gps 
  boolean newData = false;
  for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (neogps.available())
    {
      if (gps.encode(neogps.read()))
      {
        newData = true;
      }
    }
  }

  //If newData is true
  if(newData == true)
  {
    newData = false;
    Serial.println(gps.satellites.value());
    print_loc();
  }
  else
  {
   Serial.println(" GPS connection not found (loop)");  
  }

  //Accelerometer
  int x = analogRead(xpin);
  int y = analogRead(ypin);
  int z = analogRead(zpin);
  Serial.print(x-322);
  Serial.print("\t");
  Serial.print(y-320);
  Serial.print("\t");
  Serial.print(z-263);
  Serial.print("\n"); 
  
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    // Write an Int number on the database path test/int
    if (Firebase.RTDB.setInt(&fbdo, "test/pulse", Signal)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    //count++;
    
    // Write an Float number on the database path test/float
    if (Firebase.RTDB.setFloat(&fbdo, "test/temp", LM35_TempC_Sensor1)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    // Pushing fall X
    if (Firebase.RTDB.setInt(&fbdo, "fall/x",x-322 )){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    // Pushing fall Y
    if (Firebase.RTDB.setInt(&fbdo, "fall/y",y-320)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    // Pushing fall X
    if (Firebase.RTDB.setInt(&fbdo, "fall/z",z-263 )){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
    delay(3000);
}


uint32_t readADC_Cal(int ADC_Raw)
{
  esp_adc_cal_characteristics_t adc_chars;
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  return(esp_adc_cal_raw_to_voltage(ADC_Raw, &adc_chars));
}
void print_loc()
{      
  if (gps.location.isValid() == 1)
  {
   //String gps_speed = String(gps.speed.kmph());;;
    Serial.println("Lat: ");
    Serial.print(gps.location.lat(),6);
    Serial.println("Lng: ");
    Serial.print(gps.location.lng(),6); 
  }
  else
  {
    Serial.print("no data");
  }  
}
