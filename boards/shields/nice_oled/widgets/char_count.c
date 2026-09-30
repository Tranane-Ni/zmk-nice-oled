/*
 * Counts Chinese characters typed with a two-keystroke Shuangpin scheme.
 *
 * The keyboard is the wrong place to do this: the IME lives on the host and
 * never tells us what it committed. All we get is keycodes, so the count is a
 * reconstruction -- two letters in a row are taken to be one character, and a
 * letter that either sits alone behind a space or is followed by its partner
 * more than CHAR_COUNT_GAP_MS later is abandoned instead.
 *
 * The space bar is the one extra signal worth having, because the way this
 * keyboard is used is to submit each syllable with it. A pair that has not been
 * submitted yet is still sitting in the IME's own box, so a backspace after it
 * took off a letter, not a character; after a space, the same keypress is
 * really editing the text.
 *
 * sim/shuangpin_rule.py is the readable copy of this state machine and
 * sim/shuangpin_test.py exercises it -- --cases runs the boundary tests (two
 * letters exactly one threshold apart pair; one threshold plus a millisecond
 * do not), --live judges real keystrokes. sim/shuangpin_lab.html is a third
 * copy to type into by hand; the bench's --js flag checks it still judges the
 * same way, and every run of the bench diffs the threshold and the key sets
 * against this file. Change the rule in all three.
 *
 * Copyright (c) 2021 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/hid.h>

#include "char_count.h"
#include "screen.h"

/* Two letters further apart than this were not one syllable. The boundary is
 * exclusive: exactly this far apart still counts as one. Keep in step with
 * GAP_MS in sim/shuangpin_rule.py. 760 is what the typist settled on after
 * measuring their own key gaps, not a round guess. */
#ifndef CHAR_COUNT_GAP_MS
#define CHAR_COUNT_GAP_MS 760
#endif

/* HID keyboard usages: A is 0x04 through Z at 0x1D, on usage page 0x07. */
#define CHAR_USAGE_PAGE_KEYBOARD 0x07
#define CHAR_KEY_A 0x04
#define CHAR_KEY_Z 0x1D

/* zmk_mod_flags_t is the HID modifier byte: LCTL 0x01, LSFT 0x02, LALT 0x04,
 * LGUI 0x08 and the same four on the right at 0x10..0x80 (dt-bindings/zmk/
 * modifiers.h). Everything but the two shift bits. */
#define CHAR_MOD_BLOCKERS 0xDD

static int32_t chars;
static int32_t dropped;
static int32_t uncommitted; /* pairs typed since the last space, still in the IME box */
static uint8_t pending; /* 0 or 1: a letter still waiting for its partner */
static int64_t first_letter_ms; /* when that waiting letter arrived */
static int64_t last_letter_ms;
static bool have_last_letter;
static int64_t last_change_ms;

static void touch(void) { last_change_ms = k_uptime_get(); }

static bool fresh(void) {
    return !have_last_letter || k_uptime_get() - last_letter_ms <= CHAR_COUNT_GAP_MS;
}

static void add_letter(int64_t now) {
    if (pending && now - last_letter_ms > CHAR_COUNT_GAP_MS) {
        /* Its partner never came. Drop it rather than let it steal the first
         * letter of whatever is typed next. */
        dropped += pending;
        pending = 0;
    }

    if (pending == 0) {
        first_letter_ms = now;
    }
    pending++;
    last_letter_ms = now;
    have_last_letter = true;

    if (pending == 2) {
        pending = 0;
        chars++;
        uncommitted++;
    }
    touch();
}

/* A syllable was submitted with a space. Two things follow: a letter standing
 * in front of it is never going to be paired, and everything behind it is now
 * real text instead of something backspace can take apart one letter at a time.
 * ENTER is not treated this way: it is a normal key on this keyboard and is
 * deliberately ignored. */
static void submit(void) {
    if (pending) {
        dropped += pending;
        pending = 0;
    }
    uncommitted = 0;
    touch();
}

/* Backspace means one of two things and the space bar is what tells them apart.
 * Before a syllable has been submitted the IME still holds its letters, so
 * backspace takes off the last one and the character was never really written.
 * After it has been submitted, backspace is editing the text and eats the whole
 * character. */
static void undo(void) {
    if (pending == 1) {
        pending = 0;
    } else if (uncommitted > 0 && chars > 0) {
        /* Only the second letter came off. The first is still sitting in the
         * input box, waiting for a partner since it was pressed. */
        chars--;
        uncommitted--;
        pending = 1;
        last_letter_ms = first_letter_ms;
    } else if (chars > 0) {
        chars--;
    }
    touch();
}

static bool blocking_mods_down(void) {
    return (zmk_hid_get_explicit_mods() & CHAR_MOD_BLOCKERS) != 0;
}

static void char_count_eat(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev == NULL || !ev->state) { /* key releases are not input */
        return;
    }

    int64_t now = k_uptime_get();

    if (ev->usage_page == CHAR_USAGE_PAGE_KEYBOARD && ev->keycode >= CHAR_KEY_A &&
        ev->keycode <= CHAR_KEY_Z) {
        if (blocking_mods_down()) {
            return; /* Ctrl+C and friends are not Chinese input */
        }
        add_letter(now);
        return;
    }

    switch (ev->keycode) {
    case 0x2C: /* HID_SPACE */
        if (blocking_mods_down()) {
            return;
        }
        submit();
        break;
    case 0x28: /* HID_ENTER -- ignored, not a submit key. It would be harmless
                 * as input but on this keyboard the thumb key that taps as
                 * ENTER is also the layer switch, and switching layers must
                 * not abandon a syllable in progress. */
        break;
    case 0x2A: /* HID_BACKSPACE */
    case 0x4C: /* HID_DELETE */
        if (blocking_mods_down()) {
            return;
        }
        undo();
        break;
    default:
        break;
    }
}

/* The count is drawn on the layer row, so a keystroke that completes or
 * undoes a character has to repaint the screen itself: nothing else here is
 * listening for keystrokes, and waiting for the next battery event would
 * leave the number one behind for minutes. */
static void char_count_handler(const zmk_event_t *eh) {
    const int32_t before = chars;

    char_count_eat(eh);

    if (chars != before) {
        zmk_widget_screen_repaint();
    }
}

ZMK_LISTENER(char_count, char_count_handler);
ZMK_SUBSCRIPTION(char_count, zmk_keycode_state_changed);

int32_t zmk_char_count_get(void) { return chars; }

bool zmk_char_count_half_pending(void) { return pending == 1 && fresh(); }

int64_t zmk_char_count_last_change(void) { return last_change_ms; }

int32_t zmk_char_count_dropped(void) { return dropped; }
