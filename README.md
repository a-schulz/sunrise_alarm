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

| Component | Description | Quantity |
|-----------|-------------|----------|
| **ESP32** | Main microcontroller with WiFi capability | 1 |
| **WS2812B LED Strip** | Addressable RGB LED strip | 1 |
| **5V Power Supply** | For ESP32 and LED strip | 1 |
| **Jumper Cables** | For connections | Several |
| **Breadboard** *(optional)* | For prototyping | 1 |

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

## 🔘 Button Functionality

The ESP32's built-in BOOT button (GPIO0) provides manual sync functionality:

- **Press and release** the BOOT button to force sync with Supabase
- **Visual feedback**: Blue LED flash confirms sync operation
- **Wake from sleep**: Button press will wake the device from deep sleep
- **Debouncing**: Built-in 50ms debounce prevents false triggers

## 🔄 Over-The-Air (OTA) Updates

### ⚠️ Important: OTA Availability

**OTA updates are ONLY available in these scenarios:**

| **Scenario** | **OTA Available** | **How to Activate** |
|--------------|-------------------|---------------------|
| **Initial Startup** | ✅ Yes | First boot after power on or reset |
| **Button Wake** | ✅ Yes | Press BOOT button to wake from sleep |
| **Deep Sleep Wake** | ❌ No | Device goes straight back to sleep |

**Why this design?**
- **Battery efficiency**: Deep sleep wakes are frequent (every hour) - enabling OTA would drain battery
- **Predictable updates**: You know exactly when OTA is available
- **Development friendly**: Button press gives you OTA access anytime

### How to Perform OTA Updates

#### Method 1: Initial Boot (Easiest)
1. **Power cycle the device** (unplug and plug back in, or press EN/RST button)
2. **Wait for WiFi connection** (check blue LED flash every 10 seconds)
3. **Upload firmware** within the OTA window
4. Device reboots and applies update

#### Method 2: Button Wake (Most Convenient)
1. **Press BOOT button** to wake device from sleep
2. **Blue LED flashes** indicate device is awake and OTA-ready
3. **Upload firmware** via PlatformIO or Arduino IDE
4. Update completes and device reboots

### Visual Status Indicators

When OTA is available:
- **Blue LED flash** every 10 seconds on first LED
- **Web interface accessible** at `http://<IP>`
- **Serial logs** show "Device awake - Ready for OTA updates"

### OTA Update Process

**PlatformIO:**
```bash
# After first USB upload, configure platformio.ini:
# upload_protocol = espota
# upload_port = <ESP32_IP_ADDRESS>

# Then upload wirelessly:
pio run -t upload
```

**Arduino IDE:**
1. Tools → Port → Network Ports → `sunrise-alarm at <IP>`
2. Upload sketch normally

**During upload:**
- LEDs show **blue progress bar** across the strip
- Upload takes 20-30 seconds typically
- Device automatically reboots on success
- **Red flashing LEDs** indicate upload error

### OTA Configuration

| Setting | Value | Location |
|---------|-------|----------|
| **Hostname** | `sunrise-alarm` | `config.h` → `OTA_HOSTNAME` |
| **Password** | `sunrise2024` | `config.h` → `OTA_PASSWORD` |
| **Port** | `3232` | Standard ArduinoOTA port |

### OTA Troubleshooting

**"Device not found" error:**
- Device is in deep sleep - press BOOT button to wake
- Check device and computer are on same network
- Verify IP address hasn't changed (check router)

**"Auth Failed" error:**
- OTA password mismatch
- Check `config.h` has correct `OTA_PASSWORD`
- Re-upload via USB once to reset credentials

