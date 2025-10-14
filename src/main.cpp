#include <WiFi.h>
#include <ESPSupabase.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include <time.h>
#include "config.h"

// LED strip
CRGB leds[NUM_LEDS];

// Supabase client
Supabase db;

// RTC memory to persist data across deep sleep
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR uint64_t nextAlarmTime = 0;

// Button variables
volatile bool buttonPressed = false;
unsigned long lastButtonPress = 0;

// Alarm structure
struct Alarm {
  int id;
  int hour;
  int minute;
  bool daysOfWeek[7]; // Sunday = 0, Monday = 1, etc.
  bool enabled;
  int brightness;
  int duration;
  String colorPreset;
};

Alarm alarms[MAX_ALARMS];
int alarmCount = 0;

// Color presets
struct ColorPreset {
  CRGB startColor;
  CRGB endColor;
  String name;
};

ColorPreset colorPresets[] = {
  {CRGB(64, 0, 0), CRGB(255, 255, 255), "sunrise"},    // Deep red to white
  {CRGB(0, 0, 64), CRGB(0, 255, 255), "ocean"},        // Deep blue to cyan
  {CRGB(0, 32, 0), CRGB(128, 255, 0), "forest"},       // Dark green to lime
  {CRGB(64, 0, 64), CRGB(255, 192, 203), "lavender"}   // Purple to pink
};

// Function declarations
void connectToWiFi();
void syncTime();
void fetchAlarms();
void parseAlarms(String jsonResponse);
void parseTime(String timeStr, int &hour, int &minute);
void checkAlarms();
void triggerSunriseAlarm(Alarm &alarm);
void calculateNextWakeTime();
void enterDeepSleep();
void IRAM_ATTR buttonISR();
void handleButtonPress();
void setupButton();

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  DEBUG_PRINTLN("=== Sunrise Alarm Clock Starting ===");
  DEBUG_PRINT("Boot count: ");
  DEBUG_PRINTLN(++bootCount);

  // Initialize LED strip
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(50);
  FastLED.clear();
  FastLED.show();

  // Setup button
  setupButton();

  // Connect to WiFi
  connectToWiFi();

  // Initialize Supabase
  db.begin(SUPABASE_URL, SUPABASE_KEY);

  // Sync time with NTP
  syncTime();

  // Fetch alarms from Supabase
  fetchAlarms();

  // Check if any alarm should trigger now
  checkAlarms();

  // Handle any button press for manual sync
  if (buttonPressed) {
    handleButtonPress();
  }

  // Calculate next wake time and go to deep sleep
  calculateNextWakeTime();
  enterDeepSleep();
}

void loop() {
  // This should never be reached due to deep sleep
  delay(1000);
}

void setupButton() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);
}

void IRAM_ATTR buttonISR() {
  unsigned long now = millis();
  if (now - lastButtonPress > BUTTON_DEBOUNCE_MS) {
    buttonPressed = true;
    lastButtonPress = now;
  }
}

void handleButtonPress() {
  DEBUG_PRINTLN("Button pressed - Manual sync triggered");
  buttonPressed = false;
  
  // Visual feedback - quick blue flash
  fill_solid(leds, NUM_LEDS, CRGB::Blue);
  FastLED.setBrightness(100);
  FastLED.show();
  delay(200);
  FastLED.clear();
  FastLED.show();
  
  // Force fetch alarms
  fetchAlarms();
  
  DEBUG_PRINTLN("Manual sync completed");
}

void connectToWiFi() {
  DEBUG_PRINTLN("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    DEBUG_PRINT(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    DEBUG_PRINTLN("\nWiFi connected!");
    DEBUG_PRINT("IP address: ");
    DEBUG_PRINTLN(WiFi.localIP());
    DEBUG_PRINT("MAC address: ");
    DEBUG_PRINTLN(WiFi.macAddress());
  } else {
    DEBUG_PRINTLN("\nWiFi connection failed!");
  }
}

