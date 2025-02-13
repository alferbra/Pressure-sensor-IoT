#include <Arduino.h>
#include <Adafruit_ADS1X15.h>
#include "passwords.h"  //include private data

#define uS_TO_H_FACTOR 3600000000
#define TIME_TO_SLEEP  4           //Hours of deep sleep mode

#define SIM800L_TX 26               
#define SIM800L_RX 27
#define SIM800L_POWER 23

Adafruit_ADS1115 ads; 

//Variables to send request to thingspeak
const String request = "GET https://api.thingspeak.com/update?api_key=" + api_key + "&field1=";
const String con_type = "TCP";
const String url = "api.thingspeak.com";
const int port = 80;

//Messages to send via SMS
const String db_link = "";                                          //Data base link to send via sms
const String pressure_message = "Pressure: ";
const String link_message = " bar. Lastest data: " + db_link;
const String phone_number[] = {};                                     //Phone numbers you want to send SMS

//Variables to get the sensor value
float bar_value;
int16_t adc0;
float volts0;


/*************************************************************************************
* READ RESPONSE FROM SIM800L
**************************************************************************************/
void serial_data(){
  while(Serial2.available() != 0){
    Serial.write(Serial2.read());
  }
}


/************************************************************************************
 * GET SENSOR VALUE
*************************************************************************************/
void read_sensor(){
  adc0 = ads.readADC_SingleEnded(0);
  volts0 = ads.computeVolts(adc0);

  bar_value = 3*(volts0-0.5);    //Conversion to bar
}


/***********************************************************************************
 * UPLOAD DATA
 ***********************************************************************************/
void data_upload(){
  
  Serial2.println("AT+CIPSHUT");                                        //Close any connection
  delay(2000);
  serial_data();
  
  Serial2.println("AT+CSTT=" + APN + "," + USER + "," + PASSWORD);      //APN configuration
  delay(3000);
  serial_data();
  
  Serial2.println("AT+CIICR");                                          //GPRS connection
  delay(3000);
  serial_data();

  Serial2.println("AT+CIFSR");                                          //Get local IP
  delay(2000);
  serial_data();

  Serial2.println("AT+CIPSTART=" + con_type + "," + url + "," + port);  //Connection type, URL and port
  delay(6000);
  serial_data();

  Serial2.println("AT+CIPSEND");                                        //Start the upload
  delay(4000);
  serial_data();

  Serial2.println(request + bar_value);                                 //GET or POST request
  delay(5000);

  Serial2.println((char)26);                                            //Send data (bar) - 0x1A
  delay(5000);
  serial_data();

  Serial2.println("AT+CIPSHUT");                                        //Close connection
  delay(2000);
  serial_data();
}


/******************************************************************************************
* SEND SMS
******************************************************************************************/
void send_sms(String message){

  Serial2.println("AT+CIPSHUT");                                            //Close any connection
  delay(1000);
  //serial_data();

  Serial2.println("AT+CMGF=1");                                            //SMS mode
  delay(200);
  //serial_data();

  for (int i=0; i<(sizeof(phone_number)/sizeof(phone_number[0])); i++){   //Send SMS to specified phone numbers
    Serial2.print("AT+CMGS=\"+34" + phone_number[i] + "\"\r");  
    delay(200);
    //serial_data();

    Serial2.println(message);
    delay(500);
    //serial_data();

    Serial2.println((char)26);
    delay(5000);
    //serial_data();
  } 
}


void setup(){
	Serial.begin(115200);                                           //Serial communication with ESP32
  delay(500);

  pinMode(SIM800L_POWER, OUTPUT);
  digitalWrite(SIM800L_POWER, HIGH);                              //Power on SIM800L
  delay(10000);

  Serial2.begin(9600, SERIAL_8N1, SIM800L_TX, SIM800L_RX);        //Serial communication with SIM800L
  delay(500);

  //                                                                ADS1015  ADS1115
  //                                                                -------  -------
  ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
  // ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
  // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
  // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
  // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV

  if (!ads.begin()) {
    Serial.write("Failed to initialize ADS.\n");
    return;
  }

  read_sensor();

  data_upload();

  //  send_sms(pressure_message + bar_value + link_message);
  

  digitalWrite(SIM800L_POWER, LOW);                               //Power off SIM800L

	esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_H_FACTOR);  //Set sleep timer

  delay(500);

	
	esp_deep_sleep_start();                                         //Go to sleep now
}

void loop(){}