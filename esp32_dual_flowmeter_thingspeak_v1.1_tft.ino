//code for v1.1 TFT display (Adafruit_ST7789.h)

#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>    
#include <HTTPClient.h>
#include <Fonts/FreeSansBoldOblique24pt7b.h>

// --- Pin Definitions ---
#define SD_CS        4    // SD card chip select
#define TFT_CS       5    // TFT chip select
#define TFT_DC       16
#define TFT_RST      17

// Flow sensor pins
#define SENSOR_INLET     13
#define SENSOR_OUTLET    12

// Push button pin
#define BUTTON_MODE      33   // Button to change display mode

// WiFi credentials
const char* ssid = "iPhone";
const char* password = "1111111111";

unsigned long lastWiFiAttempt = 0;

// Timing & calibration for flow sensors
unsigned long currentMillis = 0;
unsigned long previousMillis = 0;
const int interval = 1000;      
const float inletCalibrationFactor = 74.905;      
const float outletCalibrationFactor = 74.905;      

// Inlet variables
volatile byte pulseCountInlet;
byte pulse1SecInlet = 0;
float flowRateInlet = 0.0;
float flowLitresInlet = 0;
float totalLitresInlet = 0.0;
unsigned long totalPulseCountInlet = 0;
String lastPumpStatusInlet = "OFF";

// Outlet variables
volatile byte pulseCountOutlet;
byte pulse1SecOutlet = 0;
float flowRateOutlet = 0.0;
float flowLitresOutlet = 0;
float totalLitresOutlet = 0.0;
unsigned long totalPulseCountOutlet = 0;
String lastPumpStatusOutlet = "OFF";

// --- Display State and Mode ---
enum UnitMode { LPS, LPM, LPH };
UnitMode currentMode = LPS; // initial display mode

// Debounce settings for push button
unsigned long lastDebounceDisplay = 0;
unsigned long lastDebounceMode = 0;
const unsigned long debounceDelay = 500; // milliseconds

// ThingSpeak settings
const unsigned long thingSpeakInterval = 300000; 
unsigned long lastThingSpeakUpdate = 0;
const char* thingspeakServer = "http://api.thingspeak.com";
const char* thingSpeakWriteAPIKey = "WPFY9MS2R1GJJQGU";

// --- TFT Display ---
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// --- SD File ---
File myFile;
const char* filename = "/flow_data.txt";

volatile uint32_t lastUsInlet = 0;
volatile uint32_t lastUsOutlet = 0;

const uint32_t MIN_GAP_US = 0; 

// --- Forward declaration of functions ---
void loadVolumesFromSD();
float convertFlowRate(float rate_Lps);
String getUnitLabel();
void writeToSD(float rate, float volume, unsigned long pulse, const String& status, const String& type);
void drawUILayout();
void updateDynamicFields(float currentFlowInlet, float totalFlowInlet,
                         float currentFlowOutlet, float totalFlowOutlet);
void tryWiFiConnect();
void checkPushButtons();
void showSplash();

void updateThingSpeak(float totalLitresInlet, float totalLitresOutlet) {
  HTTPClient http;
  String url = String(thingspeakServer) + "/update?api_key=" + thingSpeakWriteAPIKey +
               "&field1=" + String(totalLitresInlet, 2) +
               "&field2=" + String(totalLitresOutlet, 2);
 
  http.begin(url); 
  int httpResponseCode = http.GET();
 
  if (httpResponseCode > 0) {
    Serial.print("ThingSpeak update successful, code: ");
    Serial.println(httpResponseCode);
  } else {
    Serial.print("ThingSpeak update failed, error: ");
    Serial.println(http.errorToString(httpResponseCode).c_str());
  }
  http.end();
}

void IRAM_ATTR pulseCounterInlet() {
  uint32_t now = micros();
  if (now - lastUsInlet > MIN_GAP_US){
    pulseCountInlet++;
    totalPulseCountInlet++;
    lastUsInlet = now;
  }
}

