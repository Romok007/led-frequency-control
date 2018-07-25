#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <Ticker.h>
#include <ESP.h>

#define LED_OUT LED_BUILTIN
#define BUTTON_IN_1 D1
#define BUTTON_IN_2 D2

#define LED_ON (1u)
#define LED_OFF (0u)

#define LED_1_ON (digitalWrite(LED_OUT, LOW))
#define LED_1_OFF (digitalWrite(LED_OUT, HIGH))

#define LED_FREQUENCY_STATE_10dot0Hz  (1u)
#define LED_FREQUENCY_STATE_05dot0Hz  (2u)
#define LED_FREQUENCY_STATE_04dot0Hz  (3u)
#define LED_FREQUENCY_STATE_02dot0Hz  (4u)
#define LED_FREQUENCY_STATE_01dot0Hz  (5u)
#define LED_FREQUENCY_STATE_00dot5Hz  (6u)
#define LED_FREQUENCY_STATE_00dot2Hz  (7u)
#define LED_FREQUENCY_STATE_00dot1Hz  (8u)

const char *ssid = "Romok_MCU";
const char *password = "12345678";

unsigned char u8LedOnOffState;
unsigned char u8LedBlinkState;
unsigned char u8LedBlinkPreviousState;

unsigned long volatile lastWdtReset;

String html;
String closeTag;
String result;

float blinkingRate = (500 / 1.0) * 0.001;
String operatingFreq;

IPAddress ip(192, 167, 11, 4);
IPAddress gateway(192, 167, 11, 4);
IPAddress subnet(255, 255, 255, 0);
ESP8266WebServer server(80);

Ticker blinker;

void setup() {
  delay(1000);

  ESP.wdtDisable();

  WiFi.softAPConfig(ip, gateway, subnet);
  WiFi.softAP(ssid, password);

  server.on("/", handleRoot);
  server.begin();

  pinMode(LED_OUT, OUTPUT);

  // initialize the pushbutton pins as an input:
  pinMode(BUTTON_IN_1, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_IN_1), button_1_ISR, FALLING);

  pinMode(BUTTON_IN_2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_IN_2), button_2_ISR, FALLING);

  u8LedOnOffState = LED_OFF;
  u8LedBlinkState = LED_FREQUENCY_STATE_01dot0Hz;
  u8LedBlinkPreviousState = LED_FREQUENCY_STATE_01dot0Hz;
  operatingFreq = "1.0 Hz";

  html = "<html><body><h1><b><center><font size = '5'>LED Frequency Controller</font></center></b></h1><head><meta http-equiv='refresh'content='5'></head><h2><b><center>";
  closeTag = "</center></b></h2><form method='post'><input type='radio' name='ledControl' value='inc'>INCREASE FREQUENCY</input><br/><input type='radio' name='ledControl' value='dec'>DECREASE FREQUENCY</input><br/><input type='radio' name='ledControl' value='rst'>RESET</input><br/><br/><br/><input type='submit' value='Submit'/></form></body></html>";
  result = html + operatingFreq + closeTag;

  blinker.attach(blinkingRate, led_blink_ISR);

  ESP.wdtEnable(2000);

  Serial.begin(9600);
}

void loop() {
  Serial.println(u8LedBlinkState);
  server.handleClient();
  ESP.wdtFeed();
}

void handleRoot() {
  if (server.hasArg("ledControl")) {
    handleSubmit();
  } else {
    server.send(200, "text/html", result);
  }
}

void handleSubmit() {

  if (!server.hasArg("ledControl")) return;

  String radioState;
  radioState = server.arg("ledControl");
  if (radioState == "inc") {
    button_1_ISR();
    server.send(200, "text/html", result);
  }
  else if (radioState == "dec") {
    button_2_ISR();
    server.send(200, "text/html", result);
  } else if (radioState == "rst") {
    mcu_reset();
  }
  else {
    return;
  }
}

//LED Blinking ISR
void led_blink_ISR() {
  digitalWrite(LED_OUT, !(digitalRead(LED_OUT)));
}

