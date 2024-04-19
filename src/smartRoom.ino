/*

                                        Smart Room with Xolcano-ESP8266 dev board
                Disclaimer: This code is for hobbyists for learning purposes. Not recommended for production use!!

                            # Dashboard Setup
                             - create account and login to the dashboard
                             - Create project.
                             - Create a node (e.g., for home- Room1 or study room).
                             - Create variables: temperature and humidity.
                            Note: Variable Identifier is essential; fill it accurately.

                            # Hardware Setup
                              - Connect LDR at A0 with voltage divider (circuit diogram linked in the readme).
                              

                  Note: The code is tested on the NodeMCU 1.0 board (ESP12E-Module)

                                                                                           Dated: 12-April-2024

*/
#include <Arduino.h>
#include <ESP8266WiFi.h>       // Ensure to include the ESP8266Wifi.h library, not the common library WiFi.
#include <PubSubClient.h>      //Include PubSubClient library to handle mqtt
#include <WiFiClientSecure.h>  // include WiFiClientSecure to establish secure connect .. anedya only allow secure connection
#include <ArduinoJson.h>       // Include the Arduino library to make json or abstract the value from the json
#include <TimeLib.h>           // Include the Time library to handle time synchronization with ATS (Anedya Time Services)

//-----------------------------------Variable section----------------------------------------------------------------------------------
//-------------------------------Anedya Setup------------------------------------------------------------------
String regionCode = "ap-in-1";                                   // Anedya region code (e.g., "ap-in-1" for Asia-Pacific/India) | For other country code, visity [https://docs.anedya.io/device/intro/#region]
const char *deviceID = "<PHYSICAL-DEVICE-UUID>";   // Fill your device Id , that you can get from your node description
const char *connectionkey = "<CONNECTION_KEY>";  // Fill your connection key, that you can get from your node description
//-------------------------------Wifi Setup------------------------------------------------------------------
const char *ssid = "<SSID>";  // Replace with your WiFi name
const char *pass = "<PASSWORD";  // Replace with your WiFi password
//-------------------------------Pins allocation---------------------------------------------------------------
#define fanPin 14
#define bulbPin 16
#define ldrPin A0
#define buzzerPin 12
//---------------------------------Mqtt-variables----------------------------------------------------------------
// MQTT connection settings
const char *mqtt_broker = "device.ap-in-1.anedya.io";                       // MQTT broker address
const char *mqtt_username = deviceID;                                       // MQTT username
const char *mqtt_password = connectionkey;                                  // MQTT password
const int mqtt_port = 8883;                                                 // MQTT port
String responseTopic = "$anedya/device/" + String(deviceID) + "/response";  // MQTT topic for device responses
String errorTopic = "$anedya/device/" + String(deviceID) + "/errors";       // MQTT topic for device errors
String commandTopic = "$anedya/device/" + String(deviceID) + "/commands";   // MQTT topic for device commands
String statusTopic = "$anedya/device/" + String(deviceID) + "/commands/updateStatus/json";  // MQTT topic update status of the command

//------------------------------------Helper variables-----------------------------------------------------------
String timeRes,submitRes,commandId, fan_commandId, bulb_commandId;         
long long submitLogTimer,submitDataTimer,submitLdrTimer;                                    // timer's variable to control flow
String fanStatus = "on";   // variable to store the status of the fan
String bulbStatus = "off";  // variable to store the status of the bulb
//--ldr-----------------------------------------------------
const int numReadings = 10; // Number of readings to average
int ldrReadings[numReadings]; // Array to store the readings
int ldrIndex = 0, intialTen=0; // Index for the next reading
int ldrTotal = 0; // Total of all readings
//------------------------------------------Function Decalaration section----------------------------------------
void connectToMQTT();                                                // function to connect with the anedya broker
void mqttCallback(char *topic, byte *payload, unsigned int length);  // function to handle call back
void setDevice_time();                                              // Function to configure the device time with real-time from ATS (Anedya Time Services)
void beep(unsigned int DELAY);                                          