void syncTime() {
  DEBUG_PRINTLN("Syncing time with NTP server...");
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
  
  struct tm timeinfo;
  int attempts = 0;
  while (!getLocalTime(&timeinfo) && attempts < 10) {
    delay(1000);
    attempts++;
  }
  
  if (attempts < 10) {
    DEBUG_PRINTLN("Time synchronized successfully");
    DEBUG_PRINT("Current time: ");
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  } else {
    DEBUG_PRINTLN("Failed to sync time");
  }
}

void fetchAlarms() {
  if (WiFi.status() != WL_CONNECTED) {
    DEBUG_PRINTLN("WiFi not connected, skipping alarm fetch");
    return;
  }

  DEBUG_PRINTLN("Fetching alarms from Supabase...");
  
  // Use ESPSupabase API to fetch alarms for this device
  String result = db.from("alarms").select("*").eq("device_id", WiFi.macAddress()).eq("is_enabled", "true").doSelect();
  
  if (result.length() > 0 && !result.startsWith("error")) {
    DEBUG_PRINTLN("Alarms fetched successfully");
    parseAlarms(result);
  } else {
    DEBUG_PRINT("Supabase Error: ");
    DEBUG_PRINTLN(result);
  }
}

void parseAlarms(String jsonResponse) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, jsonResponse);
  
  if (error) {
    DEBUG_PRINT("JSON parsing error: ");
    DEBUG_PRINTLN(error.c_str());
    return;
  }
  
  alarmCount = 0;
  JsonArray alarmsArray = doc.as<JsonArray>();
  
  for (JsonObject alarm : alarmsArray) {
    if (alarmCount >= MAX_ALARMS) break;
    
    alarms[alarmCount].id = alarm["id"];
    String timeStr = alarm["time"];
    parseTime(timeStr, alarms[alarmCount].hour, alarms[alarmCount].minute);
    
    JsonArray days = alarm["days_of_week"];
    for (int i = 0; i < 7; i++) {
      alarms[alarmCount].daysOfWeek[i] = false;
    }
    for (JsonVariant day : days) {
      int dayNum = day.as<int>();
      if (dayNum >= 0 && dayNum < 7) {
        alarms[alarmCount].daysOfWeek[dayNum] = true;
      }
    }
    
    alarms[alarmCount].enabled = alarm["is_enabled"];
    alarms[alarmCount].brightness = alarm["brightness_level"] | DEFAULT_BRIGHTNESS;
    alarms[alarmCount].duration = alarm["duration_minutes"] | DEFAULT_SUNRISE_DURATION;
    alarms[alarmCount].colorPreset = alarm["color_preset"] | "sunrise";
    
    DEBUG_PRINT("Loaded alarm: ");
    DEBUG_PRINT(alarms[alarmCount].hour);
    DEBUG_PRINT(":");
    DEBUG_PRINTLN(alarms[alarmCount].minute);
    
    alarmCount++;
  }
  
  DEBUG_PRINT("Total alarms loaded: ");
  DEBUG_PRINTLN(alarmCount);
}

void parseTime(String timeStr, int &hour, int &minute) {
  int colonIndex = timeStr.indexOf(':');
  if (colonIndex > 0) {
    hour = timeStr.substring(0, colonIndex).toInt();
    minute = timeStr.substring(colonIndex + 1).toInt();
  } else {
    hour = 0;
    minute = 0;
  }
}

void checkAlarms() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    DEBUG_PRINTLN("Failed to get current time");
    return;
  }
  
  int currentHour = timeinfo.tm_hour;
  int currentMinute = timeinfo.tm_min;
  int currentWeekday = timeinfo.tm_wday; // 0 = Sunday
  
  DEBUG_PRINT("Checking alarms at ");
  DEBUG_PRINT(currentHour);
  DEBUG_PRINT(":");
  DEBUG_PRINTLN(currentMinute);
  
  for (int i = 0; i < alarmCount; i++) {
    if (alarms[i].enabled && 
        alarms[i].hour == currentHour && 
        alarms[i].minute == currentMinute &&
        alarms[i].daysOfWeek[currentWeekday]) {
      
      DEBUG_PRINTLN("Alarm triggered! Starting sunrise simulation...");
      triggerSunriseAlarm(alarms[i]);
      return;
    }
  }
  
  DEBUG_PRINTLN("No alarms to trigger");
}