void IRAM_ATTR pulseCounterOutlet() {
  uint32_t now = micros();
  if (now - lastUsOutlet > MIN_GAP_US){
    pulseCountOutlet++;
    totalPulseCountOutlet++;
    lastUsOutlet = now;
  }
}

void loadVolumesFromSD() {
  if (SD.exists(filename)) {
    myFile = SD.open(filename, FILE_READ);
    if (!myFile) {
      Serial.println("Error opening file for reading.");
      return; 
    }
    String line;
    float lastInletVolume = 0.0;
    float lastOutletVolume = 0.0;
    while (myFile.available()) {
      line = myFile.readStringUntil('\n');
      line.trim();
      if (line.length() > 0) {
        if (line.startsWith("Inlet -")) {
          int idx = line.indexOf("Total volume in L:");
          if (idx != -1) {
            int start = idx + String("Total volume in L:").length();
            int end = line.indexOf(" L", start);
            if (end != -1) {
              String volStr = line.substring(start, end);
              lastInletVolume = volStr.toFloat();
            }
          }
        }
        else if (line.startsWith("Outlet -")) {
          int idx = line.indexOf("Total volume in L:");
          if (idx != -1) {
            int start = idx + String("Total volume in L:").length();
            int end = line.indexOf(" L", start);
            if (end != -1) {
              String volStr = line.substring(start, end);
              lastOutletVolume = volStr.toFloat();
            }
          }
        }
      }
    }
    myFile.close();
    totalLitresInlet = lastInletVolume;
    totalLitresOutlet = lastOutletVolume;
    Serial.print("Loaded Inlet Volume: ");
    Serial.println(totalLitresInlet);
    Serial.print("Loaded Outlet Volume: ");
    Serial.println(totalLitresOutlet);
  } else {
    myFile = SD.open(filename, FILE_WRITE);
    if (myFile) {
      myFile.println("Flow Data Log Initialized");
      myFile.close();
      Serial.println("File created and initial volumes set to zero.");
    } else {
      Serial.println("Failed to create log file.");
    }
  }
}

void tryWiFiConnect() {
  if (WiFi.status() != WL_CONNECTED) {
    unsigned long now = millis();
    if (now - lastWiFiAttempt > 5000) {
      Serial.println("Trying to connect to WiFi...");
      WiFi.begin(ssid, password);
      lastWiFiAttempt = now;
    }
  }
}

void writeToSD(float rate, float volume, unsigned long pulse, const String& status, const String& type) {
  myFile = SD.open(filename, FILE_APPEND);
  if (myFile) {
    myFile.print(type); myFile.print(" - Flow rate: ");
    myFile.print(rate, 2); myFile.print(" unit, Total volume in L: ");
    myFile.print(volume, 2); myFile.print(" L, Total Pulse: ");
    myFile.print(pulse); myFile.print(", Pump Status: ");
    myFile.println(status);
    myFile.close();
  } else {
    Serial.println("Failed to open SD file for writing.");
  }
}

// =======================================================
// ===          MANUALLY INVERTED COLORS HERE          ===
// =======================================================

void drawUILayout() {
  // Background set to WHITE (inverted from BLACK)
  tft.fillScreen(ST77XX_WHITE);  
  tft.setTextSize(3);
  // Main title set to BLACK (inverted from WHITE)
  tft.setTextColor(ST77XX_BLACK); 
  tft.setCursor(2, 5);
  tft.println("FLOWMETER READOUT");
  tft.drawLine(3, 35, 318, 35, ST77XX_BLACK); // Line set to BLACK
  tft.setTextSize(3);
  // Inlet/Outlet set to RED (inverted from CYAN)
  tft.setTextColor(ST77XX_RED); 
  tft.setCursor(5, 45);
  tft.println("INLET");
  tft.setCursor(164, 45);
  tft.println("OUTLET");
  tft.setTextSize(2);
  // Labels set to BLUE (inverted from YELLOW)
  tft.setTextColor(ST77XX_BLUE); 
  tft.setCursor(5, 80);
  tft.println("CURRENT FLOW");
  tft.setCursor(5, 162);
  tft.println("TOTAL FLOW");
  tft.setCursor(164, 80);
  tft.println("CURRENT FLOW");
  tft.setCursor(164, 162);
  tft.println("TOTAL FLOW");
  // Rectangles set to BLACK (inverted from WHITE)
  tft.drawRect(3, 75, 153, 160, ST77XX_BLACK);
  tft.drawRect(162, 75, 153, 160, ST77XX_BLACK);
}

