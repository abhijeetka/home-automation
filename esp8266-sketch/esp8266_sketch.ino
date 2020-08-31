
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUDP.h>
#include <FS.h>
#include <Espalexa.h>


// -------- WIFI INFORMATION -----
const char *WIFI_SSID = "SSID";
const char *WIFI_PASSWORD = "password";
const char *AWS_IOT_ENDPOINT = "xxxxxxxxxxx-ats.iot.us-west-2.amazonaws.com";

//------Required for MQTT------ 
#define DEVICE_NAME "MyNewESP32"
#define AWS_MAX_RECONNECT_TRIES 50

// Topics for MQTT
#define AWS_IOT_PUBLISH_TOPIC "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

WiFiUDP wifiUDP;
NTPClient timeClient(wifiUDP, "pool.ntp.org");

void firstDeviceChanged(uint8_t brightness);
Espalexa espalexa;

// To control Lights , I am declaring variables.
int bedroomLight_01=5; // D1
String bedroomLight_01_status="off"; // specifies as its off

void callback(char* topic, byte* payload, unsigned int length) {

    bool change=false;
    Serial.println("incoming msg: ");
    StaticJsonDocument<512> doc;
    deserializeJson(doc, payload);
    Serial.println("Incoming Message says :");
    Serial.println((char)doc["state"]);
    serializeJsonPretty(doc, Serial);

    String deviceName = doc["state"]["reported"]["deviceName"];
    String deviceState = doc["state"]["reported"]["state"];

    // Here we are not checking the device name and only performing operation for a single light
    if(!bedroomLight_01_status.equals(deviceState)){
            Serial.println("added");
            actionPerform(deviceName,deviceState);
            sendJsonToAWS();
    }

}


WiFiClientSecure wifiSecure;
PubSubClient client(AWS_IOT_ENDPOINT, 8883, callback, wifiSecure);



void connectToWiFi(){
  
      // This function will try to connect to WIFI
      Serial.println(" Trying to connect to WiFi..");
      Serial.println(WIFI_SSID);
      
      WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
    
      // try only for 15 times and then take a pause.
      int retries = 0;
      while(WiFi.status() != WL_CONNECTED && retries < 15) {
        delay(500);
        Serial.print(".");
        retries++;  
      }
    
        Serial.println("WiFi Connected");
        Serial.println("IP Address: ");
        Serial.println(WiFi.localIP());
        timeClient.begin();
        while(!timeClient.update()){
            timeClient.forceUpdate();
        }

        wifiSecure.setX509Time(timeClient.getEpochTime());
       
}

void loadCertificates(){

  
      Serial.print("Heap: ");
      Serial.println(ESP.getFreeHeap());

      //Load Certificate file named as cert.der
      File cert = SPIFFS.open("/cert.der","r");
      if(!cert){
          Serial.println("Failed to Load Cert File");  
      }
      else{
          Serial.println("Succssfully opend cert file");  
      }

      // Load Private Key File
      File privateKey = SPIFFS.open("/private.der","r");
      if(!privateKey){
          Serial.println("Failed to Load Private Key"); 
      }
      else{
          Serial.println("Succssfully Loaded Private Key"); 
      }

      // Load CA Cert File
      File caCert = SPIFFS.open("/ca.der","r");
      if(!caCert){
          Serial.println("Failed to Load CA Cert"); 
      }
      else{
          Serial.println("Succssfully Loaded CA Cert"); 
      }


      Serial.println("Loading certificates and Keys..");

      // Inject Certificate
      if(wifiSecure.loadCertificate(cert)){
          Serial.println("Certificate Loaded..");  
      }
      else{
          Serial.println("Failed to Load Certificate..");  
      }


      // Inject Private Key
      if(wifiSecure.loadPrivateKey(privateKey)){
          Serial.println("Private Key Loaded..");  
      }
      else{
          Serial.println("Failed to Load Private Key..");  
      }


      // Inject CA Cert 
      if(wifiSecure.loadCACert(caCert)){
          Serial.println("CA Cert Loaded..");  
      }
      else{
          Serial.println("Failed to Load CA Cert..");  
      }

      Serial.print("Heap: "); 
      Serial.println(ESP.getFreeHeap());


}


void connectToAWSIOT(){
            
          int retries = 0;
          Serial.println("Connecting to AWS IOT");
      
          while(!client.connect(DEVICE_NAME) && retries < AWS_MAX_RECONNECT_TRIES ) {  
              Serial.print(".");
              delay(100);
              retries++;
          }

          if(!client.connected()){
              Serial.println("Timeout connecting to AWS IOT");
              return;  
          }

          client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

          Serial.println("Successfully Connected to AWS IOT");
}


void sendJsonToAWS(){

  
    StaticJsonDocument<512> jsonDoc;
    JsonObject stateObj = jsonDoc.createNestedObject("state");
    JsonObject reportedObj = stateObj.createNestedObject("reported");

    reportedObj["deviceName"].set("bedroom light");
    reportedObj["state"].set(bedroomLight_01_status);
    reportedObj["wifi_strength"].set(WiFi.RSSI());

    Serial.println("Publishing message to AWS...");
    char jsonBuffer[512];
    serializeJson(jsonDoc, jsonBuffer);
    Serial.println(jsonBuffer);
    client.publish(AWS_IOT_PUBLISH_TOPIC,jsonBuffer);
    Serial.println("Successfully Publishing message to AWS...");
    
  
}

void actionPerform(String lightName, String action){
      Serial.println("Inside Action Perform" );
      if(lightName.equals("bedroom light")){
            Serial.println("Light name is bedroom light" );
            //Check the action value
            if(bedroomLight_01_status.equals("off") && action.equals("on")){
                // Turn on the Lights
                Serial.println("We are inside");
                digitalWrite(bedroomLight_01,HIGH);
                bedroomLight_01_status="on";
                delay(100);
            }
            else{
                Serial.println("We are inside condition false");
                digitalWrite(bedroomLight_01,LOW);
                bedroomLight_01_status="off"; 
                delay(100); 
            }  
      }
  
}

void lightsInitalize(){

     // Setup the inbuilt LED Pin
     pinMode(LED_BUILTIN, OUTPUT); 
     // Low BedroomLight_01
     pinMode(bedroomLight_01,OUTPUT);
     digitalWrite(bedroomLight_01, LOW);

}

void firstDeviceChanged(uint8_t brightness) {
  //brightness parameter contains the new device state (0:off,255:on,1-254:dimmed)
  
  //do what you'd like to happen here (e.g. control an LED)
  Serial.println("First Device Changed");
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  lightsInitalize();
  connectToWiFi();

  // Mount fileSystem
  if(!SPIFFS.begin()){
      Serial.println("Failed to Mount File System..");
      return;  
  }
  loadCertificates();
  connectToAWSIOT();
  sendJsonToAWS();
  espalexa.addDevice("Device 01", firstDeviceChanged);
  espalexa.begin();
}



void loop() {
  // put your main code here, to run repeatedly:
  
  client.loop();
  espalexa.loop();
  delay(1000);
  digitalWrite(LED_BUILTIN, HIGH); // turn the LED on (HIGH is the voltage level)
  delay(100); // wait for a second
  digitalWrite(LED_BUILTIN, LOW); // turn the LED off by making the voltage LOW
  delay(100); // wait for a second
}