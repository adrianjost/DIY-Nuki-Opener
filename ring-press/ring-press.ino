#include <ESP32Servo.h>

#include <WiFi.h>

#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <WebServer.h>

// CONFIGURATION START
#define HOSTNAME "DESIRED_NETWORK_HOSTNAME"
#define SSID "YOUR_WIFI_SSID"
#define PASSWORD "YOUR_WIFI_PASSWORD"

#define PIN_SERVO 23              // data pin of servo, must be a PWM Pin
#define PIN_AUDIO 36              // analog pin to measure volume
#define PRESSED 33                // servo position when button is pressed
#define RELEASED 135              // servo position when button shouldn't be pressed
#define FAST 10                   // delay between servo pos changes
#define SLOW 30                   // delay between servo pos changes
#define RING_THRESHOLD 1.1        // 1.1 => volume must be 110% of AVG to be detected as ringing
#define RING_TO_OPEN_DURATION 120 // in seconds
// CONFIGURATION END

// GLOBAL VARIABLES START
Servo myservo;
byte servoPos = 0;

WebServer server(80);

unsigned int avgVolume = 128;
unsigned long long ringToOpenUntil = 0;
// GLOBAL VARIABLES END

void openDoor()
{
  for (servoPos = RELEASED; servoPos >= PRESSED; servoPos -= 1)
  {
    myservo.write(servoPos);
    delay((servoPos > PRESSED + 20) ? FAST : SLOW);
  }
  delay(2000);
  for (servoPos = PRESSED; servoPos <= RELEASED; servoPos += 1)
  {
    myservo.write(servoPos);
    delay(FAST);
  }
}

// WEBSERVER HANDLER START
void handleOpenDoor()
{
  server.send(200, "text/plain", "OK");
  openDoor();
}

void handleRingToOpenDoor()
{
  server.send(200, "text/plain", "Ring to open activated");
  ringToOpenUntil = millis() + (RING_TO_OPEN_DURATION * 1000);
}

void handle_NotFound()
{
  server.send(404, "text/plain", "Not found");
}
// WEBSERVER HANDLER END

void setup()
{
  myservo.attach(PIN_SERVO);
  myservo.write(RELEASED);

  WiFi.mode(WIFI_STA);
  WiFi.hostname(HOSTNAME);
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    // Failed to connect
    delay(5000);
    ESP.restart();
  }

  ArduinoOTA.begin();

  // setup server roots
  server.on("/open", handleOpenDoor);
  server.on("/ring_to_open", handleRingToOpenDoor);
  server.onNotFound(handle_NotFound);
  server.begin();
}

void loop()
{
  ArduinoOTA.handle();
  server.handleClient();

  if (ringToOpenUntil > millis())
  {
    if (analogRead(PIN_AUDIO) > ((float)avgVolume * RING_THRESHOLD))
    {
      openDoor();
      ringToOpenUntil = millis();
    }
  }
  else
  {
    avgVolume = (avgVolume + analogRead(PIN_AUDIO)) / 2;
  }
}