float convertFlowRate(float rate_Lps) {
  if (currentMode == LPS)
    return rate_Lps;
  else if (currentMode == LPM)
    return rate_Lps * 60.0;
  else if (currentMode == LPH)
    return rate_Lps * 3600.0;
  return rate_Lps; 
}

String getUnitLabel() {
  if (currentMode == LPS)
    return "L/sec";
  else if (currentMode == LPH)
    return "L/hour";
  else
    return "L/min";
}

void updateDynamicFields(float currentFlowInlet, float totalFlowInlet,
                         float currentFlowOutlet, float totalFlowOutlet) {
  String unitLabel = getUnitLabel();
  // FillRect set to WHITE (inverted from BLACK)
  tft.fillRect(5, 118, 150, 20, ST77XX_WHITE);
  tft.fillRect(5, 200, 150, 20, ST77XX_WHITE);
  tft.setTextSize(2);
  // Data values set to BLACK (inverted from WHITE)
  tft.setTextColor(ST77XX_BLACK); 
  tft.setCursor(5, 118);
  tft.print(currentFlowInlet, 1);
  tft.print(" ");
  tft.print(unitLabel);
  tft.setCursor(5, 200);
  tft.print(totalFlowInlet, 1);
  tft.print(" L");
 
  // FillRect set to WHITE (inverted from BLACK)
  tft.fillRect(164, 118, 150, 20, ST77XX_WHITE);
  tft.fillRect(164, 200, 150, 20, ST77XX_WHITE);
  tft.setCursor(164, 118);
  tft.print(currentFlowOutlet, 1);
  tft.print(" ");
  tft.print(unitLabel);
  tft.setCursor(164, 200);
  tft.print(totalFlowOutlet, 1);
  tft.print(" L");
}

void showSplash(){
  // Splash screen manually inverted
  tft.fillScreen(ST77XX_WHITE); // WHITE background
  tft.setFont(&FreeSansBoldOblique24pt7b);
  tft.setTextColor(ST77XX_BLACK, ST77XX_WHITE); // BLACK text
  const char* msg = "HAZ LABS";
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(msg, 0, 0, &x1, &y1, &w, &h);
  uint16_t screenW = 320, screenH = 240;
  int16_t cx = (screenW - w)/2 - x1;
  int16_t cy = (screenH + h)/2 - y1;
  tft.setCursor(cx, cy);
  tft.print(msg);
  delay(2000);
  tft.setFont(NULL);
}



void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(TFT_CS, OUTPUT); digitalWrite(TFT_CS, HIGH);
  pinMode(SD_CS, OUTPUT); digitalWrite(SD_CS, HIGH);
 
  pinMode(SENSOR_INLET, INPUT_PULLUP);
  pinMode(SENSOR_OUTLET, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SENSOR_INLET), pulseCounterInlet, FALLING);
  attachInterrupt(digitalPinToInterrupt(SENSOR_OUTLET), pulseCounterOutlet, FALLING);
 
  pinMode(BUTTON_MODE, INPUT_PULLUP);
 
  WiFi.mode(WIFI_STA);
  tryWiFiConnect();
 
  if (!SD.begin(SD_CS)) {
    Serial.println("SD Card initialization failed!");
  } else {
    Serial.println("SD Card initialized.");
    loadVolumesFromSD();
  }

  tft.init(240, 320);
  tft.setRotation(1); // Set display rotation

 

  showSplash();
  drawUILayout();
}

