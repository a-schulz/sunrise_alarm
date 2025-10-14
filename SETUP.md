# Sunrise Alarm Clock Setup Guide

## 🚀 Quick Start

### 1. Hardware Setup

**Required Components:**
- ESP32 development board
- WS2812B LED strip (addressable RGB LEDs)
- 5V power supply (adequate for your LED strip)
- Jumper wires

**Wiring:**
```
ESP32 Pin 4  ──► LED Strip Data Pin
ESP32 GND    ──► LED Strip GND
5V Supply    ──► LED Strip VCC
5V Supply    ──► ESP32 VIN (or use USB power)
```

### 2. Software Setup

**Prerequisites:**
- PlatformIO installed (comes with VS Code PlatformIO extension)
- Supabase account (free tier available)

**Steps:**

1. **Configure your settings:**
   ```bash
   cd /path/to/sunrise_alarm
   cp include/config.example.h include/config.h
   ```

2. **Edit `include/config.h` with your details:**
   ```cpp
   #define WIFI_SSID "YourWiFiName"
   #define WIFI_PASSWORD "YourWiFiPassword"
   #define SUPABASE_URL "https://your-project.supabase.co"
   #define SUPABASE_KEY "your_supabase_anon_key"
   ```

3. **Set up Supabase database:**
   - Create a new Supabase project
   - Run the SQL script from `database_setup.sql` in your Supabase SQL editor
   - Get your project URL and anon key from Settings > API

4. **Build and upload:**
   ```bash
   pio run -t upload
   ```

5. **Monitor serial output:**
   ```bash
   pio device monitor
   ```

### 3. Database Configuration

After running the database setup script, you'll have a table with sample alarms. Update the `device_id` field with your ESP32's MAC address (shown in serial monitor).

**Example alarm configuration:**
```sql
UPDATE alarms 
SET device_id = 'AA:BB:CC:DD:EE:FF'  -- Replace with your ESP32 MAC
WHERE id = 1;
```

## 🎨 Color Presets

The system includes four built-in color presets:

- **sunrise**: Deep red → Orange → Yellow → White
- **ocean**: Deep blue → Cyan → Light blue  
- **forest**: Dark green → Light green → Yellow-green
- **lavender**: Purple → Pink

## ⚙️ Configuration Options

### Hardware Settings
- `LED_PIN`: GPIO pin connected to LED strip (default: 4)
- `NUM_LEDS`: Number of LEDs in your strip (default: 60)
- `LED_TYPE`: LED chip type (default: WS2812B)

### Timing Settings  
- `GMT_OFFSET_SEC`: Timezone offset in seconds
- `DAYLIGHT_OFFSET_SEC`: Daylight saving offset
- `DEEP_SLEEP_DURATION`: Sleep time between alarm checks

### Alarm Settings
- `DEFAULT_SUNRISE_DURATION`: Default alarm duration in minutes
- `DEFAULT_BRIGHTNESS`: Default LED brightness (0-255)

## 🔧 Troubleshooting

### Common Issues

**WiFi Connection Failed:**
- Check SSID and password in config.h
- Ensure 2.4GHz WiFi (ESP32 doesn't support 5GHz)
- Check signal strength

**Time Sync Failed:**
- Verify internet connection
- Check timezone settings
- Try different NTP server

**Database Connection Issues:**
- Verify Supabase URL and API key
- Check Row Level Security policies
- Ensure device_id matches ESP32 MAC address

**LEDs Not Working:**
- Check wiring connections
- Verify power supply voltage and capacity
- Confirm LED_PIN and NUM_LEDS settings

### Debug Mode

Enable detailed logging by setting `DEBUG_MODE 1` in config.h. Monitor serial output at 115200 baud for troubleshooting information.

## 🔋 Power Management

The ESP32 uses deep sleep mode to conserve power:
- **Active time**: ~10 seconds every hour (or before next alarm)
- **Sleep current**: ~10µA in deep sleep mode
- **Total power**: Depends mainly on LED strip power consumption

## 📱 Managing Alarms

### Adding New Alarms

Use Supabase dashboard or SQL commands:

```sql
INSERT INTO alarms (device_id, time, days_of_week, brightness_level, duration_minutes, color_preset)
VALUES ('YOUR_MAC_ADDRESS', '07:00:00', ARRAY[1,2,3,4,5], 255, 30, 'sunrise');
```

### Day Codes
- 0 = Sunday
- 1 = Monday  
- 2 = Tuesday
- 3 = Wednesday
- 4 = Thursday
- 5 = Friday
- 6 = Saturday

### Disabling Alarms

Set `is_enabled = false` in the database:

```sql
UPDATE alarms SET is_enabled = false WHERE id = 1;
```

## 🚨 Safety Notes

- Use appropriate power supply for your LED strip
- Ensure proper electrical connections
- Consider heat dissipation for large LED installations
- Test with a small number of LEDs first

## 📞 Support

If you encounter issues:
1. Check the troubleshooting section above
2. Review serial monitor output with debug mode enabled
3. Verify hardware connections
4. Check Supabase database configuration

Happy waking up! 🌅