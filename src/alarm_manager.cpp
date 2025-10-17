#include "alarm_manager.h"
#include "led_controller.h"
#include "config.h"
#include <ArduinoJson.h>
#include <time.h>
#include <logger.h>
#include <network_manager.h>
#include <database.h>

Alarm AlarmManager::alarms[10];
int AlarmManager::alarm_count = 0;

ColorPreset AlarmManager::color_presets[] = {
    {{{CRGB(32, 0, 0), 0.15f},
      {CRGB(80, 8, 0), 0.25f},
      {CRGB(160, 32, 0), 0.35f},
      {CRGB(255, 80, 16), 0.20f},
      {CRGB(255, 180, 80), 0.15f},
      {CRGB(255, 220, 180), 0.10f}},
     6,
     "sunrise"},
    {{{CRGB(0, 8, 32), 0.20f},
      {CRGB(0, 32, 80), 0.25f},
      {CRGB(0, 80, 160), 0.25f},
      {CRGB(32, 160, 255), 0.20f},
      {CRGB(80, 200, 255), 0.10f}},
     5,
     "ocean"},
    {{{CRGB(8, 16, 0), 0.25f},
      {CRGB(16, 40, 8), 0.25f},
      {CRGB(40, 80, 16), 0.25f},
      {CRGB(80, 160, 40), 0.15f},
      {CRGB(120, 255, 80), 0.10f}},
     5,
     "forest"},
    {{{CRGB(32, 0, 32), 0.20f},
      {CRGB(80, 16, 80), 0.25f},
      {CRGB(160, 80, 160), 0.25f},
      {CRGB(200, 120, 180), 0.20f},
      {CRGB(255, 180, 220), 0.10f}},
     5,
     "lavender"}};

int AlarmManager::color_preset_count = 4;

void AlarmManager::fetch_alarms_from_db()
{
    if (!NetworkManager::wifi_connected)
    {
        WEB_LOG("WiFi not connected, cannot fetch alarms");
        return;
    }

    WEB_LOG("Fetching alarms from Supabase...");
    String result = Database::db.from("alarms")
                        .select("*")
                        .eq("device_id", NetworkManager::get_device_id())
                        .eq("is_enabled", "true")
                        .doSelect();

    if (result.length() > 0 && !result.startsWith("error"))
    {
        WEB_LOG("Alarms fetched successfully");
        parse_alarms(result);
        return;
    }
    else
    {
        WEB_LOG("Supabase Error: " + result);
        return;
    }
}

void AlarmManager::parse_alarms(const String &json_response)
{
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json_response);

    if (error)
    {
        DEBUG_PRINT("JSON parsing error: ");
        DEBUG_PRINTLN(error.c_str());
        return;
    }

    alarm_count = 0;
    JsonArray alarms_array = doc.as<JsonArray>();

    for (JsonObject alarm : alarms_array)
    {
        if (alarm_count >= MAX_ALARMS)
            break;

        alarms[alarm_count].id = alarm["id"];
        String time_str = alarm["time"];
        parse_time(time_str, alarms[alarm_count].hour, alarms[alarm_count].minute);

        JsonArray days = alarm["days_of_week"];
        for (int i = 0; i < 7; i++)
        {
            alarms[alarm_count].days_of_week[i] = false;
        }
        for (JsonVariant day : days)
        {
            int day_num = day.as<int>();
            if (day_num >= 0 && day_num < 7)
            {
                alarms[alarm_count].days_of_week[day_num] = true;
            }
        }

        alarms[alarm_count].enabled = alarm["is_enabled"];
        alarms[alarm_count].brightness = alarm["brightness_level"] | DEFAULT_BRIGHTNESS;
        alarms[alarm_count].duration = alarm["duration_minutes"] | DEFAULT_SUNRISE_DURATION;
        alarms[alarm_count].color_preset = alarm["color_preset"] | "sunrise";

        DEBUG_PRINT("Loaded alarm: ");
        DEBUG_PRINT(alarms[alarm_count].hour);
        DEBUG_PRINT(":");
        DEBUG_PRINTLN(alarms[alarm_count].minute);

        alarm_count++;
    }

    DEBUG_PRINT("Total alarms loaded: ");
    DEBUG_PRINTLN(alarm_count);
}

void AlarmManager::check_alarms()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        DEBUG_PRINTLN("Failed to get current time");
        return;
    }

    int current_hour = timeinfo.tm_hour;
    int current_minute = timeinfo.tm_min;
    int current_weekday = timeinfo.tm_wday;

    DEBUG_PRINT("Checking alarms at ");
    DEBUG_PRINT(current_hour);
    DEBUG_PRINT(":");
    DEBUG_PRINTLN(current_minute);

    for (int i = 0; i < alarm_count; i++)
    {
        if (alarms[i].enabled &&
            alarms[i].hour == current_hour &&
            alarms[i].minute == current_minute &&
            alarms[i].days_of_week[current_weekday])
        {

            DEBUG_PRINTLN("Alarm triggered! Starting sunrise simulation...");
            trigger_sunrise_alarm(alarms[i]);
            return;
        }
    }

    DEBUG_PRINTLN("No alarms to trigger");
}

time_t AlarmManager::calculate_next_alarm_time()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        DEBUG_PRINTLN("Failed to get time for next wake calculation");
        return DEEP_SLEEP_DURATION / 1000000;
    }

    time_t now = mktime(&timeinfo);
    time_t next_alarm = now + DEEP_SLEEP_DURATION / 1000000;

    for (int i = 0; i < alarm_count; i++)
    {
        if (!alarms[i].enabled)
            continue;

        for (int day = 0; day < 8; day++)
        {
            struct tm alarm_time = timeinfo;
            alarm_time.tm_mday += day;
            alarm_time.tm_hour = alarms[i].hour;
            alarm_time.tm_min = alarms[i].minute;
            alarm_time.tm_sec = 0;

            mktime(&alarm_time);

            if (alarms[i].days_of_week[alarm_time.tm_wday])
            {
                time_t alarm_timestamp = mktime(&alarm_time);
                if (alarm_timestamp > now && alarm_timestamp < next_alarm)
                {
                    next_alarm = alarm_timestamp;
                }
            }
        }
    }

    DEBUG_PRINT("Next wake in ");
    DEBUG_PRINT((next_alarm - now));
    DEBUG_PRINTLN(" seconds");

    return next_alarm - now;
}

void AlarmManager::parse_time(const String &time_str, int &hour, int &minute)
{
    int colon_index = time_str.indexOf(':');
    if (colon_index > 0)
    {
        hour = time_str.substring(0, colon_index).toInt();
        minute = time_str.substring(colon_index + 1).toInt();
    }
    else
    {
        hour = 0;
        minute = 0;
    }
}

void AlarmManager::trigger_sunrise_alarm(Alarm &alarm)
{
    DEBUG_PRINT("Starting sunrise alarm with preset: ");
    DEBUG_PRINTLN(alarm.color_preset);

    ColorPreset *preset = find_color_preset(alarm.color_preset);
    if (preset == nullptr)
    {
        preset = &color_presets[0];
    }

    LEDController::run_sunrise_animation(*preset, alarm.duration * 60000, alarm.brightness);
}

ColorPreset *AlarmManager::find_color_preset(const String &name)
{
    for (int i = 0; i < color_preset_count; i++)
    {
        if (color_presets[i].name == name)
        {
            return &color_presets[i];
        }
    }
    return nullptr;
}
