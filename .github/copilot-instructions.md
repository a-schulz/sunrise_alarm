# Copilot Instructions - Sunrise Alarm Clock (ESP32)

## Project Overview

ESP32-based sunrise alarm clock with WS2812B LED strips, cloud database integration (Supabase), deep sleep power management, OTA updates, and web interface.

## Development Environment

- **Platform**: PlatformIO (NOT Arduino IDE)
- **Board**: ESP32 DevKit
- **Framework**: Arduino framework for ESP32
- **Keep `platformio.ini` updated** with all library dependencies and build flags

## Code Style & Best Practices

### C++ Conventions

- Use snake_case for functions and variables
- Use PascalCase for structs and classes
- Use UPPER_CASE for constants and defines
- Prefer `const` and `constexpr` over `#define` where possible
- Use explicit types (avoid `auto` in embedded contexts)

### Embedded Systems Principles

- **Memory efficiency**: ESP32 has limited RAM (~320KB)
- **Stack safety**: Avoid deep recursion, prefer iterative solutions
- **IRAM functions**: Mark interrupt handlers with `IRAM_ATTR`
- **RTC memory**: Use `RTC_DATA_ATTR` for data that persists across deep sleep
- **Heap fragmentation**: Avoid frequent dynamic allocations
- **Flash wear**: Minimize EEPROM/Flash writes

### Code Organization

- Write **modular, reusable** functions
- **Keep functions focused** on single responsibilities
- Extract magic numbers to named constants in `config.h`
- **Remove redundant code** - don't duplicate logic
- Group related functionality logically

### Comments

- **Do NOT add comments about changes** (e.g., "// Added feature X")
- Use comments to **explain WHY, not WHAT**
- Document complex algorithms and non-obvious logic
- **Don't comment trivial code** (e.g., `i++; // increment i`)
- Add brief function headers for public APIs only when needed

## Hardware & Power Management

### Deep Sleep Patterns

- **Always use deep sleep** between alarms to save power
- Wake sources: timer (`esp_sleep_enable_timer_wakeup`) and button (`esp_sleep_enable_ext0_wakeup`)
- **Clean shutdown**: Disconnect WiFi, clear LEDs, disable peripherals before sleep
- Use `RTC_DATA_ATTR` for variables that must persist across sleep cycles
- Calculate next wake time intelligently based on alarm schedule

### LED Control (FastLED)

- **Initialize LEDs lazily** - only when needed (saves power)
- Always call `FastLED.clear()` and `FastLED.show()` before deep sleep
- Use `FastLED.setBrightness()` for global brightness control
- Prefer `fill_solid()`, `fill_rainbow()` over pixel-by-pixel loops when possible
- Consider power draw: high brightness on many LEDs can exceed USB power limits

### Button Handling

- **Always debounce** button inputs (use timestamp-based debouncing)
- Use interrupts (`attachInterrupt`) for responsive button handling
- Mark ISRs with `IRAM_ATTR` for reliability
- Keep ISR code minimal - just set flags, process in main loop

## WiFi & Network

### Connection Management

- **Connect only when needed** to save power
- Disconnect WiFi before deep sleep: `WiFi.disconnect(); WiFi.mode(WIFI_OFF);`
- Handle connection failures gracefully with timeouts
- Use WiFi MAC address as unique device identifier

### WiFiManager (tzapu/WiFiManager)

- **Captive portal** for easy WiFi configuration without hardcoded credentials
- **Auto-connect**: Attempts to reconnect to last known network
- **Fallback AP mode**: Creates access point if connection fails
- **Non-blocking**: Use `setConfigPortalBlocking(false)` for async operation
- **Custom parameters**: Add Supabase URL/key as portal parameters for full setup
- **Reset option**: Provide button or web endpoint to clear saved WiFi credentials
- **Timeout handling**: Set reasonable timeouts to prevent indefinite blocking
- **Portal callbacks**: Use callbacks to show status on LEDs during configuration

### OTA Updates

- **OTA only available when device is awake** (not during deep sleep)
- Provide visual feedback during uploads (LED progress bar)
- Handle OTA errors gracefully with user feedback
- Password-protect OTA for security

### Web Server (esp32async/ESPAsyncWebServer)

- Use **async handlers** to prevent blocking
- Track web activity with timestamps to manage wake state
- Implement auto-refresh for log pages
- Keep HTML minimal - ESP32 has limited resources
- Always call `trackWebActivity()` in handlers to extend wake time

## Logging

### Dual Output Strategy

All logs must go to **both serial AND web** (when web server is active):

