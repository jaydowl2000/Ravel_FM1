/* SPDX-License-Identifier: GPL-3.0-only */
/* TEST FIXTURES, not firmware: the three demo songs (RAVEL, DUST, GRAPH) that Ravel once carried. The firmware
 * has no demos any more (rv_demo gives a blank project); the scenarios and the emulator's smoke test use these as
 * ready-made material through the host's "demo N" command, so what they check is kept. The tracks play their
 * default sounds, as the demos did once the factory sounds were gone. Included by ravel_host.c and ravel_web.c
 * after the app (the rv.h state) is in. */

static rv_step *fx_st(int t, int s) { return &rv.pat[t][0].s[s]; }

static void fx_on(int t, int s, int note, int vel, int len)
{
    rv_step *p = fx_st(t, s);
    p->flags = ST_ON;
    p->note = (uint8_t)note;
    p->vel = (uint8_t)vel;
    p->len = (uint8_t)len;
}

static void fx_route(int a, int b, int type, int amt, int arg)
{
    rv.infl[a][b].type = (uint8_t)type;
    rv.infl[a][b].amt = (int8_t)amt;
    rv.infl[a][b].arg = (uint8_t)arg;
    rv.infl[a][b].chance = 100;
}


static void fx_name(const char *s)
{
    int i;
    for (i = 0; i < 11 && s[i]; i++)
        rv.name[i] = s[i];
    rv.name[i] = 0;
}

