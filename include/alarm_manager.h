#ifndef ALARM_MANAGER_H
#define ALARM_MANAGER_H

#include <Arduino.h>
#include <FastLED.h>

struct Alarm
{
    int id;
    int hour;
    int minute;
    bool days_of_week[7];
    bool enabled;
    int brightness;
    int duration;
    String color_preset;
};

struct SunriseStage
{
    CRGB color;
    float duration_percent;
};

struct ColorPreset
{
    SunriseStage stages[6];
    int stage_count;
    String name;
};

class AlarmManager
{
public:
    static void fetch_alarms_from_db();
    static void parse_alarms(const String &json_response);
    static void check_alarms();
    static bool has_alarms() { return alarm_count > 0; }
    static int get_alarm_count() { return alarm_count; }
    static time_t calculate_next_alarm_time();

private:
    static Alarm alarms[10];
    static int alarm_count;
    static ColorPreset color_presets[];
    static int color_preset_count;

    static void parse_time(const String &time_str, int &hour, int &minute);
    static void trigger_sunrise_alarm(Alarm &alarm);
    static ColorPreset *find_color_preset(const String &name);
};

#endif
