/*
 * scenes.c — 5 Scene-Slots + Scenes-UI. Siehe scenes.h.
 */

#include "scenes.h"
#include "menu.h"
#include "synth_controls.h"
#include "params.h"
#include "engine.h"
#ifdef FAM_SOUND_PRODUCT
#include "engine_product.h"
#include <stddef.h>
#endif
#include "oled.h"
#include "baked_font.h"
#include <string.h>
#include <stdio.h>

#ifdef FAM_SOUND_PRODUCT
#define SCENE_MAGIC 0x53434E37u /* SCN7: three Worlds, Dry/Room, collection */
#else
#define SCENE_MAGIC 0x53434E36u
#endif

typedef struct {
    uint32_t     magic;           /* SCENE_MAGIC = belegt                  */
    menu_state_t menu;
    int8_t       drive_pct;
#ifdef FAM_SOUND_PRODUCT
    uint8_t      collection;      /* 0 Major / 1 Minor; occupies SCN6 padding */
#endif
    int16_t      bright_hz;       /* -600..800                             */
    uint32_t     gen_seed;
} scene_slot_t;

typedef struct {
    uint32_t     magic;           /* Store-Gueltigkeit                     */
    scene_slot_t slot[SCENES_COUNT];
#ifdef FAM_SOUND_PRODUCT
    uint32_t checksum; /* Covers header and every slot, including fixed padding. */
#endif
} scene_store_t;
_Static_assert(sizeof(scene_store_t)<=512, "Scene store exceeds flash staging buffer");

/* Exact SCN5 wire layout. Read only; old presets gain neutral shape/core
 * defaults and are written as SCN6 only when the player next saves. */
typedef struct { uint8_t values[16]; uint16_t locks; } legacy_menu_t;
typedef struct {
    uint32_t magic; legacy_menu_t menu; int8_t drive_pct; int16_t bright_hz; uint32_t gen_seed;
} legacy_slot_t;
typedef struct { uint32_t magic; legacy_slot_t slot[SCENES_COUNT]; } legacy_store_t;

#ifdef FAM_SOUND_PRODUCT
/* Exact SCN6 ABI: never reinterpret legacy IDs as product IDs. */
typedef struct {
    uint8_t world,key_pc,tuning,voice,synth,cell,bass,color,fx;
    uint8_t space,shimmer,atmos,motion,age,echo,blur;
    uint16_t locks;
    uint8_t reso,attack,release,sweep,envmod,core[6][6];
} scn6_menu_t;
typedef struct {
    uint32_t magic; scn6_menu_t menu; int8_t drive_pct;
    int16_t bright_hz; uint32_t gen_seed;
} scn6_slot_t;
typedef struct { uint32_t magic; scn6_slot_t slot[SCENES_COUNT]; } scn6_store_t;
_Static_assert(sizeof(scn6_menu_t)==60 && sizeof(scn6_slot_t)==72, "SCN6 wire changed");
_Static_assert(offsetof(scn6_slot_t,bright_hz)==66 && offsetof(scn6_slot_t,gen_seed)==68, "SCN6 offsets changed");
_Static_assert(sizeof(legacy_slot_t)==32, "SCN5 wire changed");
static uint8_t cap100(uint8_t v) { return v>100 ? 100 : v; }
static void migrate_legacy(scene_slot_t *sl,int old_world) {
    static const uint8_t world_map[5]={2,0,0,1,1};
    sl->menu.world=old_world>=0 && old_world<5 ? world_map[old_world] : 0;
    sl->collection=0x80u | ((old_world>=2 && old_world<=4) ? 1u : 0u);
    sl->menu.fx=sl->menu.fx==0 ? 0 : 1;
    sl->menu.key_pc=sl->menu.key_pc<12 ? sl->menu.key_pc : 2;
    sl->menu.tuning=sl->menu.tuning==1 ? 1 : 0;
    sl->menu.space=cap100(sl->menu.space);
    sl->menu.atmos=0; sl->menu.motion=50; /* old noise/LFO targets aren't equivalent */
    sl->menu.attack=cap100(sl->menu.attack); sl->menu.release=cap100(sl->menu.release);
    sl->menu.voice=sl->menu.synth=sl->menu.cell=sl->menu.bass=sl->menu.color=0;
    sl->menu.shimmer=sl->menu.age=sl->menu.echo=sl->menu.blur=0;
    sl->menu.reso=sl->menu.sweep=sl->menu.envmod=0; sl->menu.locks=0;
    memset(sl->menu.core,0,sizeof sl->menu.core);
    sl->drive_pct=0;
    if(sl->bright_hz < -600) sl->bright_hz=-600;
    if(sl->bright_hz > 800) sl->bright_hz=800;
    sl->magic=SCENE_MAGIC;
}
static uint32_t store_checksum(const scene_store_t *s) {
    const uint8_t *p=(const uint8_t *)s; uint32_t crc=0xffffffffu;
    for(unsigned i=0;i<offsetof(scene_store_t,checksum);++i) {
        crc^=p[i];
        for(int b=0;b<8;++b) crc=(crc>>1)^(0xedb88320u & (0u-(crc&1u)));
    }
    return ~crc;
}
#endif