void triggerSunriseAlarm(Alarm &alarm) {
  DEBUG_PRINT("Starting sunrise alarm with preset: ");
  DEBUG_PRINTLN(alarm.colorPreset);
  
  // Find color preset
  ColorPreset preset = colorPresets[0]; // Default to sunrise
  for (int i = 0; i < 4; i++) {
    if (colorPresets[i].name == alarm.colorPreset) {
      preset = colorPresets[i];
      break;
    }
  }
  
  unsigned long startTime = millis();
  unsigned long duration = alarm.duration * 60000UL; // Convert to milliseconds
  
  while (millis() - startTime < duration) {
    float progress = (float)(millis() - startTime) / (float)duration;
    if (progress > 1.0) progress = 1.0;
    
    // Calculate current color
    CRGB currentColor = blend(preset.startColor, preset.endColor, progress * 255);
    
    // Calculate brightness
    int brightness = (int)(progress * alarm.brightness);
    FastLED.setBrightness(brightness);
    
    // Set all LEDs to current color
    fill_solid(leds, NUM_LEDS, currentColor);
    FastLED.show();
    
    delay(1000); // Update every second
  }
  
  // Keep LEDs on for 5 more minutes at full brightness
  FastLED.setBrightness(alarm.brightness);
  delay(300000); // 5 minutes
  
  // Fade out
  for (int brightness = alarm.brightness; brightness >= 0; brightness -= 5) {
    FastLED.setBrightness(brightness);
    FastLED.show();
    delay(100);
  }
  
  FastLED.clear();
  FastLED.show();
  
  DEBUG_PRINTLN("Sunrise alarm completed");
}

void calculateNextWakeTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    DEBUG_PRINTLN("Failed to get time for next wake calculation");
    nextAlarmTime = ALARM_CHECK_INTERVAL;
    return;
  }
  
  time_t now = mktime(&timeinfo);
  time_t nextAlarm = now + DEEP_SLEEP_DURATION / 1000000; // Default to 1 hour
  
  // Find the next alarm
  for (int i = 0; i < alarmCount; i++) {
    if (!alarms[i].enabled) continue;
    
    for (int day = 0; day < 8; day++) { // Check next 7 days + today
      struct tm alarmTime = timeinfo;
      alarmTime.tm_mday += day;
      alarmTime.tm_hour = alarms[i].hour;
      alarmTime.tm_min = alarms[i].minute;
      alarmTime.tm_sec = 0;
      
      mktime(&alarmTime); // Normalize the time structure
      
      if (alarms[i].daysOfWeek[alarmTime.tm_wday]) {
        time_t alarmTimestamp = mktime(&alarmTime);
        if (alarmTimestamp > now && alarmTimestamp < nextAlarm) {
          nextAlarm = alarmTimestamp;
        }
      }
    }
  }
  
  nextAlarmTime = (nextAlarm - now) * 1000000ULL; // Convert to microseconds
  
  DEBUG_PRINT("Next wake in ");
  DEBUG_PRINT(nextAlarmTime / 1000000);
  DEBUG_PRINTLN(" seconds");
}

void enterDeepSleep() {
  DEBUG_PRINTLN("Entering deep sleep...");
  
  // Turn off WiFi
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  
  // Clear LEDs
  FastLED.clear();
  FastLED.show();
  
  // Configure wake up sources
  esp_sleep_enable_timer_wakeup(min(nextAlarmTime, DEEP_SLEEP_DURATION));
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0); // Wake on button press (low level)
  
  // Enter deep sleep
  esp_deep_sleep_start();
}