void checkPushButtons() { 
  if (digitalRead(BUTTON_MODE) == LOW) {
    if (millis() - lastDebounceMode > debounceDelay) {
      if (currentMode == LPS)
        currentMode = LPM;
      else if (currentMode == LPM)
        currentMode = LPH;
      else
        currentMode = LPS;
      Serial.print("Display mode changed to: ");
      Serial.println(getUnitLabel());
      lastDebounceMode = millis();
    }
  }
}

void loop() {
  currentMillis = millis();
  tryWiFiConnect();
  checkPushButtons();
 
  if (currentMillis - previousMillis >= interval) {
    pulse1SecInlet = pulseCountInlet;
    pulseCountInlet = 0;
    flowRateInlet = ((1000.0 / interval) * pulse1SecInlet) / inletCalibrationFactor;
    float displayFlowInlet = convertFlowRate(flowRateInlet);
    flowLitresInlet = flowRateInlet;
    totalLitresInlet += flowLitresInlet;
    String pumpStatusInlet = (flowRateInlet > 0.0) ? "ON" : "OFF";
   
    pulse1SecOutlet = pulseCountOutlet;
    pulseCountOutlet = 0;
    flowRateOutlet = ((1000.0 / interval) * pulse1SecOutlet) / outletCalibrationFactor;
    float displayFlowOutlet = convertFlowRate(flowRateOutlet);
    flowLitresOutlet = flowRateOutlet;
    totalLitresOutlet += flowLitresOutlet;
    String pumpStatusOutlet = (flowRateOutlet > 0.0) ? "ON" : "OFF";
   
    previousMillis = currentMillis;
   
    Serial.println("=== FLOW DATA ===");
    Serial.print("Inlet  - Flow: "); Serial.print(flowRateInlet, 2);
    Serial.print(" L/sec, Display Flow: "); Serial.print(displayFlowInlet, 2);
    Serial.print(" "); Serial.print(getUnitLabel());
    Serial.print(", Total: "); Serial.print(totalLitresInlet, 2);
    Serial.print(" L, Pulses: "); Serial.print(totalPulseCountInlet);
    Serial.print(", Pump Status: "); Serial.println(pumpStatusInlet);
   
    Serial.print("Outlet - Flow: "); Serial.print(flowRateOutlet, 2);
    Serial.print(" L/sec, Display Flow: "); Serial.print(displayFlowOutlet, 2);
    Serial.print(" "); Serial.print(getUnitLabel());
    Serial.print(", Total: "); Serial.print(totalLitresOutlet, 2);
    Serial.print(" L, Pulses: "); Serial.print(totalPulseCountOutlet);
    Serial.print(", Pump Status: "); Serial.println(pumpStatusOutlet);
    Serial.println("=================\n");
   
    if (flowRateInlet > 0.0 || lastPumpStatusInlet == "ON") {
      writeToSD(flowRateInlet, totalLitresInlet, totalPulseCountInlet, pumpStatusInlet, "Inlet");
      lastPumpStatusInlet = pumpStatusInlet;
    }
   
    if (flowRateOutlet > 0.0 || lastPumpStatusOutlet == "ON") {
      writeToSD(flowRateOutlet, totalLitresOutlet, totalPulseCountOutlet, pumpStatusOutlet, "Outlet");
      lastPumpStatusOutlet = "ON";
    }

    if (currentMillis - lastThingSpeakUpdate >= thingSpeakInterval) {
      updateThingSpeak(totalLitresInlet, totalLitresOutlet);
      lastThingSpeakUpdate = currentMillis;
    }

    
    updateDynamicFields(displayFlowInlet, totalLitresInlet,
                        displayFlowOutlet, totalLitresOutlet);
  }
}