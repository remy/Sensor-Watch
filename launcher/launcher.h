#ifndef LAUNCHER_H_
#define LAUNCHER_H_
#include <stdio.h>
#include <stdbool.h>

// TODO: none of this is implemented
typedef union {
    struct {
        uint32_t clock_mode_24h : 1;        // determines whether clock should use 12 or 24 hour mode.
        uint32_t signal_should_sound : 1;   // if true, a double beep is played at the top of each hour.
        uint32_t alarm_should_sound : 1;    // if true, the alarm interrupt can match a time and play a song.
        uint32_t alarm_minute : 6;          // the minute of the alarm we want to match
        uint32_t alarm_hour : 5;            // the second of the alarm we want to match
        uint32_t note_index : 4;            // the index of the tone to play on button press, or 0xF for no tone.
        uint32_t screensaver_interval : 3;  // 0 to disable screensaver, or a screensaver activation interval.
        uint32_t led_duration : 3;          // how many seconds to shine the LED for, or 0 to disable it.
        uint32_t led_red_color : 4;         // for general purpose illumination, the red LED value (0-15)
        uint32_t led_green_color : 4;       // for general purpose illumination, the green LED value (0-15)
    } bit;
    uint32_t value;
} LauncherSettings;

typedef enum LauncherEvent {
    EVENT_NONE = 0,             // There is no event to report.
    EVENT_ACTIVATE,             // Your widget is entering the foreground.
    EVENT_TICK,                 // Most common event type. Your widget is being called from the tick callback.
    EVENT_LIGHT_BUTTON_DOWN,    // The light button has been pressed, but not yet released.
    EVENT_LIGHT_BUTTON_UP,      // The light button was pressed and released.
    EVENT_LIGHT_LONG_PRESS,     // The light button was held for >2 seconds, and released.
    EVENT_MODE_BUTTON_DOWN,     // The mode button has been pressed, but not yet released.
    EVENT_MODE_BUTTON_UP,       // The mode button was pressed and released.
    EVENT_MODE_LONG_PRESS,      // The mode button was held for >2 seconds, and released.
    EVENT_ALARM_BUTTON_DOWN,    // The alarm button has been pressed, but not yet released.
    EVENT_ALARM_BUTTON_UP,      // The alarm button was pressed and released.
    EVENT_ALARM_LONG_PRESS,     // The alarm button was held for >2 seconds, and released.
} LauncherEvent;

typedef void (*launcher_widget_setup)(LauncherSettings *settings, void ** context_ptr);
typedef void (*launcher_widget_activate)(LauncherSettings *settings, void *context);
typedef void (*launcher_widget_loop)(LauncherEvent event, LauncherSettings *settings, void *context);
typedef void (*launcher_widget_resign)(LauncherSettings *settings, void *context);

typedef struct WatchWidget {
    char widget_name[11];
    bool snapback_enabled;
    launcher_widget_setup setup;
    launcher_widget_activate activate;
    launcher_widget_loop loop;
    launcher_widget_resign resign;
} WatchWidget;

typedef struct LauncherState {
    // properties stored in BACKUP register
    LauncherSettings launcher_settings;

    // transient properties
    int16_t current_widget;
    bool widget_changed;

    // LED stuff
    uint8_t light_ticks;
    bool led_on;
    
    // button tracking for long press
    uint8_t light_down_timestamp;
    uint8_t mode_down_timestamp;
    uint8_t alarm_down_timestamp;
} LauncherState;

void cb_mode_pressed();
void cb_light_pressed();
void cb_alarm_pressed();
void cb_tick();

#endif // LAUNCHER_H_
