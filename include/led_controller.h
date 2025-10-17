#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <FastLED.h>
#include <Arduino.h>
#include "alarm_manager.h"

class LEDController {
public:
    static void init();
    static void clear();
    static void show_status_indicator();
    static void show_button_feedback();
    static void show_ota_progress(unsigned int progress, unsigned int total);
    static void show_ota_error();
    static void run_test_animation();
    static void run_sunrise_animation(ColorPreset& preset, int duration_ms, int max_brightness);
    static bool is_initialized() { return initialized; }
    
private:
    static CRGB* leds;
    static int num_leds;
    static bool initialized;
    
    static CRGB blend_multiple_colors(SunriseStage stages[], int stage_count, float progress);
    static void add_sparkle_effect(CRGB base_color, float intensity);
    static void add_breathing_effect(float progress, float& brightness_multiplier);
    static void add_warmth_gradient(float progress);
    static void add_wave_effect(CRGB base_color, float progress);
};

#endif