//Increase frequency ISR
void button_1_ISR() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();

  if (interrupt_time - last_interrupt_time > 100) {
    switch (u8LedBlinkState) {
      case LED_FREQUENCY_STATE_10dot0Hz:
        u8LedBlinkState = LED_FREQUENCY_STATE_10dot0Hz;
        operatingFreq = "10.0 Hz";
        blinkingRate = (500 / 10.0) * 0.001;
        break;

      case LED_FREQUENCY_STATE_05dot0Hz:
        u8LedBlinkState = LED_FREQUENCY_STATE_10dot0Hz;
        operatingFreq = "10.0 Hz";
        blinkingRate = (500 / 10.0) * 0.001;
        break;

      case LED_FREQUENCY_STATE_04dot0Hz:
        u8LedBlinkState = LED_FREQUENCY_STATE_05dot0Hz;
        operatingFreq = "5.0 Hz";
        blinkingRate = (500 / 5.0) * 0.001;
        break;

      case LED_FREQUENCY_STATE_02dot0Hz:
        u8LedBlinkState = LED_FREQUENCY_STATE_04dot0Hz;
        operatingFreq = "4.0 Hz";
        blinkingRate = (500 / 4.0) * 0.001;
        break;

      case LED_FREQUENCY_STATE_01dot0Hz:
        u8LedBlinkState = LED_FREQUENCY_STATE_02dot0Hz;
        operatingFreq = "2.0 Hz";
        blinkingRate = (500 / 2.0) * 0.001;
        break;

      case LED_FREQUENCY_STATE_00dot5Hz:
        u8LedBlinkState = LED_FREQUENCY_STATE_01dot0Hz;
        operatingFreq = "1.0 Hz";
        blinkingRate = (500 / 1.0) * 0.001;
        break;

      case LED_FREQUENCY_STATE_00dot2Hz:
        u8LedBlinkState = LED_FREQUENCY_STATE_00dot5Hz;
        operatingFreq = "0.5 Hz";
        blinkingRate = (500 / 0.5) * 0.001;
        break;

      case LED_FREQUENCY_STATE_00dot1Hz:
        u8LedBlinkState = LED_FREQUENCY_STATE_00dot2Hz;
        operatingFreq = "0.2 Hz";
        blinkingRate = (500 / 0.2) * 0.001;
        break;

      default:
        break;
    }
    result = html + operatingFreq + closeTag;
  }
  blinker.detach();
  blinker.attach(blinkingRate, led_blink_ISR);
  last_interrupt_time = interrupt_time;
}

//Decrease Frequency ISR
void button_2_ISR() {
  // LED frequency increment State machine
  static unsigned long last_interrupt2_time = 0;
  unsigned long interrupt2_time = millis();

  if (interrupt2_time - last_interrupt2_time > 100) {
    switch (u8LedBlinkState) {

      case LED_FREQUENCY_STATE_10dot0Hz:
        u8LedBlinkState = LED_FREQUENCY_STATE_05dot0Hz;
        operatingFreq = "5.0 Hz";
        blinkingRate = (500 / 5.0) * 0.001;
        break;

      case LED_FREQUENCY_STATE_05dot0Hz:
        u8LedBlinkState = LED_FREQUENCY_STATE_04dot0Hz;
        operatingFreq = "4.0 Hz";
        blinkingRate = (500 / 4.0) * 0.001;
        break;

      case LED_FREQUENCY_STATE_04dot0Hz:
        u8LedBlinkState = LED_FREQUENCY_STATE_02dot0Hz;
        operatingFreq = "2.0 Hz";
        blinkingRate = (500 / 2.0) * 0.001;
        break;

      case LED_FREQUENCY_STATE_02dot0Hz:
        u8LedBlinkState = LED_FREQUENCY_STATE_01dot0Hz;
        operatingFreq = "1.0 Hz";
        blinkingRate = (500 / 1.0) * 0.001;
        break;

      case LED_FREQUENCY_STATE_01dot0Hz:
        u8LedBlinkState = LED_FREQUENCY_STATE_00dot5Hz;
        operatingFreq = "0.5 Hz";
        blinkingRate = (500 / 0.5) * 0.001;
        break;

      case LED_FREQUENCY_STATE_00dot5Hz:
        u8LedBlinkState = LED_FREQUENCY_STATE_00dot2Hz;
        operatingFreq = "0.2 Hz";
        blinkingRate = (500 / 0.2) * 0.001;
        break;

      case LED_FREQUENCY_STATE_00dot2Hz:
        u8LedBlinkState = LED_FREQUENCY_STATE_00dot1Hz;
        operatingFreq = "0.1 Hz";
        blinkingRate = (500 / 0.1) * 0.001;
        break;

      case LED_FREQUENCY_STATE_00dot1Hz:
        u8LedBlinkState = LED_FREQUENCY_STATE_00dot1Hz;
        operatingFreq = "0.1 Hz";
        blinkingRate = (500 / 0.1) * 0.001;
        break;

      default:
        break;
    }
    result = html + operatingFreq + closeTag;
  }
  blinker.detach();
  blinker.attach(blinkingRate, led_blink_ISR);
  last_interrupt2_time = interrupt2_time;
}

void mcu_reset() {
  while (1) {};
}
