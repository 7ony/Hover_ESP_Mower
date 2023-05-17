#include <ESP8266WebServer.h>
#include <FS.h>
#include <ESP8266WiFi.h>

//Set Wifi ssid and password
char ssid[] = "";
char pass[] = "";


ESP8266WebServer server (80);


// ########################## DEFINES ##########################
#define HOVER_SERIAL_BAUD   115200      // [-] Baud rate for HoverSerial (used to communicate with the hoverboard)
#define START_FRAME         0xABCD     	// [-] Start frme definition for reliable serial communication
#define TIME_SEND           200         // [ms] Sending time interval


#include <SoftwareSerial.h>
SoftwareSerial HoverSerialWheels(16,5);       // RX, TX
SoftwareSerial HoverSerialCuttingMotors(4,0); // RX, TX

typedef struct{
   uint16_t start;
   int16_t  steer;
   int16_t  speed;
   uint16_t checksum;
} SerialCommand;
SerialCommand Wheels;
SerialCommand CuttingMotor;

// ########################## SEND ##########################
void Send()
{
  // Create command
  Wheels.start    = (uint16_t)START_FRAME;
  Wheels.checksum = (uint16_t)(Wheels.start ^ Wheels.steer ^ Wheels.speed);

  CuttingMotor.start    = (uint16_t)START_FRAME;
  CuttingMotor.checksum = (uint16_t)(CuttingMotor.start ^ CuttingMotor.steer ^ CuttingMotor.speed);

  //Serial.println("steer = " + String(Command.steer) + " speed = " + String(Command.speed));
  // Write to Serial
  HoverSerialWheels.write((uint8_t *) &Wheels, sizeof(SerialCommand)); 
  HoverSerialCuttingMotors.write((uint8_t *) &CuttingMotor, sizeof(SerialCommand)); 

}

//This function takes the parameters passed in the URL(the x and y coordinates of the joystick)
//and sets the motor speed based on those parameters. 
void handleJSData(){
  Wheels.steer = server.arg(0).toInt() * 10;
  Wheels.speed = server.arg(1).toInt() * 2;
  CuttingMotor.speed = server.arg(2).toInt()*10;
  Send();
  //return an HTTP 200
  server.send(200, "text/plain", "");   
}

void setup()
{
  Serial.begin(9600);
  Serial.println("\nWait wifi connection");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Debug console
  Serial.begin(9600);
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  //initialize SPIFFS to be able to serve up the static HTML files. 
  if (!SPIFFS.begin()){
    Serial.println("SPIFFS Mount failed");
  } 
  else {
    Serial.println("SPIFFS Mount succesfull");
  }
  //set the static pages on SPIFFS for the html and js
  server.serveStatic("/", SPIFFS, "/joystick.html"); 
  server.serveStatic("/virtualjoystick.js", SPIFFS, "/virtualjoystick.js");
  //call handleJSData function when this URL is accessed by the js in the html file
  server.on("/jsData.html", handleJSData);  
  server.begin();
  Wheels.steer = 0;
  Wheels.speed = 0;
  CuttingMotor.steer = 0;
  CuttingMotor.speed = 0;
  HoverSerialWheels.begin(HOVER_SERIAL_BAUD);
  HoverSerialCuttingMotors.begin(HOVER_SERIAL_BAUD);
}

unsigned long previousMillis = 0;

void loop()
{
  unsigned long currentMillis = millis(); 
  server.handleClient();
  if (currentMillis - previousMillis >= TIME_SEND) {
    previousMillis = currentMillis;
    Send();
  }
  
}