#include "web_server.h"
#include "logger.h"
#include "led_controller.h"
#include "network_manager.h"
#include "alarm_manager.h"
#include "config.h"
#include <WiFi.h>

AsyncWebServer *WebServerManager::server = nullptr;
unsigned long WebServerManager::last_web_request = 0;
bool WebServerManager::initialized = false;

void WebServerManager::init()
{
    if (initialized)
        return;

    server = new AsyncWebServer(WEB_SERVER_PORT);
    setup_routes();
    server->begin();
    initialized = true;
    WEB_LOG("Web server started on http://" + WiFi.localIP().toString());
}

void WebServerManager::track_activity()
{
    last_web_request = millis();
}

bool WebServerManager::has_recent_activity()
{
    return (millis() - last_web_request < 60000);
}

unsigned long WebServerManager::get_last_activity_time()
{
    return last_web_request;
}

void WebServerManager::setup_routes()
{
    server->on("/", HTTP_GET, [](AsyncWebServerRequest *request)
               {
        track_activity();
        request->send(200, "text/html", build_dashboard_html()); });

    server->on("/logs", HTTP_GET, [](AsyncWebServerRequest *request)
               {
        track_activity();
        request->send(200, "text/html", build_logs_html()); });

    server->on("/test", HTTP_GET, [](AsyncWebServerRequest *request)
               {
        track_activity();
        WEB_LOG("LED test triggered via web");
        LEDController::run_test_animation();
        request->send(200, "text/plain", "LED test completed!"); });

    server->on("/sync", HTTP_GET, [](AsyncWebServerRequest *request)
               {
        track_activity();
        WEB_LOG("Manual sync triggered via web");
        AlarmManager::fetch_alarms_from_db();
            request->send(200, "text/plain", "Alarm sync started. Check logs for details!"); });

    server->on("/alarm/dismiss", HTTP_GET, [](AsyncWebServerRequest *request)
               {
        track_activity();
        if (LEDController::is_alarm_running()) {
            WEB_LOG("Alarm dismissed via web interface");
            LEDController::dismiss_alarm();
            request->send(200, "text/plain", "Alarm dismissed!");
        } else {
            WEB_LOG("Dismiss requested but no alarm is running");
            request->send(200, "text/plain", "No alarm is currently running.");
        } });
}

String WebServerManager::build_dashboard_html()
{
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<title>Sunrise Alarm</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>body{font-family:Arial;margin:20px;background:#f0f0f0}";
    html += ".card{background:white;padding:20px;margin:10px 0;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1)}";
    html += ".btn{background:#007bff;color:white;padding:10px 20px;border:none;border-radius:4px;cursor:pointer;margin:5px}";
    html += ".btn:hover{background:#0056b3}";
    html += ".btn-danger{background:#dc3545}";
    html += ".btn-danger:hover{background:#c82333}";
    html += ".status{padding:10px;border-radius:4px;margin:10px 0}";
    html += ".success{background:#d4edda;color:#155724;border:1px solid #c3e6cb}";
    html += ".info{background:#d1ecf1;color:#0c5460;border:1px solid #bee5eb}";
    html += ".warning{background:#fff3cd;color:#856404;border:1px solid #ffeaa7}</style></head><body>";

    html += "<h1>🌅 Sunrise Alarm Control</h1>";

    html += "<div class='card'><h2>System Status</h2>";
    html += "<div class='status info'>Device: " + String(OTA_HOSTNAME) + "</div>";
    html += "<div class='status info'>IP: " + WiFi.localIP().toString() + "</div>";
    html += "<div class='status info'>MAC: " + WiFi.macAddress() + "</div>";

    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%A, %B %d %Y %H:%M:%S", &timeinfo);
        html += "<div class='status success'>Time: " + String(time_str) + "</div>";
    }

    html += "<div class='status info'>Free Heap: " + String(ESP.getFreeHeap()) + " bytes</div>";
    html += "<div class='status info'>Alarms Loaded: " + String(AlarmManager::get_alarm_count()) + "</div>";
    html += "</div>";

    html += "<div class='card'><h2>Controls</h2>";
    
    if (LEDController::is_alarm_running()) {
        html += "<div class='status warning'>⚠️ Alarm is currently running!</div>";
        html += "<button class='btn btn-danger' onclick=\"if(confirm('Dismiss the current alarm?')) location.href='/alarm/dismiss'\">❌ Dismiss Alarm</button><br>";
    }
    
    html += "<button class='btn' onclick=\"location.href='/logs'\">📋 View Logs</button>";
    html += "<button class='btn' onclick=\"location.href='/test'\">🌈 Test LEDs</button>";
    html += "<button class='btn' onclick=\"location.href='/sync'\">🔄 Sync Alarms</button>";
    html += "</div>";

    html += "</body></html>";
    return html;
}

String WebServerManager::build_logs_html()
{
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<title>System Logs</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<meta http-equiv='refresh' content='5'>";
    html += "<style>body{font-family:monospace;margin:20px;background:#000;color:#0f0}";
    html += ".log{padding:2px 0;border-bottom:1px solid #333}</style></head><body>";
    html += "<h2>📋 System Logs (Auto-refresh: 5s)</h2>";
    html += "<a href='/' style='color:#0ff'>← Back to Dashboard</a><br><br>";

    html += Logger::getLogsHtml();

    html += "</body></html>";
    return html;
}