static scene_store_t   s_store;
static scenes_write_fn s_write;
static scenes_read_fn  s_read;
static int             s_active = -1;

static bool     s_ui_on = false;
static uint32_t s_ui_last;

#ifdef FAM_SOUND_PRODUCT
void scenes_init(scenes_write_fn write_fn, scenes_read_fn read_fn) {
    s_write=write_fn; s_read=read_fn; s_active=-1; s_ui_on=false;
    memset(&s_store,0,sizeof s_store);
    if(s_read && s_read(&s_store,sizeof s_store) && s_store.magic==SCENE_MAGIC && s_store.checksum==store_checksum(&s_store)) return;
    memset(&s_store,0,sizeof s_store); s_store.magic=SCENE_MAGIC;
    scn6_store_t old6={0};
    if(s_read && s_read(&old6,sizeof old6) && old6.magic==0x53434E36u) {
        for(int i=0;i<SCENES_COUNT;++i) if(old6.slot[i].magic==0x53434E36u) {
            scene_slot_t *sl=&s_store.slot[i];
            memcpy(&sl->menu,&old6.slot[i].menu,sizeof sl->menu);
            sl->bright_hz=old6.slot[i].bright_hz; sl->gen_seed=old6.slot[i].gen_seed;
            migrate_legacy(sl,old6.slot[i].menu.world);
        }
        return;
    }
    legacy_store_t old5={0};
    if(s_read && s_read(&old5,sizeof old5) && old5.magic==0x53434E35u) {
        for(int i=0;i<SCENES_COUNT;++i) if(old5.slot[i].magic==0x53434E35u) {
            scene_slot_t *sl=&s_store.slot[i];
            memcpy(&sl->menu,&old5.slot[i].menu,sizeof old5.slot[i].menu);
            sl->menu.attack=sl->menu.release=50;
            sl->bright_hz=old5.slot[i].bright_hz; sl->gen_seed=old5.slot[i].gen_seed;
            migrate_legacy(sl,old5.slot[i].menu.values[0]);
        }
    }
}
bool scenes_migrated(int slot) { return slot>=0 && slot<SCENES_COUNT && (s_store.slot[slot].collection & 0x80u)!=0; }
#else
void scenes_init(scenes_write_fn write_fn, scenes_read_fn read_fn) {
    s_write = write_fn;
    s_read  = read_fn;
    s_active = -1;
    s_ui_on  = false;
    memset(&s_store, 0, sizeof s_store);
    if (s_read && s_read(&s_store, sizeof s_store) &&
        s_store.magic == SCENE_MAGIC) {
        /* geladener Store ist gueltig — Slots behalten ihre Magic-Marken */
    } else {
        memset(&s_store, 0, sizeof s_store);
        s_store.magic = SCENE_MAGIC;
        legacy_store_t old;
        if (s_read && s_read(&old, sizeof old) && old.magic == 0x53434E35u) {
            for (int i=0;i<SCENES_COUNT;++i) if (old.slot[i].magic == 0x53434E35u) {
                scene_slot_t *sl=&s_store.slot[i];
                sl->magic=SCENE_MAGIC;
                memcpy(&sl->menu, &old.slot[i].menu, sizeof(legacy_menu_t));
                sl->menu.attack=sl->menu.release=50;
                memcpy(sl->menu.core, synth_control_defaults, sizeof sl->menu.core);
                sl->drive_pct=old.slot[i].drive_pct; sl->bright_hz=old.slot[i].bright_hz;
                sl->gen_seed=old.slot[i].gen_seed;
            }
        }
    }
}
#endif

bool scenes_used(int slot) {
    return slot >= 0 && slot < SCENES_COUNT &&
           s_store.slot[slot].magic == SCENE_MAGIC;
}

int scenes_active(void) { return s_active; }

