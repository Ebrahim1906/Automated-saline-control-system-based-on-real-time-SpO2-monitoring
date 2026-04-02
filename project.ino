#define BLYNK_TEMPLATE_ID "TMPL6OhNt5JTc"
#define BLYNK_TEMPLATE_NAME "Saline System"
#define BLYNK_AUTH_TOKEN "bMzmZNLM43IYbVSOWLnjU1eqd9vzEGGa"

#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Wire.h>
#include "MAX30105.h"

// WiFi credentials
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "TP-Link_C2EA";
char pass[] = "kainatt08";

// Sensor and data
MAX30105 particleSensor;
float SpO2 = 0;
float salineVolume = 0;
uint32_t tsLastReport = 0;
#define REPORTING_PERIOD_MS 1000

#define LED_PIN D6  // Physical LED connected to D6

void setup() {
  Serial.begin(115200);
  delay(1000);

  Blynk.begin(auth, ssid, pass);
  Serial.println("Connecting to WiFi and Blynk...");

  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("FAILED to initialize MAX30102");
    while (1);
  }

  Serial.println("SUCCESS: MAX30102 initialized");

  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeIR(0x0A);
  particleSensor.setPulseAmplitudeGreen(0);

  pinMode(LED_PIN, OUTPUT); // Set D6 as output for LED
}

void loop() {
  Blynk.run();

  long irValue = particleSensor.getIR();

  // Skip checking for beat since we are no longer calculating BPM
  SpO2 = calculateSpO2();               // Get SpO₂
  salineVolume = getSalineVolume(SpO2); // Determine saline need

  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    tsLastReport = millis();

    // Serial output for debugging
    Serial.print("SpO2: ");
    Serial.print(SpO2);
    Serial.print(" % / Saline Volume: ");
    Serial.print(salineVolume);
    Serial.println(" mL");

    // Send to Blynk app
    Blynk.virtualWrite(V3, SpO2);  // Update SpO2 on Blynk
    Blynk.virtualWrite(V4, salineVolume);  // Update saline volume on Blynk

    // LED control based on SpO2 (Physical LED and Blynk LED)
    if (SpO2 < 88) {
      digitalWrite(LED_PIN, HIGH);      // Turn on physical LED
      Blynk.virtualWrite(V5, 255);      // Turn on Blynk LED
    } else {
      digitalWrite(LED_PIN, LOW);       // Turn off physical LED
      Blynk.virtualWrite(V5, 0);        // Turn off Blynk LED
    }
  }
}

// SpO₂ calculation
float calculateSpO2() {
  const int samples = 100;
  float redMin = 100000, redMax = 0;
  float irMin = 100000, irMax = 0;
  float redSum = 0, irSum = 0;

  for (int i = 0; i < samples; i++) {
    uint32_t red = particleSensor.getRed();
    uint32_t ir = particleSensor.getIR();

    redSum += red;
    irSum += ir;

    if (red < redMin) redMin = red;
    if (red > redMax) redMax = red;
    if (ir < irMin) irMin = ir;
    if (ir > irMax) irMax = ir;

    delay(5);
  }

  float redAC = redMax - redMin;
  float redDC = redSum / samples;
  float irAC = irMax - irMin;
  float irDC = irSum / samples;

  float ratio = (redAC / redDC) / (irAC / irDC);
  float spo2 = 110.0 - 25.0 * ratio;

  if (spo2 > 100) spo2 = 100;
  if (spo2 < 70) spo2 = 70;

  return spo2;
}

// Saline volume logic
float getSalineVolume(float spo2) {
  if (spo2 < 80) {
    return 700.0;
  } else if (spo2 <= 85) {
    return 600.0;
  } else if (spo2 <= 88) {
    return 500.0;
  } else if (spo2 <= 92) {
    return 400.0;
  } else if (spo2 <= 95) {
    return 300.0;
  } else {
    return 0.0;
  }
}