//------------------------------------------- client initialization------------------------------------------------
WiFiClientSecure esp_client;
PubSubClient mqtt_client(esp_client);
//-------------------------------------------------Setup-----------------------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);  // Initialize serial communication with  your device compatible baud rate
  delay(1500);           // Delay for 1.5 seconds

  // Connect to WiFi network
    WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  Serial.println();
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  submitLogTimer=millis();  // assigning value to handle interval
  submitDataTimer = millis();
  submitLdrTimer=millis();

  esp_client.setInsecure();
  mqtt_client.setServer(mqtt_broker, mqtt_port);  // Set the MQTT server address and port for the MQTT client to connect to anedya broker
  mqtt_client.setKeepAlive(60);                   // Set the keep alive interval (in seconds) for the MQTT connection to maintain connectivity
  mqtt_client.setCallback(mqttCallback);          // Set the callback function to be invoked when MQTT messages are received
  connectToMQTT();                                // Attempt to establish a connection to the anedya broker
  mqtt_client.subscribe(responseTopic.c_str());   // subscribe to get response
  mqtt_client.subscribe(errorTopic.c_str());      // subscibe to get error
  mqtt_client.subscribe(commandTopic.c_str());

  setDevice_time();  // function to sync the the device time

  pinMode(fanPin, OUTPUT);
  pinMode(bulbPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  anedya_submitLog("", "Device Restarted!!");  //it will submit log to the anedya that device is started
}
//--------------------------------------------------LOOP-----------------------------------------------------------------------------------------------
void loop() {

  if (!mqtt_client.connected()) {
    connectToMQTT();
  }

  //----------------------------------LDR-------------------------
  if (millis() - submitLdrTimer >= 10000 or intialTen<10) {
   
   // Read the ldr valur and handle value fluation
    int ldrValue = analogRead(ldrPin);
    ldrTotal -= ldrReadings[ldrIndex];
    ldrReadings[ldrIndex] = ldrValue;
    ldrTotal += ldrReadings[ldrIndex];
    ldrIndex = (ldrIndex + 1) % numReadings;
    int ldrAverage = ldrTotal / numReadings;
    intialTen++;

    Serial.print("LDR Value: ");
    float mapLdrValue = map(ldrAverage, 850, 1024, 0, 100); //mapping the value
    Serial.println(mapLdrValue);
  
    if(intialTen>10){
    anedya_submitData("ldr", mapLdrValue);
    intialTen=12;
    }
    submitLdrTimer = millis(); //interval handler
  }
//--------------------------------------------------------fan-------------------------------
 if ( fan_commandId != "") { 
    if (fanStatus == "on" || fanStatus == "ON") {
      digitalWrite(fanPin, HIGH);  //turn on the fan 
      Serial.println("Fan ON");
      beep(250);
      String statusSuccessPayload = "{\"reqId\": \"\",\"commandId\": \"" + fan_commandId + "\",\"status\": \"success\",\"ackdata\": \"\",\"ackdatatype\": \"\"}";
      mqtt_client.publish(statusTopic.c_str(), statusSuccessPayload.c_str());  //publish the fan status
      anedya_submitLog("", "Fan ON");
    } else if (fanStatus == "off" || fanStatus == "OFF") {
      digitalWrite(fanPin, LOW);
      Serial.println("Fan OFF");
       beep(250);
      String statusSuccessPayload = "{\"reqId\": \"\",\"commandId\": \"" + fan_commandId + "\",\"status\": \"success\",\"ackdata\": \"\",\"ackdatatype\": \"\"}";
      mqtt_client.publish(statusTopic.c_str(), statusSuccessPayload.c_str());
      anedya_submitLog("", "Fan OFF");
    } else {
      String statusSuccessPayload = "{\"reqId\": \"\",\"commandId\": \"" + fan_commandId + "\",\"status\": \"failure\",\"ackdata\": \"\",\"ackdatatype\": \"\"}";
      mqtt_client.publish(statusTopic.c_str(), statusSuccessPayload.c_str());
      Serial.println("Invalid Command");
    }
    fan_commandId = "";  // checks
  }

 //--------------------------------------------------------bulb------------------------------
if (bulb_commandId != "") {  // condition block to publish the command success or failure message
    if (bulbStatus == "on" || bulbStatus == "ON") {
      digitalWrite(bulbPin, HIGH);
       beep(250);
      Serial.println("Bulb ON");
      String statusSuccessPayload = "{\"reqId\": \"\",\"commandId\": \"" + bulb_commandId + "\",\"status\": \"success\",\"ackdata\": \"\",\"ackdatatype\": \"\"}";
      mqtt_client.publish(statusTopic.c_str(), statusSuccessPayload.c_str());
      anedya_submitLog("", "Bulb ON");
    } else if (bulbStatus == "off" || bulbStatus == "OFF") {
      digitalWrite(bulbPin, LOW);
      Serial.println("Bulb OFF");
       beep(250);
      String statusSuccessPayload = "{\"reqId\": \"\",\"commandId\": \"" + bulb_commandId + "\",\"status\": \"success\",\"ackdata\": \"\",\"ackdatatype\": \"\"}";
      mqtt_client.publish(statusTopic.c_str(), statusSuccessPayload.c_str());
      anedya_submitLog("", "Bulb OFF");
    } else {
      String statusSuccessPayload = "{\"reqId\": \"\",\"commandId\": \"" + bulb_commandId + "\",\"status\": \"failure\",\"ackdata\": \"\",\"ackdatatype\": \"\"}";
      mqtt_client.publish(statusTopic.c_str(), statusSuccessPayload.c_str());
      Serial.println("Invalid Command");
      anedya_submitLog("", "Invalid Command");   //submit log in the case of the unknown or invalid command
    }
    bulb_commandId = "";  //flow checks
  }

  mqtt_client.loop();
}