```cpp
WEB_LOG("Message"); // Logs to serial + web buffer
```

### Web Logging System

- Circular buffer with `MAX_LOG_ENTRIES` limit
- Include timestamps in log messages
- Auto-refresh web logs page every 5 seconds
- Serial output always active, web only when server running

### Debug Levels

- Use conditional compilation: `#if DEBUG_MODE`
- Production builds should minimize serial output to reduce overhead
- Keep critical logs (errors, state changes) always enabled

## Database Integration (Supabase)

### ESPSupabase Library

- Initialize once: `db.begin(SUPABASE_URL, SUPABASE_KEY);`
- Use method chaining: `db.from("table").select("*").eq("field", "value").doSelect()`
- Always check for errors in returned JSON
- Filter by `device_id` (WiFi MAC) for multi-device deployments

### Data Parsing

- Use `ArduinoJson` with `JsonDocument`
- Handle parsing errors gracefully
- Validate all fields before use (use `|` operator for defaults)
- Parse time strings carefully (HH:MM:SS format)

## Time Management

### NTP Synchronization

- Sync time on every wake from deep sleep
- Use `configTime()` with timezone offsets
- Always validate `getLocalTime()` success before using time data
- Handle NTP failures with retry logic

### Alarm Scheduling

- Store alarms in array with `MAX_ALARMS` limit
- Check day-of-week matching (0=Sunday, 6=Saturday)
- Calculate next wake time to minimize unnecessary wake cycles
- Default to hourly checks if no alarms scheduled

## Animation & Visual Effects

### Sunrise Simulation

- Multi-stage color transitions with duration percentages
- Smooth blending using `blend()` and easing functions
- Variable update rates (slower at start/end, faster in middle)
- Add subtle effects: sparkles, breathing, warmth gradients

### Performance

- Keep animation frame rates reasonable (20-50ms delays)
- Use efficient FastLED functions
- Avoid floating-point math in tight loops where possible
- Provide progress indicators for long operations

## Configuration Management

### Config File Pattern

- Use `config.h` for all user-configurable values
- Provide `config.example.h` template (committed to git)
- **Never commit actual `config.h`** (add to `.gitignore`)
- Group related settings with clear comments
- Keep `config.h` and `config.example.h` up to date

### Security

- Protect WiFi credentials, API keys, OTA passwords
- Use Row Level Security (RLS) in Supabase
- MAC address filtering for device isolation

## Documentation

### Keep README.md Updated

- Document all features and architecture changes
- Include hardware wiring diagrams (use mermaid syntax)
- Provide troubleshooting guides
- Add setup instructions for new users
- Update feature tables and status indicators

### Code Documentation

- Update function signatures when changing behavior
- Document non-obvious return values or side effects
- Explain complex bit manipulations or algorithms
- Note hardware dependencies and timing requirements

## Testing & Validation

### Before Deployment

- Test deep sleep wake cycles
- Verify OTA updates work
- Check memory usage (`ESP.getFreeHeap()`)
- Validate alarm triggering at correct times
- Test button debouncing under rapid presses
- Confirm web server accessibility

### Serial Monitoring

- Always monitor serial output during testing
- Use baud rate 115200 (defined in `platformio.ini`)
- Log important state transitions
- Include timestamps for debugging timing issues

## Common Patterns

### Lazy Initialization

```cpp
if (!componentInitialized) {
    initializeComponent();
    componentInitialized = true;
}
```

### Error Handling

- Check return values and handle failures gracefully
- Provide user feedback via LEDs/serial/web
- Don't crash on network failures
- Implement timeouts for blocking operations

### State Management

- Use global variables sparingly
- Leverage RTC memory for persistence
- Reset volatile state after deep sleep
- Track boot count for first-run logic

## Library-Specific Guidelines

### FastLED

- Call `FastLED.addLeds()` only once during initialization
- Use color temperature functions for natural white
- `ease8InOutQuad()` for smooth brightness transitions

### ESPAsyncWebServer

- Register all handlers in `setupWebServer()`
- Use lambda captures carefully (avoid dangling references)
- Send responses with appropriate MIME types

### ArduinoJson

- Choose document size based on actual data (use ArduinoJson Assistant)
- Reuse documents when possible
- Check `deserializeJson()` return value

## Performance Optimization

### Power Consumption

- Minimize WiFi connection time
- Use appropriate deep sleep intervals
- Reduce LED brightness when possible
- Disable unused peripherals

### Responsiveness

- Keep main loop iterations fast (<100ms)
- Use non-blocking delays
- Handle OTA and web server in loop
- Check wake conditions frequently during active periods
