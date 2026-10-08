#ifndef FAM_WORLD_GRAMMAR_H
#define FAM_WORLD_GRAMMAR_H
#include <stdbool.h>
#include <stdint.h>

/* Product catalogue V1; deliberately separate from the five legacy IDs. */
enum { WORLD_COAST = 0, WORLD_WOODLAND, WORLD_HIGHLANDS, CORE_WORLD_COUNT };
typedef struct {
    uint32_t rng, next_ms, episode_until;
    uint32_t notes, episodes, answers, returns, heard_ms;
    uint32_t rhythm[4], memory_rhythm[4];
    int8_t last, motif[4], memory[4], transpose;
    uint8_t world, phase, pos, length, direction, timing_valid;
    uint8_t episode_valid, memory_len, recalled, focus;
    float time_spacing;
} world_grammar_t;
typedef struct {
    world_grammar_t next;
    uint32_t hold_ms, gap_ms;
    float velocity, spacing;
    int index;
    uint8_t role, held_limit;
    bool rest;
} world_offer_t;

void world_grammar_init(world_grammar_t *g, int world, uint32_t seed);
bool world_grammar_due(const world_grammar_t *g, uint32_t now_ms);
/* Pure proposal: failures leave the RNG, heard motif and timing untouched. */
world_offer_t world_grammar_propose(const world_grammar_t *g,
                                  uint32_t now_ms, float activity);
void world_grammar_commit(world_grammar_t *g, const world_offer_t *offer,
                          int heard_index, uint32_t now_ms);
int world_pitch_count(int key_pc, bool minor);
int world_pitch_midi(int index, int key_pc, bool minor);

/* COAST's explicit three-role phrase. Main-only, pure proposals; only a real
 * audio-start acknowledgement advances heard state. Phases 0..2 are outer
 * pairs, phase 3 coalesces their destination, phase 4 is a bounded rest. */
typedef struct {
    uint32_t rng, next_ms, phase_ms, notes, episodes, returns;
    uint8_t phase, heard_mask, timing_valid, variant;
} coast_phrase_t;
typedef struct {
    uint32_t hold_ms, gap_ms;
    int midi;
    float velocity;
    uint8_t phase, role;
} coast_offer_t;
void coast_phrase_init(coast_phrase_t *g, uint32_t seed);
void coast_phrase_restart(coast_phrase_t *g);
bool coast_phrase_due(const coast_phrase_t *g, uint32_t now_ms);
coast_offer_t coast_phrase_propose(const coast_phrase_t *g, int role,
                                  int key_pc, bool minor, float activity);
bool coast_phrase_heard(coast_phrase_t *g, const coast_offer_t *offer,
                        uint32_t now_ms);
bool coast_phrase_pitch_allowed(int midi, int key_pc, bool minor);

/* WOODLAND: two long owned strings; replace one chord tone while the other
 * stays. Six heard starts, then rest. Offers/acks are Main-only and bounded. */
typedef struct {
    uint32_t rng,next_ms,notes,episodes,answers,returns;
    uint8_t phase,timing_valid,variant;
} woodland_phrase_t;
typedef struct {
    uint32_t hold_ms,gap_ms;
    int midi;
    float velocity;
    uint8_t phase,role;
} woodland_offer_t;
void woodland_phrase_init(woodland_phrase_t *g,uint32_t seed);
void woodland_phrase_restart(woodland_phrase_t *g);
bool woodland_phrase_due(const woodland_phrase_t *g,uint32_t now_ms);
woodland_offer_t woodland_phrase_propose(const woodland_phrase_t *g,
                                       int key_pc,bool minor,float activity);
bool woodland_phrase_heard(woodland_phrase_t *g,const woodland_offer_t *o,
                          uint32_t now_ms);
bool woodland_phrase_pitch_allowed(int midi,int key_pc,bool minor);

/* HIGHLANDS: wide chord -> related voicing with the same held third ->
 * actual retirement, open pause, reprise. Eight starts, three owned roles.
 * Main acknowledges starts and begins silence only after all owners retire. */
typedef struct {
    uint32_t rng,next_ms,notes,episodes,returns;
    uint8_t phase,timing_valid,rest_valid,variant;
} highlands_phrase_t;
typedef struct {
    uint32_t hold_ms,gap_ms;
    int midi;
    float velocity;
    uint8_t phase,role;
} highlands_offer_t;
void highlands_phrase_init(highlands_phrase_t *g,uint32_t seed);
void highlands_phrase_restart(highlands_phrase_t *g);
bool highlands_phrase_due(const highlands_phrase_t *g,uint32_t now_ms);
highlands_offer_t highlands_phrase_propose(const highlands_phrase_t *g,
                                         int key_pc,bool minor,float activity);
bool highlands_phrase_heard(highlands_phrase_t *g,const highlands_offer_t *o,
                           uint32_t now_ms);
/* Engine calls only at phase 5/8 after actual sources have retired. */
bool highlands_phrase_pause(highlands_phrase_t *g,uint32_t now_ms,float activity);
bool highlands_phrase_pitch_allowed(int midi,int key_pc,bool minor);
#endif