//<-------------------------------------------------Function Section------------------------------------------------------------------------------------->
//---------------------------------------MQTT connection function----------------------------------------------
void connectToMQTT() {
  while (!mqtt_client.connected()) {
    const char *client_id = deviceID;
    Serial.print("Connecting to Anedya Broker....... ");
    if (mqtt_client.connect(client_id, mqtt_username, mqtt_password))  // checks to check mqtt connection
    {
      Serial.println("Connected to Anedya broker");
    } else {
      Serial.print("Failed to connect to Anedya broker, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" Retrying in 5 seconds.");
      delay(5000);
    }
  }
}
//-------------------------------------------MQTT call back function--------------------------------------------
void mqttCallback(char *topic, byte *payload, unsigned int length) {
  // Serial.print("Message received on topic: ");
  // Serial.println(topic);
  char res[150] = "";
  for (unsigned int i = 0; i < length; i++) {
    res[i] = payload[i];
  }
  String str_res(res);
 // Serial.println(str_res);
  JsonDocument Response;
  deserializeJson(Response, str_res);
  if (Response["deviceSendTime"])  // block to get the device send time
  {
    timeRes = str_res;
  } else if (Response["command"])  // block to get the command
  {
    commandId = String(Response["commandId"]);
    String equipment = Response["command"].as<String>();
    String statusReceivedPayload = "{\"reqId\": \"\",\"commandId\": \"" + commandId + "\",\"status\": \"received\",\"ackdata\": \"\",\"ackdatatype\": \"\"}";
    mqtt_client.publish(statusTopic.c_str(), statusReceivedPayload.c_str());
    if (equipment == "fan" or equipment == "Fan") {
      fanStatus = String(Response["data"]);
      fan_commandId = String(Response["commandId"]);
    } else if (equipment == "bulb" or equipment == "Bulb") {
      bulbStatus = String(Response["data"]);
      bulb_commandId = String(Response["commandId"]);
    }else{
          String statusReceivedPayload = "{\"reqId\": \"\",\"commandId\": \"" + commandId + "\",\"status\": \"failure\",\"ackdata\": \"\",\"ackdatatype\": \"\"}";
    mqtt_client.publish(statusTopic.c_str(), statusReceivedPayload.c_str());
    anedya_submitLog("","unknown command!!");
    beep(3000);
    }
  } else if (String(Response["errCode"]) == "0") {
    submitRes = str_res;
  } else  // block to debug errors
  {
    Serial.println(str_res);
  }
}

//------------------------------------------------------------------Time synchronization function---------------------------
// Function to configure time synchronization  with Anedya server
// For more info, visit [https://docs.anedya.io/devicehttpapi/http-time-sync/]
void setDevice_time() {
  String timeTopic = "$anedya/device/" + String(deviceID) + "/time/json";  // time topic wil provide the current time from the anedya server
  const char *mqtt_topic = timeTopic.c_str();
  // Attempt to synchronize time with Anedya server
  if (mqtt_client.connected()) {

    Serial.print("Time synchronizing......");

    boolean timeCheck = true;  // iteration to re-sync to ATS (Anedya Time Services), in case of failed attempt
    
    long long deviceSendTime;
    long long timeTimer = millis();
    while (timeCheck) {
      mqtt_client.loop();

      unsigned int iterate = 2000;
      if (millis() - timeTimer >= iterate)  
      {
        Serial.print(".");
        timeTimer = millis();
        deviceSendTime = millis();

        // Prepare the request payload
        StaticJsonDocument<200> requestPayload;             // Declare a JSON document with a capacity of 200 bytes
        requestPayload["deviceSendTime"] = deviceSendTime;  // Add a key-value pair to the JSON document
        String jsonPayload;                                 // Declare a string to store the serialized JSON payload
        serializeJson(requestPayload, jsonPayload);         // Serialize the JSON document into a string
        // Convert String object to pointer to a string literal
        const char *jsonPayloadLiteral = jsonPayload.c_str();
        mqtt_client.publish(mqtt_topic, jsonPayloadLiteral);

      }  

      if (timeRes != "")  
      {
        String strResTime(timeRes);

        // Parse the JSON response
        DynamicJsonDocument jsonResponse(100);      // Declare a JSON document with a capacity of 200 bytes
        deserializeJson(jsonResponse, strResTime);  // Deserialize the JSON response from the server into the JSON document

        long long serverReceiveTime = jsonResponse["serverReceiveTime"];  // Get the server receive time from the JSON document
        long long serverSendTime = jsonResponse["serverSendTime"];        // Get the server send time from the JSON document

        // Compute the current time
        long long deviceRecTime = millis();                                                                 // Get the device receive time
        long long currentTime = (serverReceiveTime + serverSendTime + deviceRecTime - deviceSendTime) / 2;  // Compute the current time
        long long currentTimeSeconds = currentTime / 1000;                                                  // Convert current time to seconds

        // Set device time
        setTime(currentTimeSeconds);  // Set the device time based on the computed current time
        Serial.println("\n synchronized!");
        timeCheck = false;
      }  // response check
    }
  } else {
    connectToMQTT();
  }  // mqtt connect check end
}  // set device time function end

//-----------------------------------------------------------submitdata--------------------------------------------------------------------
// Function to submit data to Anedya server
// For more info, visit [https://docs.anedya.io/devicehttpapi/submitdata/]
void anedya_submitData(String datapoint, float sensor_data)
{
  boolean check = true;

  String strSubmitTopic = "$anedya/device/" + String(deviceID) + "/submitdata/json";
  const char *submitTopic = strSubmitTopic.c_str();

  while (check)
  {
    if (mqtt_client.connected())
    {

      if (millis() - submitDataTimer >= 2000)
      {
        submitDataTimer = millis();
        // Get current time and convert it to milliseconds
        long long current_time = now();                     // Get the current time
        long long current_time_milli = current_time * 1000; // Convert current time to milliseconds

        // Construct the JSON payload with sensor data and timestamp
        String jsonStr = "{\"data\":[{\"variable\": \"" + datapoint + "\",\"value\":" + String(sensor_data) + ",\"timestamp\":" + String(current_time_milli) + "}]}";
        const char *submitJsonPayload = jsonStr.c_str();
        mqtt_client.publish(submitTopic, submitJsonPayload);
      }
      mqtt_client.loop();
      if (submitRes != "")
      {
        // Parse the JSON response
        DynamicJsonDocument jsonResponse(100);    // Declare a JSON document with a capacity of 200 bytes
        deserializeJson(jsonResponse, submitRes); // Deserialize the JSON response from the server into the JSON document

        int errorCode = jsonResponse["errCode"]; // Get the server receive time from the JSON document
        if (errorCode == 0)
        {
          Serial.println("Data pushed to Anedya!!");
        }
        else if (errorCode == 4040)
        {
          Serial.println("Failed to push data!!");
          Serial.println("unknown variable Identifier");
          Serial.println(submitRes);
        }
        else
        {
          Serial.println("Failed to push data!!");
          Serial.println(submitRes);
        }
        check = false;
        submitDataTimer=5000;
      }
    }
    else
    {
      connectToMQTT();
    } // mqtt connect check end
  }
}
//-------------------------------------------------------Function to submit log---------------------------------------------------------------
void anedya_submitLog(String reqID, String Log)
{
  boolean check = true;
  String strSubmitTopic = "$anedya/device/" + String(deviceID) + "/logs/submitLogs/json";
  const char *submitTopic = strSubmitTopic.c_str();
  while (check)
  {
    if (mqtt_client.connected())
    {
      if (millis() - submitLogTimer >= 2000)
      {
        submitLogTimer = millis();
        // Get current time and convert it to milliseconds
        long long current_time = now();                     // Get the current time
        long long current_time_milli = current_time * 1000; // Convert current time to milliseconds
        String strLog = "{\"reqId\":\"" + reqID + "\",\"data\":[{\"timestamp\":" + String(current_time_milli) + ",\"log\":\"" + Log + "\"}]}";
        const char *submitLogPayload = strLog.c_str();
        mqtt_client.publish(submitTopic, submitLogPayload);
      }
      mqtt_client.loop();
      if (submitRes != "")
      {
        // Parse the JSON response
        DynamicJsonDocument jsonResponse(100);    // Declare a JSON document with a capacity of 200 bytes
        deserializeJson(jsonResponse, submitRes); // Deserialize the JSON response from the server into the JSON document
        int errorCode = jsonResponse["errCode"];  // Get the server receive time from the JSON document
        if (errorCode == 0)
        {
          Serial.println("Log pushed to Anedya!!");
        }
        else
        {
          Serial.println("Failed to push!");
          Serial.println(submitRes);
        }
        check = false;
        submitLogTimer=5000;
      }
    }
    else
    {
      connectToMQTT();
    } // mqtt connect check end
  }
}
//------------------------------------------------Buzzer--------------------------------------------------------------------
void beep(unsigned int DELAY){
  digitalWrite(buzzerPin, HIGH);
  delay(DELAY);
  digitalWrite(buzzerPin,LOW);
}


