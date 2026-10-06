#ifndef FAM_WORLD_GRAMMAR_H
#define FAM_WORLD_GRAMMAR_H
#include <stdbool.h>
#include <stdint.h>

/* Product catalogue V1; deliberately separate from the five legacy IDs. */
enum { WORLD_COAST = 0, WORLD_WOODLAND, WORLD_HIGHLANDS, CORE_WORLD_COUNT };
typedef struct {
    uint32_t rng, next_ms, episode_until;
    uint32_t notes, episodes, answers;
    int8_t last, motif[4];
    uint8_t world, phase, pos, length, direction, timing_valid;
} world_grammar_t;
typedef struct {
    world_grammar_t next;
    uint32_t hold_ms, gap_ms;
    float velocity;
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
#endif
