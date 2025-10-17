#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <WiFi.h>
#include <ArduinoOTA.h>
#include <ESPSupabase.h>
#include <Arduino.h>

class NetworkManager
{
public:
    static bool connect_wifi();
    static void disconnect_wifi();
    static void setup_ota();
    static void handle_ota();
    static void sync_time();
    static String get_device_id();

    static bool wifi_connected;

private:
    static bool ota_initialized;
};

#endif
