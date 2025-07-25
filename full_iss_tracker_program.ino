//Thanks for making the ISS tracker! Open-sourcing this project was a lot of work, but I'm excited to see what people do with it! -Will, youtube.com/@willdoesbuild

#include <ESP32Servo.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

//PUT YOUR WIFI CREDENTIALS HERE
const char* SSID = "Your SSID";
const char* PASSWORD =  "Your password";

Servo latitude;
Servo longitude;

#define longitudePin 18
#define latitudePin 19
#define button1Pin 25
#define button2Pin 33
#define WiFiLEDPin 23
#define lampLEDPin 26
#define lampTouchPin 32

float issLat;
float issLong;

static int longitudeMax = 2400;
static int longitudeMin = 620;

static int latitudeMax = 2500;
static int latitudeMin = 480;

short int currentLat = 0;
short int currentLong = 0;

bool lampState = 0;
bool touchRn;
long unsigned int lastUpdate = 0;

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(WiFiLEDPin, HIGH);
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  digitalWrite(WiFiLEDPin, LOW);
}

void setup() {
  Serial.begin(115200);

  longitude.attach(longitudePin);
  latitude.attach(latitudePin);

  pinMode(WiFiLEDPin, OUTPUT);
  pinMode(lampLEDPin, OUTPUT);
  pinMode(button1Pin, INPUT_PULLUP);
  pinMode(button2Pin, INPUT_PULLUP);

  initWiFi();

  goTo(0, -90);
  delay(1000);
  updateISSLocation();
  goToSlow(issLat, issLong);
}

void loop() {
  if (millis() - lastUpdate > 15000) {
    updateISSLocation();
    goToSlow(issLat, issLong);
    lastUpdate = millis();
  }
  //THIS HAS NOT BEEN TESTED YET, FUTURE WILL!!!!!!!!!
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(WiFiLEDPin, HIGH);
  } else {
    digitalWrite(WiFiLEDPin, LOW);
  }

  if (touchRead(lampTouchPin) < 50) {
    if (touchRn == 0) {
      touchRn = 1;
      if (lampState == 0) {
        digitalWrite(lampLEDPin, HIGH);
        lampState = 1;
      } else {
        digitalWrite(lampLEDPin, LOW);
        lampState = 0;
      }
    }
  } else {
    touchRn = 0;
  }

  if (digitalRead(button1Pin) == LOW) {
    goToSlow(0, -90);
    delay(5000);
    updateISSLocation();
    goToSlow(issLat, issLong);
    lastUpdate = millis();
  }
}

//pull the ISS location from open notify and update the variables
void updateISSLocation() {
  digitalWrite(WiFiLEDPin, HIGH);

  if ((WiFi.status() == WL_CONNECTED)) {
    HTTPClient client;

    client.begin("http://api.open-notify.org/iss-now.json");
    int httpCode = client.GET();

    if (httpCode > 0) {
      String payload = client.getString();
      Serial.println("n\Statuscode: " + String(httpCode));
      Serial.println(payload);

      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (error) {
        Serial.print("JSON deserialization failed: ");
        Serial.println(error.c_str());
        return;
      }

      issLat = doc["iss_position"]["latitude"].as<float>();
      issLong = doc["iss_position"]["longitude"].as<float>();

      Serial.print("pulled latitude: ");
      Serial.println(issLat);
      Serial.print("pulled longitude: ");
      Serial.println(issLong);
    }
  } else {
    Serial.println("Connection Lost");
  }
  digitalWrite(WiFiLEDPin, LOW);
}

//slowly go to a given location
void goToSlow(int latTarget, int longTargetUnadjusted) {
  int longTarget;
  if (longTargetUnadjusted < 0) {
    longTarget = longTargetUnadjusted + 360;
  } else {
    longTarget = longTargetUnadjusted;
  }
  for (bool reachedPosition = false; reachedPosition != true;) {
    if (latTarget > currentLat) {
      latitudeGoTo(currentLat + 1);
      //Serial.println("added");
    } else if (latTarget < currentLat) {
      latitudeGoTo(currentLat - 1);
      //Serial.println("subtracted");
    }
    if (longTarget > currentLong) {
      longitudeGoTo(currentLong + 1);
    } else if (longTarget < currentLong) {
      longitudeGoTo(currentLong - 1);
    }
    delay(25);
    if (currentLat == latTarget && currentLong == longTarget) {
      reachedPosition = true;
      //Serial.println("reached position");
    }
  }
}

//go to a location quickly
void goTo(int latitude, int longitudeUnadjusted) {
  int longitude;
  if (longitudeUnadjusted < 0) {
    longitude = longitudeUnadjusted + 360;
  } else {
    longitude = longitudeUnadjusted;
  }
  latitudeGoTo(latitude);
  longitudeGoTo(longitude);
}

void longitudeGoTo(int angle) {
  longitude.writeMicroseconds(map(angle, 360, 0, longitudeMin, longitudeMax));
  currentLong = angle;
}

void latitudeGoTo(int angle) {
  latitude.writeMicroseconds(map(angle, -90, 90, latitudeMin, latitudeMax));
  currentLat = angle;
}