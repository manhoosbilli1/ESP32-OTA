#define TINY_GSM_MODEM_SIM800

//Increase RX buffer
#define TINY_GSM_RX_BUFFER 256

//NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
#include <TinyGPS++.h> //https://github.com/mikalhart/TinyGPSPlus
#include <TinyGsmClient.h> //https://github.com/vshymanskyy/TinyGSM
#include <ArduinoHttpClient.h> //https://github.com/arduino-libraries/ArduinoHttpClient
//NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
///new pakistan
//NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
const char FIREBASE_HOST[]  = "ssos-9b607-default-rtdb.firebaseio.com";
const String FIREBASE_AUTH  = "ettV8DrKulNiufBLMtgvgrlis4Ba3PSOOe0VihsZ";
const String FIREBASE_PATH  = "/users/user1";
const String FIREBASE_PATH_HISTORY = "/users/user1/history";
const int SSL_PORT          = 443;
//NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN

//NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
// Your GPRS credentials
// Leave empty, if missing user or pass
char apn[]  = "internet";
char user[] = "";
char pass[] = "";
//NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN

//NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
//GSM Module RX pin to ESP32 2
//GSM Module TX pin to ESP32 4
#define rxPin 4
#define txPin 2
#define btn 18
HardwareSerial sim800(1);
TinyGsm modem(sim800);
//NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN

//NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
//GPS Module RX pin to ESP32 17
//GPS Module TX pin to ESP32 16
#define RXD2 16
#define TXD2 17
HardwareSerial neogps(2);
TinyGPSPlus gps;
//NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN

//NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
TinyGsmClientSecure gsm_client_secure_modem(modem, 0);
HttpClient http_client = HttpClient(gsm_client_secure_modem, FIREBASE_HOST, SSL_PORT);
//NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN

unsigned long previousMillis = 0;
long interval = 10000;
String gpsData = "";
//************
String number = "+923124392257";
const float maxDistance = 20000;
const float OmaxDistance = 25;


float initialLatitude = 33.621216;
float initialLongitude = 73.080624;

float OinitialLatitude = 33.720481;
float OinitialLongitude = 73.052629;

float latitudeF, longitudeF;
String latitude, longitude;
String city = "user one is out from islamabad";
String Name = "SOS USER 1";

volatile bool start = false;
void IRAM_ATTR isr() {
  start = true;
}


float getDistance(float, float,float, float);
void call() {
  if (start == true) {
    start = false;
    Serial.println("in call");
    sim800.println("AT");    //Sets the GSM Module in Text Mode
    delay(1000);
    sim800.println("AT+CMGF=1");    //Sets the GSM Module in Text Mode
    delay(1000);
    //Serial.println ("Set SMS Number");
    sim800.println("AT+CMGS=\"" + number + "\"\r"); //Mobile phone number to send message
    delay(1000);
    String SMS = "help me";
    sim800.println(SMS);
    delay(1000);
    sim800.println((char)26);// ASCII code of CTRL+Z
    delay(1000);
    
    
    sim800.print (F("ATD"));
    sim800.print (number);
    sim800.print (F(";\r\n"));
  }
}
String _readSerial() {
  
  
  if (sim800.available()>0) 
  {
    return sim800.readString();
  }
}

void gps_loop(); 

void setup() {
  Serial.begin(115200);
  pinMode(18, INPUT_PULLUP);
  attachInterrupt(18, isr, FALLING);
  Serial.println("esp32 serial initialize");

  sim800.begin(9600, SERIAL_8N1, rxPin, txPin);
  Serial.println("SIM800L serial initialize");

  neogps.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial.println("neogps serial initialize");
  delay(3000);

  //NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
  //Restart takes quite some time
  //To skip it, call init() instead of restart()
  Serial.println("Initializing modem...");
  modem.restart();
  String modemInfo = modem.getModemInfo();
  Serial.print("Modem: ");
  Serial.println(modemInfo);
  //NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN

  // Unlock your SIM card with a PIN
  //modem.simUnlock("1234");

  http_client.setHttpResponseTimeout(90 * 1000); //^0 secs timeout
}
//************