static void rv_demo_fixture(int which)
{
    int s;
    rv_project_init();
    if (which == 0) {
        /* RAVEL: synthesize and rebuild: the wood hits shake T7, T8 holds a phasing pad under it all. */
        fx_name("RAVEL");
        rv.gp[GP_TEMPO] = 104;
        rv.tp[0][TP_LEN] = 32;
        fx_on(0, 0, 57, 80, 60);
        fx_on(0, 16, 53, 80, 60);
        fx_on(0, 24, 55, 70, 30);
        rv.tp[1][TP_LEN] = 16;
        fx_on(1, 0, 45, 110, 6);
        fx_on(1, 3, 45, 80, 3);
        fx_on(1, 6, 57, 90, 2);
        fx_on(1, 8, 43, 110, 6);
        fx_on(1, 11, 43, 80, 3);
        fx_on(1, 14, 55, 90, 2);
        rv.tp[2][TP_LEN] = 12;                     /* polymeter against 16 */
        fx_on(2, 0, 76, 90, 3);
        fx_on(2, 3, 79, 70, 3);
        fx_on(2, 5, 81, 80, 6);
        fx_on(2, 9, 74, 70, 3);
        rv.tp[3][TP_LEN] = 16;
        fx_on(3, 2, 69, 100, 4);
        fx_on(3, 7, 72, 90, 4);
        fx_on(3, 13, 67, 100, 4);
        fx_st(3, 13)->ratchet = 2 | (1 << 3);
        rv.tp[4][TP_LEN] = 16;
        for (s = 0; s < 16; s += 4)
            fx_on(4, s, 50, 120, 2);
        fx_on(4, 10, 62, 80, 2);
        fx_on(4, 15, 57, 70, 2);
        fx_st(4, 15)->micro = 6;
        rv.tp[5][TP_LEN] = 16;
        fx_on(5, 6, 74, 70, 2);
        fx_on(5, 14, 79, 60, 2);
        rv.tp[6][TP_LEN] = 16;
        rv.tp[7][TP_LEN] = 16;
        fx_on(7, 0, 60, 100, 32);
        fx_on(7, 8, 67, 90, 32);
        /* influence: wood excites the metal and shakes the shaker; the bass opens the pad */
        fx_route(4, 5, RT_EXCITE, 40, 0);
        fx_route(4, 6, RT_TRIG, 0, 0);
        fx_route(1, 0, RT_MOD, 30, TP_CUT);
        fx_route(3, 7, RT_TRIG, 0, 0);
        rv.gp[GP_DLYTIME] = 92;
        rv.gp[GP_DLYFB] = 60;
        rv.gp[GP_REVSIZE] = 90;
        rv.scene[0].used = 1;
        for (s = 0; s < RV_TRACKS; s++) {
            int b;
            rv.scene[0].level[s] = (uint8_t)rv.tp[s][TP_LEVEL];
            for (b = 0; b < RV_TRACKS; b++)
                rv.scene[0].infl[s][b] = rv.infl[s][b];
        }
        rv.arr_len = 1;
        rv.arr[0].scene = 0;
        rv.arr[0].bars = 8;
        rv.gp[GP_SONGMODE] = 1;                    /* SONG: the scene comes round every 8 bars */
    } else if (which == 1) {
        /* DUST: lowercase. Clicks, droplets through the vocoder, a struck disc rung by dust,
         * a membrane and the feedback bus returning into the strings. */
        fx_name("DUST");
        rv.gp[GP_TEMPO] = 92;
        rv.tp[0][TP_LEVEL] = 60;
        rv.tp[0][TP_LEN] = 32;
        fx_on(0, 0, 60, 70, 120);
        fx_on(0, 16, 63, 70, 120);
        rv.tp[1][TP_LEN] = 16;
        rv.tp[1][TP_CLKSRC] = 3;                   /* dust clocks it */
        fx_on(1, 0, 72, 90, 2);
        fx_on(1, 5, 79, 80, 2);
        fx_on(1, 9, 84, 70, 2);
        rv.tp[2][TP_LEN] = 16;
        fx_on(2, 0, 41, 110, 4);
        fx_on(2, 11, 41, 80, 4);
        rv.tp[3][TP_LEN] = 16;
        rv.tp[3][TP_DIR] = DIR_PING;
        fx_on(3, 2, 67, 100, 2);
        fx_on(3, 6, 70, 90, 2);
        fx_on(3, 9, 74, 90, 2);
        fx_on(3, 13, 77, 100, 2);
        rv.tp[4][TP_LEN] = 16;
        fx_on(4, 4, 60, 100, 16);
        fx_on(4, 12, 60, 100, 16);
        rv.tp[5][TP_LEN] = 16;
        fx_on(5, 8, 55, 100, 4);
        rv.tp[5][TP_FBS] = 50;
        rv.gp[GP_FBRET] = 6;                       /* the feedback bus rings T6's strings */
        rv.gp[GP_FBPROC] = 2;
        rv.gp[GP_FBREGEN] = 90;
        rv.tp[6][TP_LEN] = 16;
        for (s = 0; s < 16; s += 2)
            fx_on(6, s, 53 + (s * 5) % 16, 100, 4);
        fx_route(2, 6, RT_CLOCK, 0, 0);
        fx_route(4, 5, RT_EXCITE, 30, 0);
        rv.gp[GP_REVSIZE] = 110;
        rv.gp[GP_REVLEVEL] = 100;
        rv.scene[0].used = 1;
        for (s = 0; s < RV_TRACKS; s++) {
            int b;
            rv.scene[0].level[s] = (uint8_t)rv.tp[s][TP_LEVEL];
            for (b = 0; b < RV_TRACKS; b++)
                rv.scene[0].infl[s][b] = rv.infl[s][b];
        }
        rv.arr_len = 1;
        rv.arr[0].scene = 0;
        rv.arr[0].bars = 16;
        rv.gp[GP_SONGMODE] = 1;
    } else if (which == 3) {
        /* GRAPH: the graph sequencer. T1's two heads split rhythm from melody: head 1 walks a
         * short looping path and plays (GATE), head 2 wanders the long way round at a third of
         * the speed, swelling faster and slower, and sets the pitch (PITCH). T2's head 1 rolls
         * into its steps (a DECAY motion), and its head 2 plays T5's wood on its own path. */
        static const uint8_t mel[16] = {60, 63, 67, 70, 72, 70, 67, 65, 63, 58, 60, 55, 67, 74, 72, 62};
        fx_name("GRAPH");
        rv.gp[GP_TEMPO] = 96;
        for (s = 2; s < RV_TRACKS; s++)
            if (s != 4 && s != 5)
                rv.mute |= (uint8_t)(1u << s);
        rv.tp[0][TP_LEN] = 16;
        for (s = 0; s < 16; s++)
            fx_on(0, s, mel[s], 70 + (s * 37) % 50, 3);
        fx_st(0, 3)->to[0] = 1;                          /* cords (step + 1): steps 0-3 loop back to 0, */
        fx_st(0, 3)->to[1] = 8;                          /* or, every other time, out to 7 */
        fx_st(0, 11)->to[0] = 1;                         /* and 11 returns to 0 */
        rv.tp[0][TP_SEQMODE] = SEQ_GRAPH;
        rv.tp[0][TP_BRANCH] = 1;                      /* TURNS */
        rv.tp[0][TP_H1ROLE] = 1;                      /* GATE */
        rv.tp[0][TP_H2ROLE] = ROLE_PITCH;
        rv.tp[0][TP_H2RATE] = 1;                      /* 1/3 */
        rv.tp[0][TP_H2START] = 9;
        rv.tp[0][TP_H2SHAPE] = FG_SWELL;
        rv.tp[0][TP_H2DEPTH] = 48;
        rv.tp[0][TP_H2CYCLE] = 12;
        rv.tp[0][TP_REVS] = 70;
        rv.tp[0][TP_DLYS] = 50;
        rv.tp[1][TP_LEN] = 8;
        fx_on(1, 0, 36, 110, 4);
        fx_on(1, 3, 36, 80, 2);
        fx_on(1, 5, 43, 90, 2);
        fx_on(1, 6, 34, 100, 4);
        fx_st(1, 6)->to[1] = 1;
        rv.tp[1][TP_SEQMODE] = SEQ_GRAPH;
        rv.tp[1][TP_BRANCH] = 0;                      /* CHANCE */
        rv.tp[1][TP_H1SHAPE] = FG_DECAY;
        rv.tp[1][TP_H1DEPTH] = 63;
        rv.tp[1][TP_H1CYCLE] = 8;
        rv.tp[1][TP_H2ROLE] = ROLE_NOTES;
        rv.tp[1][TP_H2TARGET] = 5;                    /* head 2 plays T5 */
        rv.tp[1][TP_H2RATE] = 6;                      /* x2 */
        rv.tp[1][TP_H2START] = 4;
        rv.tp[1][TP_H2SHAPE] = FG_LFO1;
        rv.tp[1][TP_H2DEPTH] = 40;
        rv.tp[1][TP_H2COMB] = COMB_ADD;
        rv.gp[GP_LFO1RATE] = 30;
        rv.tp[5][TP_LEN] = 16;
        fx_on(5, 4, 74, 60, 2);
        fx_on(5, 12, 79, 50, 2);
        rv.tp[5][TP_REVS] = 60;
        rv.gp[GP_REVSIZE] = 100;
        rv.gp[GP_DLYTIME] = 96;
    } else {
        fx_name("BLANK");
    }
}
