/* SPDX-License-Identifier: GPL-3.0-only */
/* Ravel: sound presets (PRESETS on the voice pages) and the demo projects. */
#include "rv.h"

typedef struct { const char *name; uint8_t kind, engine; const int8_t *v; const rv_cz *cz; } rv_preset;   /* v: (parameter, value) pairs; cz: a CZ tone */
#define END -1

/* No factory sounds in this fork: PRESETS on the voice pages walks only your own sounds, from the bank
 * (SAVE > SOUNDS). The synth, CZ, piano, physical, resin, tine, voice, PERC and KIT presets, and the sounds the
 * demos picked, are gone: their tables were most of this file's room in the FM-1's flash. The demos stay, with
 * each track's default sound. */
#define NPRESETS 0

int rv_preset_count(void) { return NPRESETS; }
const char *rv_preset_name(int i) { (void)i; return ""; }
int rv_preset_is_kit(int i) { (void)i; return 0; }
int rv_preset_kind(int i) { (void)i; return -1; }

/* KIND changed on the track: its defaults (a PERC track starts as a KIT; a synth keeps its engine),
 * and its steps cleared when it changes between lanes and notes, which can't read each other's */
void rv_track_set_kind(int t, int kind)
{
    int was_kit = rv.tp[t & 7][TP_KIND] == KIND_PERC && rv.tp[t & 7][TP_ENGINE] == 1;
    int was_loop = rv.tp[t & 7][TP_KIND] == KIND_LOOP;
    int eng = kind == KIND_PERC ? 1 : kind == KIND_SYNTH && rv.tp[t & 7][TP_KIND] == KIND_SYNTH ? rv.tp[t & 7][TP_ENGINE] : 0;
    rv_track_defaults(t, kind, eng);
    if (kind == KIND_WEFT)                            /* WEFT: starts from its first patch */
        weft_preset_apply(t, 0);
    if (was_kit != (kind == KIND_PERC) || was_loop != (kind == KIND_LOOP)) {   /* (a LOOP track's steps are trigs: start clean) */
        int pt = rv.pat_cur[t & 7];
        for (rv.pat_cur[t & 7] = 0; rv.pat_cur[t & 7] < RV_PATTERNS; rv.pat_cur[t & 7]++)
            rv_pattern_clear(t);
        rv.pat_cur[t & 7] = (uint8_t)pt;
    }
}

void rv_preset_apply(int t, int i) { (void)t; (void)i; }   /* (no factory sounds) */
int rv_preset_step(int t, int cur, int dir) { (void)t; (void)dir; return cur; }

/* -------------------------------------------------------------- new project --- */
static void name_set(const char *s)
{
    int i;
    for (i = 0; i < 11 && s[i]; i++)
        rv.name[i] = s[i];
    rv.name[i] = 0;
}

/* A blank project. The demos (RAVEL, DUST, GRAPH) are gone from this fork: rv_demo keeps its name because the
 * platforms and the UI call it for a new project, and whatever the argument, it gives the blank one. */
RV_SMALL void rv_demo(int which)
{
    (void)which;
    rv_project_init();
    name_set("BLANK");
}
