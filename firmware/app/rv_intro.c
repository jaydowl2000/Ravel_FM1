/* SPDX-License-Identifier: GPL-3.0-only */
/* Ravel: no start screen. The VHS-tape intro (about 480 lines of code, the RAVEL wordmark's bitmap and the
 * floating-point work of its snow, rain and drips) is gone: its room in the FM-1's flash is what undo
 * (rv_ui.c) needed. The platforms still call ui_intro() at power-on, which now does nothing, so the
 * instrument comes up at once. The version is on GLO > FLASH. rv_logo.h and tools/make_logo.py stay for
 * whoever wants it back: the last version of this file with the start screen is in the repository's history.
 * Part of the engine unit (ravel_app.c), after rv_ui.c. */

static int intro_frame(void) { return 0; }
static int intro_skip(void) { return 0; }

/* the platform starts it (the FM-1 at power-on, the emulator page as it opens) */
void ui_intro(void) {}
int ui_intro_on(void) { return 0; }
