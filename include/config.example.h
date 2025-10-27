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
//   start = When DST starts (Mm.n.d format where m=month, n=week, d=day)
//           Week 5 means "last occurrence of that weekday in the month"
//           Day: 0=Sunday, 1=Monday, 2=Tuesday, 3=Wednesday, 4=Thursday, 5=Friday, 6=Saturday
//           Example: M3.5.0 = Last Sunday (day 0) of March (month 3)
//   end = When DST ends (same format as start)
#define TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"

// Power Management
#define DEEP_SLEEP_DURATION 3600000000ULL // 1 hour in microseconds
#define ALARM_CHECK_INTERVAL 60000000ULL  // 1 minute in microseconds

// Alarm Configuration
#define MAX_ALARMS 10
#define DEFAULT_SUNRISE_DURATION 30 // minutes
#define DEFAULT_BRIGHTNESS 255

// Debug Mode
#define ENABLE_DEBUG_MODE 1

// OTA Configuration
#define OTA_HOSTNAME "sunrise-alarm"
#define OTA_PASSWORD "super strong password"
#define OTA_PORT 3232

// Web Server Configuration
#define WEB_SERVER_PORT 80
#define ENABLE_WEB_LOGS 1
#define MAX_LOG_ENTRIES 100

#if ENABLE_DEBUG_MODE
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

#endif // CONFIG_H