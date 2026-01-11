// ---------------- ENERGY MONITOR (ESP32 + ACS712-20A + ZMPT101B) ---------------- 
// Blynk Info 
#define BLYNK_TEMPLATE_ID "TMPL3H5ptsGjk" 
#define BLYNK_TEMPLATE_NAME "ENERGY CONSUMPTION MONITOR" 
#define BLYNK_AUTH_TOKEN "61VyLGHLcJ8RtzSflQV7yE49RRutKEfi" 
#include <WiFi.h> 
#include <BlynkSimpleEsp32.h> 
#include <EEPROM.h> 
#include "ACS712.h" 
#include <ZMPT101B.h> 
// WiFi Credentials 
char ssid[] = "abishek"; 
char pass[] = "12345678"; 
// ---------------- SENSOR SETTINGS ---------------- 
ACS712 ACS(34, 3.3, 4095, 100);     // 20A model → 100 mV/A 
ZMPT101B voltageSensor(35, 50.0);   // ADC pin + 50Hz AC 
// EEPROM 
#define EEPROM_SIZE 512 
#define UNIT_ADDRESS 0 
float energy_kWh = 0; 
unsigned long prevMillis = 0; 
unsigned long lastEEPROMWrite = 0; 
// Write EEPROM every 60 seconds 
const unsigned long EEPROM_WRITE_INTERVAL_MS = 60000; 
// Blynk Sync 
BLYNK_CONNECTED() { 
Blynk.syncVirtual(V0, V1, V2, V3); 
} 
void setup() { 
Serial.begin(115200); 
delay(200); 
EEPROM.begin(EEPROM_SIZE); 
energy_kWh = EEPROM.readFloat(UNIT_ADDRESS); 
if (isnan(energy_kWh)) energy_kWh = 0; 
// Auto-calibrate ACS712 midpoint 
ACS.autoMidPoint(); 
// ZMPT calibration (works for most modules) 
voltageSensor.setSensitivity(675.0f); 
32 
33 
 
  // Connect Blynk 
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass); 
  prevMillis = millis(); 
  lastEEPROMWrite = millis(); 
} 
 
// ---------------- READ CURRENT (AC RMS) ---------------- 
float readCurrent_mA() { 
  float sum = 0; 
  int samples = 200; 
 
  for (int i = 0; i < samples; i++) { 
    sum += ACS.mA_AC(); 
    delay(1); 
  } 
  float avg = sum / samples; 
  if (fabs(avg) < 10) return 0;  // ignore noise 
 
  return avg;  // in mA 
} 
// ---------------- MAIN LOOP ---------------- 
void loop() { 
  Blynk.run(); 
 
  unsigned long now = millis(); 
  float dt = (now - prevMillis) / 1000.0;  // seconds 
  prevMillis = now; 
 
  // Read Voltage 
  float voltage = voltageSensor.getRmsVoltage(); 
  if (voltage < 50) voltage = 0;   // ignore noise 
 
  // Read Current 
  float current_mA = readCurrent_mA() * 0.49; 
 
  float current_A = current_mA / 1000.0; 
 
  // Power (W) 
  float powerW = voltage * current_A; 
 
  // Energy 
  float delta_kWh = (powerW * dt) / 3600000.0;  
  energy_kWh += delta_kWh; 
 
  // ----------- SEND TO BLYNK ----------- 
  Blynk.virtualWrite(V0, voltage); 
  Blynk.virtualWrite(V1, current_mA); 
  Blynk.virtualWrite(V2, powerW); 
Blynk.virtualWrite(V3, energy_kWh); 
// ----------- EEPROM SAVE ----------- 
if (millis() - lastEEPROMWrite >= EEPROM_WRITE_INTERVAL_MS) { 
EEPROM.writeFloat(UNIT_ADDRESS, energy_kWh); 
EEPROM.commit(); 
lastEEPROMWrite = millis(); 
} 
// Debug Output 
Serial.print("V = "); Serial.print(voltage); 
Serial.print(" | I = "); Serial.print(current_mA); 
Serial.print(" mA | P = "); Serial.print(powerW); 
Serial.print(" W | kWh = "); Serial.println(energy_kWh); 
delay(200); 
}
