/*
 * Counts Chinese characters typed with a two-keystroke Shuangpin scheme.
 *
 * Compiled next to widgets/layer.c (see CMakeLists.txt) because that is the
 * widget that draws the number; widgets/screen.c is what it asks for a repaint.
 *
 * Copyright (c) 2021 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

/** Characters counted since boot, whole characters only. */
int32_t zmk_char_count_get(void);

/** True while a lone letter is still waiting for its partner *and* it has not
 *  aged out, i.e. while a display should show a half character. */
bool zmk_char_count_half_pending(void);

/** Uptime of the last event that changed any of this, so a display can tell
 *  when an unpaired letter has aged out without waiting for another key. */
int64_t zmk_char_count_last_change(void);

/** Letters abandoned instead of paired. Diagnostics only. */
int32_t zmk_char_count_dropped(void);
