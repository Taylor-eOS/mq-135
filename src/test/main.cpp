#include <Arduino.h>

const int AO_PIN = 4;
const int DO_PIN = 10;
const unsigned long SAMPLE_INTERVAL_MS = 1000;
const int SAMPLES = 10;
const float V_REF = 3.3;
const int ADC_MAX = 4095;
const float RL_VALUE = 10000.0;
const bool DO_CALIBRATE = true;
const unsigned long CALIBRATE_DURATION_MS = 10000;
const float R1 = 9870.0;
const float R2 = 19680.0;
const float DIVIDER_GAIN = (R1 + R2) / R2;

float analogToVoltage(int raw)
{
  return (raw * V_REF / ADC_MAX) * DIVIDER_GAIN;
}

float resistanceFromADC(int raw)
{
  float vout = analogToVoltage(raw);
  if (vout <= 0.0001) return NAN;
  float rs = (V_REF * DIVIDER_GAIN - vout) * RL_VALUE / vout;
  return rs;
}

int readAverageADC(int pin, int samples)
{
  long sum = 0;
  for (int i = 0; i < samples; ++i)
  {
    sum += analogRead(pin);
    delay(10);
  }
  return int(sum / samples);
}

float calibrateR0(int pin, unsigned long durationMs)
{
  unsigned long start = millis();
  int count = 0;
  double accRs = 0.0;
  while (millis() - start < durationMs)
  {
    int raw = readAverageADC(pin, SAMPLES);
    float rs = resistanceFromADC(raw);
    if (isfinite(rs))
    {
      accRs += rs;
      ++count;
    }
    delay(200);
  }
  if (count == 0) return NAN;
  return float(accRs / count);
}

void setup()
{
  Serial.begin(115200);
  analogReadResolution(12);
  pinMode(DO_PIN, INPUT);
  pinMode(AO_PIN, INPUT);
  delay(200);
  Serial.println("MQ-135 test starting");
}

void loop()
{
  static float R0 = NAN;
  static bool calibrated = false;
  if (!calibrated)
  {
    if (DO_CALIBRATE)
    {
      Serial.println("Calibrating R0. Keep sensor in clean air.");
      R0 = calibrateR0(AO_PIN, CALIBRATE_DURATION_MS);
      if (!isnan(R0)) Serial.printf("Calibration done R0=%.1f ohm\n", R0);
      else Serial.println("Calibration failed");
    }
    calibrated = true;
  }
  int raw = readAverageADC(AO_PIN, SAMPLES);
  float voltage = analogToVoltage(raw);
  float rs = resistanceFromADC(raw);
  float ratio = isnan(R0) || !isfinite(R0) || !isfinite(rs) ? NAN : rs / R0;
  int doState = digitalRead(DO_PIN);
  if (isnan(voltage) || isnan(rs))
  {
    Serial.printf("RAW=%d INVALID READ Rs=NAN Rratio=NAN DO=%d\n", raw, doState);
  }
  else
  {
    Serial.printf("RAW=%d V=%.3fV Rs=%.1fohm Rratio=%.3f DO=%d\n", raw, voltage, rs, ratio, doState);
  }
  delay(SAMPLE_INTERVAL_MS);
}

