#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <WiFi.h>
#include <ArduinoOTA.h>
#include <ESPSupabase.h>
#include <Arduino.h>

class NetworkManager {
public:
    static void init();
    static bool connect_wifi();
    static void disconnect_wifi();
    static void setup_ota();
    static void handle_ota();
    static void sync_time();
    static String fetch_alarms_from_db();
    static String get_device_id();
    
private:
    static Supabase db;
    static bool wifi_connected;
    static bool ota_initialized;
};

#endif
