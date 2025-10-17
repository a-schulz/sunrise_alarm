#include <Arduino.h>
#include "config.h"
#include "logger.h"
#include "network_manager.h"
#include "alarm_manager.h"
#include "led_controller.h"
#include "web_server.h"
#include "database.h"

RTC_DATA_ATTR int boot_count = 0;

volatile bool button_pressed = false;
unsigned long last_button_press = 0;
unsigned long boot_time = 0;

const unsigned long OTA_WINDOW_DURATION = 300000;

void IRAM_ATTR button_isr();
void setup_button();
bool should_stay_awake();
void enter_deep_sleep();

void setup()
{
  Serial.begin(115200);
  delay(100);

  boot_time = millis();
  boot_count++;

  Logger::init(MAX_LOG_ENTRIES);
  WEB_LOG("=== Sunrise Alarm Clock Starting ===");
  WEB_LOG("Boot count: " + String(boot_count));

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0)
  {
    WEB_LOG("Woke up from button press (EXT0)");
    button_pressed = true;
  }
  else if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER)
  {
    WEB_LOG("Woke up from timer");
  }
  else
  {
    WEB_LOG("Woke up from other reason: " + String(wakeup_reason));
  }

  setup_button();
  Database::init();
  if (NetworkManager::connect_wifi())
  {
    if (boot_count == 1)
    {
      NetworkManager::setup_ota();
      WebServerManager::init();
    }

    NetworkManager::sync_time();

    AlarmManager::fetch_alarms_from_db();

    AlarmManager::check_alarms();
  }

  if (!should_stay_awake())
  {
    enter_deep_sleep();
  }

  WEB_LOG("Staying awake for OTA/maintenance window");
}

void loop()
{
  if (boot_count == 1)
  {
    NetworkManager::handle_ota();
  }

  if (button_pressed)
  {
    if (millis() - last_button_press > BUTTON_DEBOUNCE_MS)
    {
      WEB_LOG("Button pressed - Manual sync triggered");
      LEDController::show_button_feedback();
      AlarmManager::fetch_alarms_from_db();
      button_pressed = false;
      WEB_LOG("Aborting OTA and Webserver - preparing for sleep");
      enter_deep_sleep();
    }
  }

  if (!should_stay_awake())
  {
    WEB_LOG("OTA window expired - preparing for sleep");
    enter_deep_sleep();
  }

  static unsigned long last_status_update = 0;
  if (millis() - last_status_update > 5000)
  {
    last_status_update = millis();
    LEDController::show_status_indicator();

    String reason = "Unknown";
    if (boot_count == 1 && WebServerManager::has_recent_activity())
    {
      unsigned long time_left = 60000 - (millis() - WebServerManager::get_last_activity_time());
      reason = "Recent web activity (expires in " + String(time_left / 1000) + "s)";
    }
    else if (boot_count == 1 && millis() - boot_time < OTA_WINDOW_DURATION)
    {
      unsigned long time_left = OTA_WINDOW_DURATION - (millis() - boot_time);
      reason = "OTA window (expires in " + String(time_left / 1000) + "s)";
    }
    else
    {
      reason = "Unknown reason";
    }

    WEB_LOG("Staying awake: " + reason);
  }

  delay(100);
}

void setup_button()
{
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), button_isr, FALLING);
}

void IRAM_ATTR button_isr()
{
  button_pressed = true;
  last_button_press = millis();
}

bool should_stay_awake()
{
  if (button_pressed)
  {
    WEB_LOG("Aborting OTA and Webserver - preparing for sleep");
    return false;
  }

  if (WebServerManager::has_recent_activity())
  {
    return true;
  }

  if (boot_count == 1 && millis() - boot_time < OTA_WINDOW_DURATION)
  {
    return true;
  }

  return false;
}

void enter_deep_sleep()
{
  WEB_LOG("Entering deep sleep...");
  WEB_LOG("Disconnecting WiFi...");
  NetworkManager::disconnect_wifi();
  WEB_LOG("Clearing LEDs...");
  LEDController::clear();

  time_t next_wake_seconds = AlarmManager::calculate_next_alarm_time();
  uint64_t sleep_duration = min((uint64_t)next_wake_seconds * 1000000ULL, DEEP_SLEEP_DURATION);

  WEB_LOG("Sleep duration: " + String(sleep_duration / 1000000) + " seconds");

  esp_sleep_enable_timer_wakeup(sleep_duration);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);

  delay(100);
  esp_deep_sleep_start();
}