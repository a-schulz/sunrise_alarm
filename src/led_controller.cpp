#include "led_controller.h"
#include "logger.h"
#include "config.h"

CRGB* LEDController::leds = nullptr;
int LEDController::num_leds = NUM_LEDS;
bool LEDController::initialized = false;

void LEDController::init() {
    if (!initialized) {
        leds = new CRGB[num_leds];
        FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, num_leds);
        FastLED.setBrightness(50);
        FastLED.clear();
        FastLED.show();
        initialized = true;
        WEB_LOG("LED strip initialized");
    }
}

void LEDController::clear() {
    if (initialized) {
        FastLED.clear();
        FastLED.show();
    }
}

void LEDController::show_status_indicator() {
    init();
    leds[0] = CRGB::Blue;
    FastLED.setBrightness(50);
    FastLED.show();
    delay(100);
    clear();
}

void LEDController::show_button_feedback() {
    init();
    fill_solid(leds, num_leds, CRGB::Blue);
    FastLED.setBrightness(100);
    FastLED.show();
    delay(200);
    clear();
}

void LEDController::show_ota_progress(unsigned int progress, unsigned int total) {
    init();
    int led_progress = (progress * num_leds) / total;
    FastLED.clear();
    for (int i = 0; i < led_progress; i++) {
        leds[i] = CRGB::Blue;
    }
    FastLED.show();
}

void LEDController::show_ota_error() {
    init();
    for (int i = 0; i < 3; i++) {
        fill_solid(leds, num_leds, CRGB::Red);
        FastLED.show();
        delay(200);
        clear();
        delay(200);
    }
}

void LEDController::run_test_animation() {
    init();
    WEB_LOG("Running LED test animation");
    
    for (int hue = 0; hue < 256; hue += 4) {
        fill_rainbow(leds, num_leds, hue, 256/num_leds);
        FastLED.show();
        delay(50);
    }
    clear();
}

void LEDController::run_sunrise_animation(ColorPreset& preset, int duration_ms, int max_brightness) {
    unsigned long start_time = millis();
    unsigned long last_update = start_time;
    
    DEBUG_PRINTLN("Starting advanced sunrise animation...");
    init();
    
    while (millis() - start_time < duration_ms) {
        float progress = (float)(millis() - start_time) / (float)duration_ms;
        if (progress > 1.0) progress = 1.0;
        
        CRGB current_color = blend_multiple_colors(preset.stages, preset.stage_count, progress);
        
        float breathing_multiplier = 1.0f;
        add_breathing_effect(progress, breathing_multiplier);
        
        float brightness_progress = ease8InOutQuad(progress * 255) / 255.0f;
        int brightness = (int)(brightness_progress * max_brightness * breathing_multiplier);
        FastLED.setBrightness(brightness);
        
        fill_solid(leds, num_leds, current_color);
        
        if (progress > 0.3f && progress < 0.8f) {
            add_sparkle_effect(current_color, (progress - 0.3f) * 2.0f);
        }
        
        if (progress > 0.5f) {
            add_warmth_gradient(progress);
        }
        
        if (preset.name == "ocean") {
            add_wave_effect(current_color, progress);
        }
        
        FastLED.show();
        
        int update_delay = (int)(50 + 450 * (1.0f - 4.0f * progress * (1.0f - progress)));
        delay(update_delay);
        
        if ((int)(progress * 10) > (int)(((float)(last_update - start_time) / (float)duration_ms) * 10)) {
            DEBUG_PRINT("Animation progress: ");
            DEBUG_PRINT((int)(progress * 100));
            DEBUG_PRINTLN("%");
            last_update = millis();
        }
    }
    
    DEBUG_PRINTLN("Main animation complete - entering daylight phase");
    
    CRGB final_color = preset.stages[preset.stage_count - 1].color;
    FastLED.setBrightness(max_brightness);
    
    for (int minute = 0; minute < 5; minute++) {
        for (int second = 0; second < 60; second++) {
            float variation = 0.95f + 0.1f * sin(millis() * 0.001f);
            FastLED.setBrightness((int)(max_brightness * variation));
            
            CRGB day_color = final_color;
            day_color.r = min(255, (int)(day_color.r * (0.98f + 0.04f * sin(millis() * 0.0005f))));
            
            fill_solid(leds, num_leds, day_color);
            FastLED.show();
            delay(1000);
        }
        
        DEBUG_PRINT("Daylight phase: ");
        DEBUG_PRINT(minute + 1);
        DEBUG_PRINTLN("/5 minutes");
    }
    
    DEBUG_PRINTLN("Starting fade out...");
    for (int brightness = max_brightness; brightness >= 0; brightness -= 2) {
        FastLED.setBrightness(brightness);
        FastLED.show();
        delay(50);
    }
    
    clear();
    DEBUG_PRINTLN("Sunrise alarm completed");
}

