# 🌅 Sunrise Alarm Clock

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![ESP32](https://img.shields.io/badge/Platform-ESP32-blue.svg)](https://www.espressif.com/en/products/socs/esp32)
[![WS2812B](https://img.shields.io/badge/LEDs-WS2812B-green.svg)](https://www.adafruit.com/category/168)
[![ESPSupabase](https://img.shields.io/badge/Library-ESPSupabase-orange.svg)](https://github.com/jhagas/ESPSupabase)

> A smart sunrise simulation alarm clock that gradually increases LED brightness to wake you up naturally, powered by ESP32 and WS2812B LED strips with cloud-based alarm management.

## ✨ Features

- 🌄 **Gradual sunrise simulation** - Smooth LED brightness transition mimicking natural sunrise
- 🔌 **Low power consumption** - Utilizes ESP32 deep sleep mode for energy efficiency
- 📡 **WiFi connectivity** - Automatic time synchronization with NTP servers
- 🗄️ **Cloud-based alarms** - Alarm configurations stored and retrieved from Supabase database
- 🔘 **Manual sync button** - Press ESP32 boot button to force sync with database
- ⏰ **Flexible scheduling** - Support for human-readable alarm patterns
- 🎨 **Customizable colors** - Full RGB spectrum control with WS2812B LED strips
- 🔒 **Secure access** - Row Level Security (RLS) policies protect device data

## 🏗️ Architecture

```
┌─────────────────┐    WiFi     ┌─────────────────┐
│     ESP32       │◄──────────►│   NTP Server    │
│   (Button GPIO0)│             │  (Time Sync)    │
└─────┬───────────┘             └─────────────────┘
      │
      │ Data Pin                          ┌─────────────────┐
      ▼                    ESPSupabase    │   Supabase DB   │
┌─────────────────┐          ◄──────────►│ (Alarm Config)  │
│   WS2812B LED   │                      │   + RLS Rules   │
│     Strip       │                      └─────────────────┘
└─────────────────┘
```

## 🛠️ Hardware Requirements

| Component                   | Description                               | Quantity |
| --------------------------- | ----------------------------------------- | -------- |
| **ESP32**                   | Main microcontroller with WiFi capability | 1        |
| **WS2812B LED Strip**       | Addressable RGB LED strip                 | 1        |
| **5V Power Supply**         | For ESP32 and LED strip                   | 1        |
| **Jumper Cables**           | For connections                           | Several  |
| **Breadboard** _(optional)_ | For prototyping                           | 1        |

### Wiring Diagram

```
ESP32 Pin 4  ──► LED Strip Data Pin
ESP32 GND    ──► LED Strip GND
5V Supply    ──► LED Strip VCC
5V Supply    ──► ESP32 VIN (or use USB power)
GPIO0        ──► Built-in BOOT button (manual sync)
```

## 📋 Software Architecture

### Database Schema (Supabase)

```sql
CREATE TABLE alarms (
    id SERIAL PRIMARY KEY,
    device_id VARCHAR(255) NOT NULL,
    time TIME NOT NULL,
    days_of_week INTEGER[] NOT NULL, -- [1,2,3,4,5] for weekdays
    is_enabled BOOLEAN DEFAULT true,
    brightness_level INTEGER DEFAULT 255,
    duration_minutes INTEGER DEFAULT 30,
    color_preset VARCHAR(50) DEFAULT 'sunrise',
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);
```

### Row Level Security (RLS) Rules

The database uses the following RLS policies for secure device access:

1. **Device Access Policy**: Allows devices to access only their own alarms using device_id
2. **API Key Access Policy**: Enables ESP32 devices to use the Supabase anon key securely

### Alarm Schedule Format

```json
{
  "time": "07:30",
  "days": [1, 2, 3, 4, 5], // Monday to Friday
  "enabled": true,
  "brightness": 255,
  "duration": 30, // minutes
  "color_preset": "sunrise"
}
```

## 🚀 Getting Started

### Prerequisites

- **PlatformIO** (recommended) or **Arduino IDE**
- **ESP32 board support** installed
- **Supabase account** (free tier available)
- **Required libraries** (automatically installed):
  - `ESPSupabase` by jhagas (v0.1.0)
  - `FastLED` (v3.6.0+)
  - `ArduinoJson` (v6.21.3+)

### Installation

1. **Clone the repository**

   ```bash
   git clone https://github.com/yourusername/sunrise-alarm.git
   cd sunrise-alarm
   ```

2. **Install dependencies**

   ```bash
   pio lib install  # PlatformIO will install all dependencies automatically
   ```

3. **Configure settings**

   ```bash
   cp include/config.example.h include/config.h
   ```

   Edit `include/config.h` with your WiFi and Supabase credentials

4. **Set up Supabase database**

   - Create a new Supabase project
   - Run the SQL script from `database_setup.sql` in your Supabase SQL editor
   - Get your project URL and anon key from Settings > API

5. **Upload to ESP32**
   ```bash
   pio run -t upload
   pio device monitor  # Monitor serial output
   ```

## ⚙️ Configuration

Create/edit `include/config.h` with your settings:

```cpp
// WiFi Configuration
#define WIFI_SSID "YourWiFiName"
#define WIFI_PASSWORD "YourWiFiPassword"

// Supabase Configuration
#define SUPABASE_URL "https://your-project.supabase.co"
#define SUPABASE_KEY "your_supabase_anon_key"

// Hardware Configuration
#define LED_PIN 4
#define NUM_LEDS 60
#define BUTTON_PIN 0  // GPIO0 (BOOT button)
```

## 🔄 Over-The-Air (OTA) Updates

### ⚠️ Important: OTA Availability

**OTA updates are ONLY available during:**

| **Scenario**         | **OTA Available** | **Duration** |
| -------------------- | ----------------- | ------------ |
| **Initial Power-On** | ✅ Yes            | 5 minutes    |
| **During Alarm**     | ❌ No             | N/A          |
| **Deep Sleep Wake**  | ❌ No             | N/A          |
| **Button Press**     | ❌ No             | N/A          |

**Why this design?**

- **Battery efficiency**: OTA is only available during the initial boot window
- **Predictable updates**: You control exactly when OTA is available by power cycling
- **No interruptions**: Alarms run without OTA overhead

### How to Perform OTA Updates

#### Power-On Boot Method

1. **Power cycle the device** (unplug and replug, or press RST/EN button)
2. **5-minute OTA window** opens automatically
3. **Blue LED flashes** every 5 seconds to confirm OTA readiness
4. **Upload firmware** via PlatformIO or Arduino IDE
5. Device reboots and applies update

### Visual Status Indicators

When OTA is available, you'll see:

- **Blue LED flash** every 5 seconds (first LED in strip)
- **Web logs** show: "OTA Ready - Use Arduino IDE or platformio for updates"

### OTA Update Process

**PlatformIO:**

```bash
# First USB upload to configure OTA
pio run -t upload

# Configure platformio.ini for wireless uploads:
# upload_protocol = espota
# upload_port = <ESP32_IP_ADDRESS>

# Upload wirelessly (within 5 minutes of power-on!)
pio run -t upload
```

**Arduino IDE:**

1. Tools → Port → Network Ports → `sunrise-alarm at <IP>`
2. Upload sketch (within 5 minutes of power-on!)

**During upload:**

- LEDs show **blue progress bar** across the strip
- Upload takes 20-30 seconds typically
- Device automatically reboots on success
- **Red flashing LEDs** indicate upload error

### OTA Configuration

| Setting            | Value           | Location                    |
| ------------------ | --------------- | --------------------------- |
| **Hostname**       | `sunrise-alarm` | `config.h` → `OTA_HOSTNAME` |
| **Password**       | `sunrise2024`   | `config.h` → `OTA_PASSWORD` |
| **Port**           | `3232`          | Standard ArduinoOTA port    |
| **Initial Window** | 5 minutes       | After power-on only         |

### OTA Troubleshooting

**"Device not found" error:**

- Device may have entered deep sleep - **power cycle to restart 5-minute window**
- Check device and computer are on same network
- Verify IP address hasn't changed (check router or serial monitor)
- **Firewall blocking**: Ensure port 13351 is open on your system firewall (you can specify this in the platformio.ini with `--host_port=13351`)
  - **NixOS users**: `sudo nixos-firewall-tool open tcp 13351`
  - **Linux (ufw)**: `sudo ufw allow 13351/tcp`
  - **Linux (firewalld)**: `sudo firewall-cmd --add-port=13351/tcp --permanent && sudo firewall-cmsd --reload`
  - **macOS**: System Preferences → Security & Privacy → Firewall → Firewall Options
  - **Windows**: Windows Defender Firewall → Advanced Settings → Inbound Rules → New Rule

**"Auth Failed" error:**

- OTA password mismatch in `config.h`
- Check `OTA_PASSWORD` matches on device and in IDE
- Re-upload via USB to reset credentials

**Upload timeout:**

- WiFi connection unstable - move closer to router
- 5-minute window expired - power cycle device
- Try accessing web interface first to confirm device is online

**Device immediately sleeps:**

- Normal behavior after 5-minute window expires
- **Solution**: Power cycle for new 5-minute window

### Development Workflow

**For active development:**

1. **Keep device powered on**: Device stays available for 5 minutes after boot
2. **Serial monitor**: Connect via USB for debugging
3. **Quick updates**: Power cycle between uploads for fresh OTA window

**For production use:**

1. **Device sleeps** between alarms for battery efficiency
2. **Update on demand**: Power cycle when update needed
3. **Web interface**: Available during initial boot window and alarms
4. **No interruption**: Updates only happen during scheduled windows

## 📱 Web Interface & Remote Access

### ⚠️ Web Server Availability

**The web interface is ONLY available when:**

| **Scenario**         | **Web Server Active** | **Duration**             |
| -------------------- | --------------------- | ------------------------ |
| **Initial Power-On** | ✅ Yes                | 5 minutes                |
| **During Alarm**     | ✅ Yes                | Duration of alarm        |
| **Deep Sleep Wake**  | ❌ No                 | N/A                      |
| **Button Press**     | ❌ No                 | Device enters deep sleep |

This design saves power by only running the web server when needed.

## 🔘 Button Functions

### BOOT Button (GPIO0)

The built-in BOOT button provides manual control:

**Press BOOT button:**

- **Triggers immediate alarm sync** with Supabase database
- **Device enters deep sleep** immediately after sync
- **Use case**: Force update of alarm schedule or manually put device to sleep

**Behavior:**

1. Press BOOT button (GPIO0)
2. Device syncs alarms from Supabase
3. LEDs briefly show sync feedback
4. Device immediately enters deep sleep
5. Wakes at next scheduled alarm time

**Note**: Button press does NOT enable OTA or web server - only syncs and sleeps.

## 🔄 Power Management

The ESP32 uses intelligent power management:

- **Active states**:
  - Initial boot: 5 minutes (OTA + Web Server)
  - During alarm: Sunrise animation duration
  - Manual sync: Brief sync then immediate sleep
- **Sleep current**: ~10µA in deep sleep mode
- **Wake sources**:
  - Timer interrupt for next alarm check
  - BOOT button press for manual sync
- **Battery life**: Weeks to months depending on LED usage and alarm frequency

### Device States

```
Power On → [5 min Active: OTA + Web] → Deep Sleep
                                            ↓
                                      Timer Wake ──→ Check Alarm
                                            ↓              ↓
                                      Deep Sleep    [Alarm Active: Web + Animation]
                                            ↑              ↓
                                            └──────────────┘
                                            
Button Press → [Brief: Sync] → Deep Sleep
```

## 🎨 Color Presets

| Preset     | Description                | Color Transition                        |
| ---------- | -------------------------- | --------------------------------------- |
| `sunrise`  | Natural sunrise simulation | Deep red → Orange → Yellow → White      |
| `ocean`    | Ocean-inspired colors      | Deep blue → Cyan → Light blue           |
| `forest`   | Forest-themed colors       | Dark green → Light green → Yellow-green |
| `lavender` | Gentle morning colors      | Purple → Pink → Light pink              |

## 🔒 Security Features

### Row Level Security (RLS)

- **Device isolation**: Each ESP32 can only access its own alarms
- **MAC address filtering**: Uses device MAC as unique identifier
- **API key protection**: Secure access using Supabase anon key
- **Automated filtering**: ESPSupabase library handles secure queries

### Data Protection

- Configuration files excluded from git (`.gitignore`)
- No sensitive data stored on device
- Encrypted HTTPS communication with Supabase

## 📱 Managing Alarms

### Adding New Alarms

Use Supabase dashboard or SQL commands:

```sql
INSERT INTO alarms (device_id, time, days_of_week, brightness_level, duration_minutes, color_preset)
VALUES ('YOUR_ESP32_MAC', '07:00:00', ARRAY[1,2,3,4,5], 255, 30, 'sunrise');
```

### Day Codes

- `0` = Sunday, `1` = Monday, `2` = Tuesday, `3` = Wednesday
- `4` = Thursday, `5` = Friday, `6` = Saturday

### Remote Management

1. **Supabase Dashboard**: Edit alarms through web interface
2. **API Integration**: Build custom apps using Supabase REST API
3. **Manual Sync**: Press BOOT button to force update from database

## 🐛 Troubleshooting

### Common Issues

**WiFi Connection Failed:**

- Check SSID and password in `config.h`
- Ensure 2.4GHz WiFi (ESP32 doesn't support 5GHz)
- Verify signal strength near device

**Database Connection Issues:**

- Verify Supabase URL and API key
- Check RLS policies are properly configured
- Ensure device MAC address matches database entries

**LEDs Not Working:**

- Check wiring: Data pin to GPIO4, proper power supply
- Verify `LED_PIN` and `NUM_LEDS` in configuration
- Test with smaller LED count first

**Button Not Responding:**

- BOOT button is GPIO0 on most ESP32 boards
- Button press triggers sync and enters deep sleep (expected behavior)
- Check serial logs to confirm button detection

**OTA Updates Failing:**

- Device must be within 5-minute boot window after power-on
- Check network connectivity and firewall settings
- Verify OTA password matches configuration
- Try USB upload first to reset OTA credentials
- Power cycle device to restart OTA window

**Web Interface Not Accessible:**

- Only available during initial 5 minutes or during alarms
- Check device IP address in router admin panel
- Ensure device and computer on same network
- Try `http://sunrise-alarm.local` if mDNS is available
- Power cycle to restart web server window

**Device Immediately Sleeps:**

- Normal behavior after 5-minute boot window
- Press BOOT button to sync and sleep manually
- Wait for scheduled alarm to wake device
- Power cycle for new 5-minute active window

### Debug Mode

Enable detailed logging by setting `DEBUG_MODE 1` in `config.h`:

```cpp
#define DEBUG_MODE 1
```

Monitor serial output at 115200 baud for troubleshooting information.

### Remote Debugging

During the 5-minute boot window or alarms:

1. **Check status**: Access dashboard for system health
2. **Monitor logs**: Use auto-refreshing logs page
3. **Force sync**: Press BOOT button to trigger manual sync
4. **Test hardware**: Run LED test animation
5. **Update firmware**: Upload fixes via OTA (boot window only)

## 🔄 Development Workflow

```bash
# Build and upload
pio run -t upload

# Monitor serial output
pio device monitor

# Clean build
pio run -t clean

# Update libraries
pio lib update
```

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the project
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- [ESPSupabase](https://github.com/jhagas/ESPSupabase) - Simplified Supabase integration for ESP32
- [FastLED](https://github.com/FastLED/FastLED) - High-performance LED library
- [Supabase](https://supabase.com/) - Backend as a Service platform
- [ESP32 Community](https://github.com/espressif/arduino-esp32) - ESP32 Arduino framework

## 📸 Gallery

> _Add photos of your completed project here_

---

<div align="center">
  <strong>Wake up naturally with the Sunrise Alarm Clock! 🌅</strong>
  <br>
  <sub>Built with ❤️ using ESP32, WS2812B LEDs, and Supabase</sub>
</div>
