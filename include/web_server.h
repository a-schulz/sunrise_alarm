#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESPAsyncWebServer.h>
#include <Arduino.h>

class WebServerManager {
public:
    static void init();
    static void track_activity();
    static bool has_recent_activity();
    static unsigned long get_last_activity_time();
    
private:
    static AsyncWebServer* server;
    static unsigned long last_web_request;
    static bool initialized;
    
    static void setup_routes();
    static String build_dashboard_html();
    static String build_logs_html();
};

#endif