bool scenes_save(int slot, uint32_t now_ms) {
    if (slot < 0 || slot >= SCENES_COUNT) return false;
    scene_slot_t *sl = &s_store.slot[slot];
    scene_slot_t previous=*sl;
    sl->magic = SCENE_MAGIC;
    menu_get_state(&sl->menu);
    sl->drive_pct = (int8_t)params_drive_pct();
    sl->bright_hz = (int16_t)params_bright_hz();
    sl->gen_seed  = engine_gen_seed();
#ifdef FAM_SOUND_PRODUCT
    sl->drive_pct=0;
    sl->collection=(uint8_t)engine_product_collection();
    uint32_t previous_checksum=s_store.checksum;
    s_store.checksum=store_checksum(&s_store);
#endif
    if(s_write && !s_write(&s_store,sizeof s_store)) {
        *sl=previous;
#ifdef FAM_SOUND_PRODUCT
        s_store.checksum=previous_checksum;
#endif
        return false;
    }
    s_active  = slot;
    s_ui_last = now_ms;
    return true;
}

bool scenes_recall(int slot, uint32_t now_ms) {
    if (!scenes_used(slot)) return false;
    const scene_slot_t *sl = &s_store.slot[slot];
#ifdef FAM_SOUND_PRODUCT
    engine_set_mode(sl->collection & 1u);
#endif
    menu_apply_state(&sl->menu);
    params_apply_scene(sl->drive_pct, (float)sl->bright_hz);
    engine_set_gen_seed(sl->gen_seed);
    s_active  = slot;
    s_ui_last = now_ms;
    return true;
}

/* --- Scenes-UI ----------------------------------------------------------- */

void scenes_ui_open(uint32_t now_ms) { s_ui_on = true;  s_ui_last = now_ms; }
void scenes_ui_close(void)           { s_ui_on = false; }
bool scenes_ui_active(void)          { return s_ui_on; }

void scenes_ui_tick(uint32_t now_ms) {
    if (s_ui_on && (uint32_t)(now_ms - s_ui_last) >= SCENES_UI_IDLE_MS)
        s_ui_on = false;
}

void scenes_ui_cell(uint8_t cell, bool shift, uint32_t now_ms) {
    if (!s_ui_on || cell >= SCENES_COUNT) return;
    if (shift) scenes_save((int)cell, now_ms);
    else       (void)scenes_recall((int)cell, now_ms);
    s_ui_last = now_ms;
}

/* Screen: "SCENES" oben, 5 Slot-Pillen unten (leer/belegt/aktiv) —
 * gleiche Typo/Idiome wie menu.c, aber ohne dessen Interna. */
#define SC_GS_BG     0
#define SC_GS_DIM    5
#define SC_GS_LABEL  9
#define SC_GS_VALUE  13
#define SC_GS_ACTIVE 15
#define SC_PAD_L     22
#define SC_PAD_T     14

void scenes_ui_render(void) {
    oled_fill(SC_GS_BG);
    bfont_draw(&font_hn_label, SC_PAD_L, SC_PAD_T, "SCENES", SC_GS_LABEL);

    /* Hinweistext: Cell = laden, SHIFT+Cell = speichern */
    bfont_draw(&font_hn_label, SC_PAD_L, SC_PAD_T + (int)font_hn_label.line + 4,
               "CELL = LOAD   SHIFT+CELL = SAVE", SC_GS_DIM);

    /* 5 Slot-Pillen, mittig — Hoehe/Idiom wie die Menue-Bar. */
    const int pill_w = 34, pill_h = 22, gap = 10;
    int total = SCENES_COUNT * pill_w + (SCENES_COUNT - 1) * gap;
    int x0 = (OLED_WIDTH - total) / 2;
    int y0 = (OLED_HEIGHT - pill_h) / 2 + 8;
    for (int i = 0; i < SCENES_COUNT; ++i) {
        int x = x0 + i * (pill_w + gap);
        bool used   = scenes_used(i);
        bool active = (scenes_active() == i) && used;
        if (active)      oled_rrect_fill(x, y0, pill_w, pill_h, 6, SC_GS_ACTIVE);
        else if (used)   oled_rrect_fill(x, y0, pill_w, pill_h, 6, SC_GS_DIM);
        else             oled_rrect_fill(x, y0 + pill_h/2 - 1, pill_w, 2, 1, SC_GS_DIM);
        char n[2] = { (char)('1' + i), 0 };
        int tw = bfont_width(&font_hn_label, n);
        bfont_draw(&font_hn_label, x + (pill_w - tw) / 2,
                   y0 + (pill_h - (int)font_hn_label.line) / 2,
                   n, active ? SC_GS_BG : SC_GS_VALUE);
    }
}