CRGB LEDController::blend_multiple_colors(SunriseStage stages[], int stage_count, float progress) {
    if (progress <= 0.0f) return stages[0].color;
    if (progress >= 1.0f) return stages[stage_count - 1].color;
    
    float accumulated = 0.0f;
    for (int i = 0; i < stage_count - 1; i++) {
        float stage_end = accumulated + stages[i].duration_percent;
        if (progress <= stage_end) {
            float local_progress = (progress - accumulated) / stages[i].duration_percent;
            uint8_t blend_amount = ease8InOutQuad(local_progress * 255);
            return blend(stages[i].color, stages[i + 1].color, blend_amount);
        }
        accumulated = stage_end;
    }
    return stages[stage_count - 1].color;
}

void LEDController::add_sparkle_effect(CRGB base_color, float intensity) {
    int sparkle_count = (int)(num_leds * 0.1f * intensity);
    
    for (int i = 0; i < sparkle_count; i++) {
        int pos = random16(num_leds);
        if (random8() < 50) {
            CRGB sparkle_color = base_color;
            sparkle_color.r = min(255, sparkle_color.r + random8(50, 100));
            sparkle_color.g = min(255, sparkle_color.g + random8(30, 70));
            sparkle_color.b = min(255, sparkle_color.b + random8(20, 50));
            leds[pos] = sparkle_color;
        }
    }
}

void LEDController::add_breathing_effect(float progress, float& brightness_multiplier) {
    float breathing_intensity = 1.0f - (progress * 0.7f);
    float breathing_cycle = sin(millis() * 0.002f) * breathing_intensity * 0.1f;
    brightness_multiplier = 1.0f + breathing_cycle;
}

void LEDController::add_warmth_gradient(float progress) {
    int center_led = num_leds / 2;
    float warmth_intensity = (progress - 0.5f) * 2.0f;
    
    for (int i = 0; i < num_leds; i++) {
        float distance_from_center = abs(i - center_led) / (float)(num_leds / 2);
        float warmth_factor = 1.0f - (distance_from_center * warmth_intensity * 0.3f);
        
        leds[i].r = min(255, (int)(leds[i].r * (0.8f + warmth_factor * 0.4f)));
        leds[i].g = min(255, (int)(leds[i].g * (0.9f + warmth_factor * 0.2f)));
    }
}

void LEDController::add_wave_effect(CRGB base_color, float progress) {
    unsigned long time = millis();
    for (int i = 0; i < num_leds; i++) {
        float wave1 = sin((i * 0.1f) + (time * 0.003f)) * 0.2f;
        float wave2 = sin((i * 0.05f) + (time * 0.002f)) * 0.1f;
        float wave_effect = (wave1 + wave2) * progress;
        
        leds[i].b = min(255, (int)(leds[i].b * (1.0f + wave_effect)));
        leds[i].g = min(255, (int)(leds[i].g * (1.0f + wave_effect * 0.5f)));
    }
}