//************
void loop() {
  call();

  Serial.print(F("Connecting to "));
  Serial.print(apn);
  if (!modem.gprsConnect(apn, user, pass)) {
    Serial.println(" fail");
    delay(1000);
    return;
  }
  call();
  Serial.println(" OK");
  //NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN

  http_client.connect(FIREBASE_HOST, SSL_PORT);

  //MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
  while (true) {
    if (!http_client.connected()) {
      Serial.println();
      http_client.stop();// Shutdown
      Serial.println("HTTP  not connect");
      call();
      break;
    }
    else {
      gps_loop();
    }
  }
  //MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
}

void PostToFirebase(const char* method, const String & path , const String & data, HttpClient* http)
{
  String response;
  int statusCode = 0;
  http->connectionKeepAlive(); // Currently, this is needed for HTTPS

  String url;
  if (path[0] != '/') {
    url = "/";
  }
  url += path + ".json";
  url += "?auth=" + FIREBASE_AUTH;

  String contentType = "application/json";
  http->put(url, contentType, data);
  statusCode = http->responseStatusCode();
  Serial.print(F("Status code: "));
  Serial.println(statusCode);
  response = http->responseBody();
  Serial.print(F("Response: "));
  Serial.println(response);


  if (!http->connected())
  {
    Serial.println();
    http->stop();// Shutdown
    Serial.println("HTTP POST disconnected");
  }

}
//************


//************
void gps_loop()
{
  //NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
  //Can take up to 60 seconds
  boolean newData = false;
  for (unsigned long start = millis(); millis() - start < 7000;) {
    while (neogps.available() > 0) {
      if (gps.encode(neogps.read())) {
        newData = true;
        break;
      }
    }
  }
  //NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN

  //NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
  //If newData is true
  if (true) {


    newData = false;

    latitude = String(gps.location.lat(), 6); // Latitude in degrees (double)
    longitude = String(gps.location.lng(), 6); // Longitude in degrees (double)
    latitudeF = gps.location.lat();
    longitudeF = gps.location.lng();
    float distance = getDistance(latitudeF, longitudeF, initialLatitude, initialLongitude);
    float Odistance = getDistance(latitudeF, longitudeF, OinitialLatitude, OinitialLongitude);
    
     Serial.print("Latitude= ");
    Serial.print(latitude);
    Serial.print(" Longitude= ");
    Serial.println(longitude);
    
     Serial.print("distance: ");
    Serial.println(distance);   
     Serial.print("OFFICE distance: ");
    Serial.println(Odistance);
    
    gpsData = "{";
    gpsData += "\"lat\":" + latitude + ",";
    gpsData += "\"lng\":" + longitude + "";
    gpsData += "}";

  
  
    PostToFirebase("PATCH", FIREBASE_PATH, gpsData, &http_client);
    call(); 

  }
}
//NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN



float getDistance(float flat1, float flon1, float flat2, float flon2) {

  // Variables
  float dist_calc = 0;
  float dist_calc2 = 0;
  float diflat = 0;
  float diflon = 0;

  // Calculations
  diflat  = radians(flat2 - flat1);
  flat1 = radians(flat1);
  flat2 = radians(flat2);
  diflon = radians((flon2) - (flon1));

  dist_calc = (sin(diflat / 2.0) * sin(diflat / 2.0));
  dist_calc2 = cos(flat1);
  dist_calc2 *= cos(flat2);
  dist_calc2 *= sin(diflon / 2.0);
  dist_calc2 *= sin(diflon / 2.0);
  dist_calc += dist_calc2;

  dist_calc = (2 * atan2(sqrt(dist_calc), sqrt(1.0 - dist_calc)));

  dist_calc *= 6371000.0; //Converting to meters

  return dist_calc;
}