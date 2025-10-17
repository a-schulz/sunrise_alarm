#include <Arduino.h>
#include "config.h"
#include "logger.h"
#include "network_manager.h"
#include "alarm_manager.h"
#include "led_controller.h"
#include "web_server.h"

RTC_DATA_ATTR int boot_count = 0;
RTC_DATA_ATTR bool button_pressed = false;

volatile bool button_flag = false;
unsigned long last_button_press = 0;
unsigned long boot_time = 0;

const unsigned long OTA_WINDOW_DURATION = 300000;

void IRAM_ATTR button_isr();
void setup_button();
void handle_button_press();
bool should_stay_awake();
void enter_deep_sleep();

void setup() {
    Serial.begin(115200);
    delay(100);
    
    boot_time = millis();
    boot_count++;
    
    Logger::init(MAX_LOG_ENTRIES);
    WEB_LOG("=== Sunrise Alarm Clock Starting ===");
    WEB_LOG("Boot count: " + String(boot_count));
    
    setup_button();
    
    if (button_flag) {
        button_pressed = true;
        button_flag = false;
    }
    
    NetworkManager::init();
    if (NetworkManager::connect_wifi()) {
        if (boot_count == 1 || button_pressed) {
            NetworkManager::setup_ota();
            WebServerManager::init();
        }
        
        NetworkManager::sync_time();
        
        if (button_pressed) {
            handle_button_press();
            button_pressed = false;
        }
        
        String alarm_data = NetworkManager::fetch_alarms_from_db();
        if (alarm_data.length() > 0) {
            AlarmManager::parse_alarms(alarm_data);
        }
        
        AlarmManager::check_alarms();
    }
    
    if (!should_stay_awake()) {
        enter_deep_sleep();
    }
    
    WEB_LOG("Staying awake for OTA/maintenance window");
}

void loop() {
    NetworkManager::handle_ota();
    
    if (button_flag) {
        if (millis() - last_button_press > BUTTON_DEBOUNCE_MS) {
            handle_button_press();
            button_flag = false;
        }
    }
    
    if (!should_stay_awake()) {
        WEB_LOG("OTA window expired - preparing for sleep");
        enter_deep_sleep();
    }
    
    static unsigned long last_status_update = 0;
    if (millis() - last_status_update > 5000) {
        last_status_update = millis();
        LEDController::show_status_indicator();
        
        String reason = "Unknown";
        if (button_pressed) {
            reason = "Button press";
        } else if (WebServerManager::has_recent_activity()) {
            unsigned long time_left = 60000 - (millis() - WebServerManager::get_last_activity_time());
            reason = "Web activity (expires in " + String(time_left / 1000) + "s)";
        } else if (boot_count == 1 && millis() - boot_time < OTA_WINDOW_DURATION) {
            unsigned long time_left = OTA_WINDOW_DURATION - (millis() - boot_time);
            reason = "OTA window (expires in " + String(time_left / 1000) + "s)";
        }
        
        WEB_LOG("Staying awake: " + reason);
    }
    
    delay(100);
}

void setup_button() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), button_isr, FALLING);
}

void IRAM_ATTR button_isr() {
    button_flag = true;
    last_button_press = millis();
}

void handle_button_press() {
    WEB_LOG("Button pressed - Manual sync triggered");
    LEDController::show_button_feedback();
    
    String alarm_data = NetworkManager::fetch_alarms_from_db();
    if (alarm_data.length() > 0) {
        AlarmManager::parse_alarms(alarm_data);
    }
    
    WEB_LOG("Manual sync completed");
}

bool should_stay_awake() {
    if (button_pressed || button_flag) {
        return true;
    }
    
    if (WebServerManager::has_recent_activity()) {
        return true;
    }
    
    if (boot_count == 1 && millis() - boot_time < OTA_WINDOW_DURATION) {
        return true;
    }
    
    return false;
}

void enter_deep_sleep() {
    WEB_LOG("Entering deep sleep...");
    
    button_pressed = false;
    button_flag = false;
    
    NetworkManager::disconnect_wifi();
    LEDController::clear();
    
    time_t next_wake_seconds = AlarmManager::calculate_next_alarm_time();
    uint64_t sleep_duration = min((uint64_t)next_wake_seconds * 1000000ULL, DEEP_SLEEP_DURATION);
    
    WEB_LOG("Sleep duration: " + String(sleep_duration / 1000000) + " seconds");
    
    esp_sleep_enable_timer_wakeup(sleep_duration);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
    
    delay(100);
    esp_deep_sleep_start();
}