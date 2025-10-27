#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration - Replace with your WiFi credentials
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"

// Supabase Configuration - Replace with your Supabase project details
#define SUPABASE_URL "https://your-project.supabase.co"
#define SUPABASE_KEY "your_supabase_anon_key"

// Hardware Configuration
#define LED_PIN 4
#define NUM_LEDS 60
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

// Button Configuration
#define BUTTON_PIN 0 // GPIO0 (BOOT button on most ESP32 boards)
#define BUTTON_DEBOUNCE_MS 50

// NTP Configuration
#define NTP_SERVER "pool.ntp.org"

// Timezone Configuration (POSIX timezone string format)
// This automatically handles Daylight Saving Time (DST) transitions
// Examples for common timezones:
//   Central European Time (CET/CEST - Germany, France, Italy, etc.):
//     "CET-1CEST,M3.5.0,M10.5.0/3"
//   Eastern European Time (EET/EEST - Greece, Romania, etc.):
//     "EET-2EEST,M3.5.0/3,M10.5.0/4"
//   UTC (no DST):
//     "UTC0"
//   US Pacific Time (PST/PDT):
//     "PST8PDT,M3.2.0,M11.1.0"
//   US Eastern Time (EST/EDT):
//     "EST5EDT,M3.2.0,M11.1.0"
//   UK (GMT/BST):
//     "GMT0BST,M3.5.0/1,M10.5.0"
//
// Format: STD offset DST [,start[/time],end[/time]]
//   STD = Standard time zone name
//   offset = Hours offset from UTC (positive is west, negative is east)
//   DST = Daylight saving time zone name
//   start = When DST starts (Mm.n.d = month.week.day, M3.5.0 = March, last Sunday)
//   end = When DST ends
#define TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"

// Power Management
#define DEEP_SLEEP_DURATION 3600000000ULL // 1 hour in microseconds
#define ALARM_CHECK_INTERVAL 60000000ULL  // 1 minute in microseconds

// Alarm Configuration
#define MAX_ALARMS 10
#define DEFAULT_SUNRISE_DURATION 30 // minutes
#define DEFAULT_BRIGHTNESS 255

// Debug Mode
#define DEBUG_MODE 1

#if DEBUG_MODE
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

#endif // CONFIG_H