**Upload timeout:**
- WiFi connection unstable
- Device went to sleep during upload (shouldn't happen with button wake)
- Try power cycling and uploading immediately

### Development Workflow

**For active development:**
1. Keep USB connected - serial monitor prevents sleep
2. Press BOOT button when needed for wireless updates
3. Use web interface for remote debugging

**For production use:**
1. Power cycle or button press for updates
2. Device sleeps between alarms for battery efficiency
3. Web logs available during OTA window

## 📱 Web Interface & Remote Access

### Accessing the Web Dashboard

Once your ESP32 is connected to WiFi, you can access the web interface:

1. **Find the IP address**: Check your router's admin panel or use a network scanner
2. **Open browser**: Navigate to `http://<ESP32_IP_ADDRESS>` 
3. **Alternative**: Use the hostname `http://sunrise-alarm.local` (if mDNS is supported)

### Web Interface Features

| Page | URL | Description |
|------|-----|-------------|
| **Dashboard** | `/` | System status, controls, device info |
| **Live Logs** | `/logs` | Real-time system logs with auto-refresh |
| **LED Test** | `/test` | Rainbow LED test animation |
| **Manual Sync** | `/sync` | Force alarm synchronization |

### Real-Time Logging

The device provides comprehensive web-based logging:

- **Auto-refresh**: Logs page refreshes every 5 seconds
- **Persistent storage**: Last 100 log entries retained in memory
- **Timestamps**: All logs include precise timestamps
- **Multiple sources**: Boot events, WiFi status, alarm triggers, errors

**Access logs via**: `http://<ESP32_IP>/logs`

Example log output:
```
07:30:15 === Sunrise Alarm Clock Starting ===
07:30:16 Boot count: 1
07:30:17 WiFi connected! IP: 192.168.1.100
07:30:18 OTA Ready - Use Arduino IDE or platformio for updates
07:30:19 Web server started on http://192.168.1.100
07:30:20 Alarms fetched successfully
07:30:21 Total alarms loaded: 3
```

## 🔄 Power Management

The ESP32 uses intelligent power management:

- **Active time**: ~10 seconds every hour (or before next alarm)
- **Sleep current**: ~10µA in deep sleep mode
- **Wake sources**: 
  - Timer interrupt for next alarm check
  - BOOT button press for manual sync
  - **OTA mode**: Prevents sleep when updates are available
- **Battery life**: Weeks to months depending on LED usage

### Development Mode

For development and debugging, you can prevent deep sleep:

1. **Serial monitor active**: Device stays awake when connected to USB
2. **Web interface open**: Periodic requests keep device active
3. **OTA mode**: Device remains awake for 5 minutes after boot for updates

## 🎨 Color Presets

| Preset | Description | Color Transition |
|--------|-------------|------------------|
| `sunrise` | Natural sunrise simulation | Deep red → Orange → Yellow → White |
| `ocean` | Ocean-inspired colors | Deep blue → Cyan → Light blue |
| `forest` | Forest-themed colors | Dark green → Light green → Yellow-green |
| `lavender` | Gentle morning colors | Purple → Pink → Light pink |

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
- Check `BUTTON_PIN` configuration
- Verify debounce timing

**OTA Updates Failing:**
- Device must be awake (press BOOT button or access web interface)
- Check network connectivity and firewall settings
- Verify OTA password matches configuration
- Try USB upload first to reset OTA credentials

**Web Interface Not Accessible:**
- Check device IP address in router admin panel
- Ensure device and computer on same network
- Try `http://sunrise-alarm.local` if mDNS is available
- Access logs via serial monitor if web fails

### Debug Mode

Enable detailed logging by setting `DEBUG_MODE 1` in `config.h`:

```cpp
#define DEBUG_MODE 1
```

Monitor serial output at 115200 baud for troubleshooting information.

### Remote Debugging

When away from the device, use the web interface:

1. **Check status**: Access dashboard for system health
2. **Monitor logs**: Use auto-refreshing logs page
3. **Force sync**: Trigger manual alarm sync
4. **Test hardware**: Run LED test animation
5. **Update firmware**: Upload fixes via OTA

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

> *Add photos of your completed project here*

---

<div align="center">
  <strong>Wake up naturally with the Sunrise Alarm Clock! 🌅</strong>
  <br>
  <sub>Built with ❤️ using ESP32, WS2812B LEDs, and Supabase</sub>
</div>