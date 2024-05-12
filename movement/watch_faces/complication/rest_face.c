/*
 * MIT License
 *
 * Copyright (c) 2024 <#author_name#>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include "rest_face.h"
#include "watch.h"
#include "watch_utility.h"

static const uint16_t _defaults[] = {30, 60, 120}; // default timers: 30 seconds, 60, 120
static const int8_t _sound_seq_beep[] = {BUZZER_NOTE_C8, 3, BUZZER_NOTE_REST, 3, -2, 2, BUZZER_NOTE_C8, 5, BUZZER_NOTE_REST, 35, 0};

static uint8_t _beeps_to_play;    // temporary counter for ring signals playing
// pinched wholesale from timer_face
static void _signal_callback() {
    if (_beeps_to_play) {
        _beeps_to_play--;
        watch_buzzer_play_sequence((int8_t *)_sound_seq_beep, _signal_callback);
    }
}


static void _set_next_valid_timer(rest_state_t *state) {
    state->current = (state->current + 1) % REST_SLOTS;
    if ((state->timers[state->current] & 0xFFFFFF) == 0) {
        uint8_t i = state->current;
        do {
            i = (i + 1) % REST_SLOTS;
        } while ((state->timers[i] & 0xFFFFFF) == 0 && i != state->current);
        state->current = i;
    }
}

static void _reset_display(rest_state_t *state) {
    watch_display_string("re", 0);
    watch_set_colon();
    watch_clear_indicator(WATCH_INDICATOR_BELL);
    watch_display_string("        ", 4);
}

static bool _draw(rest_state_t *state) {
    char buf[16];
    watch_duration_t duration;

    watch_display_string("re", 0);

    if (state->mode == rest_idle) {
      duration = watch_utility_seconds_to_duration(state->timers[state->current]);
      sprintf(buf, "%2d%02d", duration.minutes, duration.seconds);
      watch_display_string(buf, 4);
      return false;
    }

    if (state->remaining_seconds == 0 && state->overrun == 0) {
      // _reset_display(state);
      watch_set_indicator(WATCH_INDICATOR_BELL);
      sprintf(buf, "%2d%02d--", 0, 0);
      watch_display_string(buf, 4);
      _beeps_to_play = 2; // more beeps
      watch_buzzer_play_sequence((int8_t *)_sound_seq_beep, _signal_callback);
      return false;
    }

    if (state->remaining_seconds > 0) {
      watch_duration_t duration = watch_utility_seconds_to_duration(state->remaining_seconds);
      sprintf(buf, "%2d%02d", duration.minutes, duration.seconds);
      watch_display_string(buf, 4);
      return false;
    }

    if (state->overrun > 0) {
      watch_duration_t duration = watch_utility_seconds_to_duration(state->overrun);
      if (duration.minutes == 9 && duration.seconds == 59) {
        state->mode = rest_idle; // stop the timer entirely after 10 minutes
        sprintf(buf, "------");
        watch_display_string(buf, 4);
        watch_clear_colon();
        return true; // let the watch go to sleep
      } else {
        sprintf(buf, "%2d%02d", duration.minutes, duration.seconds);
        watch_display_string(buf, 4);
      }
    }

    return false;
}

static void _start(rest_state_t *state) {
    state->mode = rest_running;
    state->remaining_seconds = state->timers[state->current];
    state->overrun = 0;
}


void rest_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
    (void) settings;
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(rest_state_t));
        memset(*context_ptr, 0, sizeof(rest_state_t));
        rest_state_t *state = (rest_state_t *)*context_ptr;
        state->current = 0;
        for (uint8_t i = 0; i < sizeof(_defaults) / sizeof(_defaults[0]); i++) {
            state->timers[i] = _defaults[i];
        }
    }
}

void rest_face_activate(movement_settings_t *settings, void *context) {
    (void) settings;
    rest_state_t *state = (rest_state_t *)context;
    _reset_display(state);
    _draw(state);
}

bool rest_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    rest_state_t *state = (rest_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            // Show your initial UI here.
            _draw(state);
            break;
        case EVENT_TICK:
            // If needed, update your display here.
            if (state->mode == rest_running) {
              if (state->remaining_seconds > 0) {
                state->remaining_seconds--;
                if (state->remaining_seconds == 0) {
                  watch_set_led_green();
                }
              } else {
                if (state->overrun < 3) {
                  if (state->overrun % 2 == 1) {
                    watch_set_led_green();
                  } else {
                    watch_set_led_off();
                  }
                }

                state->overrun++;
              }
              _draw(state);
            }
            break;
        case EVENT_LIGHT_BUTTON_UP:
            // You can use the Light button for your own purposes. Note that by default, Movement will also
            // illuminate the LED in response to EVENT_LIGHT_BUTTON_DOWN; to suppress that behavior, add an
            // empty case for EVENT_LIGHT_BUTTON_DOWN.
            if (state->mode == rest_idle) {
              _set_next_valid_timer(state);
              _draw(state);
            }
            break;
        case EVENT_ALARM_BUTTON_UP:
            _reset_display(state);
            _start(state);
            _draw(state);
            break;
        case EVENT_MODE_BUTTON_UP:
            if (state->mode == rest_running) {
              watch_set_led_off();
              state->mode = rest_idle;
              _reset_display(state);
              _draw(state);
              break;
            }
            movement_move_to_next_face();
            break;
        case EVENT_TIMEOUT:
            // Your watch face will receive this event after a period of inactivity. If it makes sense to resign,
            // you may uncomment this line to move back to the first watch face in the list:
            movement_move_to_face(0);
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            // If you did not resign in EVENT_TIMEOUT, you can use this event to update the display once a minute.
            // Avoid displaying fast-updating values like seconds, since the display won't update again for 60 seconds.
            // You should also consider starting the tick animation, to show the wearer that this is sleep mode:
            // watch_start_tick_animation(500);
            break;
        default:
            // Movement's default loop handler will step in for any cases you don't handle above:
            // * EVENT_LIGHT_BUTTON_DOWN lights the LED
            // * EVENT_MODE_BUTTON_UP moves to the next watch face in the list
            // * EVENT_MODE_LONG_PRESS returns to the first watch face (or skips to the secondary watch face, if configured)
            // You can override any of these behaviors by adding a case for these events to this switch statement.
            return movement_default_loop_handler(event, settings);
    }

    // return true if the watch can enter standby mode. Generally speaking, you should always return true.
    // Exceptions:
    //  * If you are displaying a color using the low-level watch_set_led_color function, you should return false.
    //  * If you are sounding the buzzer using the low-level watch_set_buzzer_on function, you should return false.
    // Note that if you are driving the LED or buzzer using Movement functions like movement_illuminate_led or
    // movement_play_alarm, you can still return true. This guidance only applies to the low-level watch_ functions.
    return true;
}

void rest_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;

    // handle any cleanup before your watch face goes off-screen.
}

