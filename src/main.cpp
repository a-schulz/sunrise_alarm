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

// Enhanced Color presets with multiple stages for realistic sunrise
struct SunriseStage {
  CRGB color;
  float duration_percent; // Percentage of total duration for this stage
};

struct ColorPreset {
  SunriseStage stages[6]; // Up to 6 stages for complex transitions
  int stageCount;
  String name;
};

ColorPreset colorPresets[] = {
  // Realistic sunrise: Deep red -> Orange -> Yellow -> Warm white -> Cool white -> Bright white
  {
    {
      {CRGB(32, 0, 0), 0.15f},      // Deep red - pre-dawn
      {CRGB(80, 8, 0), 0.25f},      // Dark orange - early sunrise
      {CRGB(160, 32, 0), 0.35f},    // Orange - sunrise begins
      {CRGB(255, 80, 16), 0.20f},   // Bright orange - sun visible
      {CRGB(255, 180, 80), 0.15f},  // Warm yellow - morning light
      {CRGB(255, 220, 180), 0.10f}  // Warm white - daylight
    },
    6,
    "sunrise"
  },
  
  // Ocean depths to surface
  {
    {
      {CRGB(0, 8, 32), 0.20f},      // Deep ocean blue
      {CRGB(0, 32, 80), 0.25f},     // Mid-ocean blue
      {CRGB(0, 80, 160), 0.25f},    // Shallow water blue
      {CRGB(32, 160, 255), 0.20f},  // Surface blue
      {CRGB(80, 200, 255), 0.10f}   // Bright cyan
    },
    5,
    "ocean"
  },
  
  // Forest awakening
  {
    {
      {CRGB(8, 16, 0), 0.25f},      // Dark forest green
      {CRGB(16, 40, 8), 0.25f},     // Dawn green
      {CRGB(40, 80, 16), 0.25f},    // Morning green
      {CRGB(80, 160, 40), 0.15f},   // Bright green
      {CRGB(120, 255, 80), 0.10f}   // Lime green
    },
    5,
    "forest"
  },
  
  // Lavender field
  {
    {
      {CRGB(32, 0, 32), 0.20f},     // Deep purple
      {CRGB(80, 16, 80), 0.25f},    // Purple
      {CRGB(160, 80, 160), 0.25f},  // Light purple
      {CRGB(200, 120, 180), 0.20f}, // Pink-purple
      {CRGB(255, 180, 220), 0.10f}  // Light pink
    },
    5,
    "lavender"
  }
};

// Function declarations
void connectToWiFi();
void syncTime();
void fetchAlarms();
void parseAlarms(String jsonResponse);
void parseTime(String timeStr, int &hour, int &minute);
void checkAlarms();
void triggerSunriseAlarm(Alarm &alarm);
void runAdvancedSunriseAnimation(ColorPreset &preset, int duration_ms, int max_brightness);
void addSparkleEffect(CRGB baseColor, float intensity);
void addBreathingEffect(float progress, float &brightness_multiplier);
void addWarmthGradient(float progress);
void addWaveEffect(CRGB baseColor, float progress);
CRGB blendMultipleColors(SunriseStage stages[], int stageCount, float progress);
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
  for (int i = 0; i < sizeof(colorPresets) / sizeof(colorPresets[0]); i++) {
    if (colorPresets[i].name == alarm.colorPreset) {
      preset = colorPresets[i];
      break;
    }
  }
  
  runAdvancedSunriseAnimation(preset, alarm.duration * 60000, alarm.brightness);
}

