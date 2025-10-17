#include "network_manager.h"
#include "led_controller.h"
#include "logger.h"
#include "config.h"

Supabase NetworkManager::db;
bool NetworkManager::wifi_connected = false;
bool NetworkManager::ota_initialized = false;

void NetworkManager::init() {
    db.begin(SUPABASE_URL, SUPABASE_KEY);
}

bool NetworkManager::connect_wifi() {
    WEB_LOG("Connecting to WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        DEBUG_PRINT(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        wifi_connected = true;
        WEB_LOG("WiFi connected!");
        WEB_LOG("IP address: " + WiFi.localIP().toString());
        WEB_LOG("MAC address: " + WiFi.macAddress());
        return true;
    } else {
        wifi_connected = false;
        WEB_LOG("WiFi connection failed!");
        return false;
    }
}

void NetworkManager::disconnect_wifi() {
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    wifi_connected = false;
}

void NetworkManager::setup_ota() {
    WEB_LOG("Setting up OTA updates...");
    
    ArduinoOTA.setHostname(OTA_HOSTNAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);
    
    ArduinoOTA.onStart([]() {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        WEB_LOG("OTA Start updating " + type);
        LEDController::init();
        LEDController::clear();
    });
    
    ArduinoOTA.onEnd([]() {
        WEB_LOG("OTA Update completed");
    });
    
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        LEDController::show_ota_progress(progress, total);
    });
    
    ArduinoOTA.onError([](ota_error_t error) {
        String error_msg = "OTA Error[" + String(error) + "]: ";
        if (error == OTA_AUTH_ERROR) error_msg += "Auth Failed";
        else if (error == OTA_BEGIN_ERROR) error_msg += "Begin Failed";
        else if (error == OTA_CONNECT_ERROR) error_msg += "Connect Failed";
        else if (error == OTA_RECEIVE_ERROR) error_msg += "Receive Failed";
        else if (error == OTA_END_ERROR) error_msg += "End Failed";
        WEB_LOG(error_msg);
        LEDController::show_ota_error();
    });
    
    ArduinoOTA.begin();
    ota_initialized = true;
    WEB_LOG("OTA Ready - Hostname: " + String(OTA_HOSTNAME));
}

void NetworkManager::handle_ota() {
    if (ota_initialized) {
        ArduinoOTA.handle();
    }
}

void NetworkManager::sync_time() {
    WEB_LOG("Syncing time with NTP server...");
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
    
    struct tm timeinfo;
    int attempts = 0;
    while (!getLocalTime(&timeinfo) && attempts < 10) {
        delay(1000);
        attempts++;
    }
    
    if (attempts < 10) {
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%A, %B %d %Y %H:%M:%S", &timeinfo);
        WEB_LOG("Time synchronized: " + String(time_str));
    } else {
        WEB_LOG("Failed to sync time");
    }
}

String NetworkManager::fetch_alarms_from_db() {
    if (!wifi_connected) {
        WEB_LOG("WiFi not connected, skipping alarm fetch");
        return "";
    }
    
    WEB_LOG("Fetching alarms from Supabase...");
    String result = db.from("alarms")
                      .select("*")
                      .eq("device_id", get_device_id())
                      .eq("is_enabled", "true")
                      .doSelect();
    
    if (result.length() > 0 && !result.startsWith("error")) {
        WEB_LOG("Alarms fetched successfully");
        return result;
    } else {
        WEB_LOG("Supabase Error: " + result);
        return "";
    }
}

String NetworkManager::get_device_id() {
    return WiFi.macAddress();
}