void runAdvancedSunriseAnimation(ColorPreset &preset, int duration_ms, int max_brightness) {
  unsigned long startTime = millis();
  unsigned long lastUpdate = startTime;
  
  DEBUG_PRINTLN("Starting advanced sunrise animation...");
  
  while (millis() - startTime < duration_ms) {
    float progress = (float)(millis() - startTime) / (float)duration_ms;
    if (progress > 1.0) progress = 1.0;
    
    // Calculate current base color from multi-stage preset
    CRGB currentColor = blendMultipleColors(preset.stages, preset.stageCount, progress);
    
    // Apply breathing effect for more natural feel
    float breathingMultiplier = 1.0f;
    addBreathingEffect(progress, breathingMultiplier);
    
    // Calculate brightness with smooth curves
    float brightnessProgress = ease8InOutQuad(progress * 255) / 255.0f;
    int brightness = (int)(brightnessProgress * max_brightness * breathingMultiplier);
    FastLED.setBrightness(brightness);
    
    // Fill base color
    fill_solid(leds, NUM_LEDS, currentColor);
    
    // Add sparkle effect during middle phases
    if (progress > 0.3f && progress < 0.8f) {
      addSparkleEffect(currentColor, (progress - 0.3f) * 2.0f);
    }
    
    // Add warmth gradient effect during later phases
    if (progress > 0.5f) {
      addWarmthGradient(progress);
    }
    
    // Add subtle wave effect for ocean preset
    if (preset.name == "ocean") {
      addWaveEffect(currentColor, progress);
    }
    
    FastLED.show();
    
    // Variable update rate - slower at beginning, faster in middle, slower at end
    int updateDelay = (int)(50 + 450 * (1.0f - 4.0f * progress * (1.0f - progress)));
    delay(updateDelay);
    
    // Progress indicator every 10%
    if ((int)(progress * 10) > (int)(((float)(lastUpdate - startTime) / (float)duration_ms) * 10)) {
      DEBUG_PRINT("Animation progress: ");
      DEBUG_PRINT((int)(progress * 100));
      DEBUG_PRINTLN("%");
      lastUpdate = millis();
    }
  }
  
  DEBUG_PRINTLN("Main animation complete - entering daylight phase");
  
  // Daylight phase - keep LEDs on with subtle variations
  CRGB finalColor = preset.stages[preset.stageCount - 1].color;
  FastLED.setBrightness(max_brightness);
  
  for (int minute = 0; minute < 5; minute++) { // 5 minutes of daylight
    for (int second = 0; second < 60; second++) {
      // Subtle brightness variation to simulate natural light
      float variation = 0.95f + 0.1f * sin(millis() * 0.001f);
      FastLED.setBrightness((int)(max_brightness * variation));
      
      // Very subtle color temperature shift
      CRGB dayColor = finalColor;
      dayColor.r = min(255, (int)(dayColor.r * (0.98f + 0.04f * sin(millis() * 0.0005f))));
      
      fill_solid(leds, NUM_LEDS, dayColor);
      FastLED.show();
      delay(1000);
    }
    
    DEBUG_PRINT("Daylight phase: ");
    DEBUG_PRINT(minute + 1);
    DEBUG_PRINTLN("/5 minutes");
  }
  
  // Smooth fade out
  DEBUG_PRINTLN("Starting fade out...");
  for (int brightness = max_brightness; brightness >= 0; brightness -= 2) {
    FastLED.setBrightness(brightness);
    FastLED.show();
    delay(50);
  }
  
  FastLED.clear();
  FastLED.show();
  DEBUG_PRINTLN("Sunrise alarm completed");
}

CRGB blendMultipleColors(SunriseStage stages[], int stageCount, float progress) {
  if (progress <= 0.0f) return stages[0].color;
  if (progress >= 1.0f) return stages[stageCount - 1].color;
  
  float accumulated = 0.0f;
  for (int i = 0; i < stageCount - 1; i++) {
    float stageEnd = accumulated + stages[i].duration_percent;
    if (progress <= stageEnd) {
      float localProgress = (progress - accumulated) / stages[i].duration_percent;
      // Use smooth blending
      uint8_t blendAmount = ease8InOutQuad(localProgress * 255);
      return blend(stages[i].color, stages[i + 1].color, blendAmount);
    }
    accumulated = stageEnd;
  }
  return stages[stageCount - 1].color;
}

void addSparkleEffect(CRGB baseColor, float intensity) {
  // Add random sparkles that enhance the base color
  int sparkleCount = (int)(NUM_LEDS * 0.1f * intensity); // Up to 10% of LEDs can sparkle
  
  for (int i = 0; i < sparkleCount; i++) {
    int pos = random16(NUM_LEDS);
    if (random8() < 50) { // 50% chance for each potential sparkle
      // Create sparkle by brightening the base color
      CRGB sparkleColor = baseColor;
      sparkleColor.r = min(255, sparkleColor.r + random8(50, 100));
      sparkleColor.g = min(255, sparkleColor.g + random8(30, 70));
      sparkleColor.b = min(255, sparkleColor.b + random8(20, 50));
      leds[pos] = sparkleColor;
    }
  }
}

void addBreathingEffect(float progress, float &brightness_multiplier) {
  // Subtle breathing effect that's more pronounced in early phases
  float breathingIntensity = 1.0f - (progress * 0.7f); // Reduce effect as progress increases
  float breathingCycle = sin(millis() * 0.002f) * breathingIntensity * 0.1f; // Slow breathing
  brightness_multiplier = 1.0f + breathingCycle;
}

void addWarmthGradient(float progress) {
  // Add warm gradient from center outward during later phases
  int centerLed = NUM_LEDS / 2;
  float warmthIntensity = (progress - 0.5f) * 2.0f; // 0 to 1 from 50% to 100% progress
  
  for (int i = 0; i < NUM_LEDS; i++) {
    float distanceFromCenter = abs(i - centerLed) / (float)(NUM_LEDS / 2);
    float warmthFactor = 1.0f - (distanceFromCenter * warmthIntensity * 0.3f);
    
    leds[i].r = min(255, (int)(leds[i].r * (0.8f + warmthFactor * 0.4f)));
    leds[i].g = min(255, (int)(leds[i].g * (0.9f + warmthFactor * 0.2f)));
  }
}

void addWaveEffect(CRGB baseColor, float progress) {
  // Gentle wave effect for ocean preset
  unsigned long time = millis();
  for (int i = 0; i < NUM_LEDS; i++) {
    float wave1 = sin((i * 0.1f) + (time * 0.003f)) * 0.2f;
    float wave2 = sin((i * 0.05f) + (time * 0.002f)) * 0.1f;
    float waveEffect = (wave1 + wave2) * progress;
    
    leds[i].b = min(255, (int)(leds[i].b * (1.0f + waveEffect)));
    leds[i].g = min(255, (int)(leds[i].g * (1.0f + waveEffect * 0.5f)));
  }
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