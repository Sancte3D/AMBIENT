#include "world_grammar.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main(void) {
    unsigned total=0;
    for (int world=0; world<CORE_WORLD_COUNT; ++world) for (uint32_t seed=1;seed<=24;++seed) {
        world_grammar_t g; world_grammar_init(&g,world,seed);
        uint32_t now=0xFFFFA000u;
        unsigned rests=0;
        for (int i=0;i<160;++i) {
            assert(world_grammar_due(&g,now));
            world_grammar_t before=g;
            world_offer_t a=world_grammar_propose(&g,now,0.5f);
            world_offer_t b=world_grammar_propose(&g,now,0.5f);
            assert(!memcmp(&g,&before,sizeof g));
            assert(!memcmp(&a,&b,sizeof a));
            assert(a.gap_ms>=1400 && a.gap_ms<=22000);
            if (a.rest) ++rests;
            else {
                assert(a.index>=0 && a.index<=7);
                if (world==WORLD_WOODLAND && g.phase==1)
                    assert(a.index==g.motif[g.pos]); /* causal heard answer */
                if (world==WORLD_WOODLAND && g.phase==2 && g.pos<g.length-1)
                    assert(a.index==g.motif[g.pos]);
                for (int key=0;key<12;++key) for (int dark=0;dark<2;++dark) {
                    int midi=world_pitch_midi(a.index,key,dark);
                    assert(midi>=50 && midi<=69);
                    assert(world_pitch_count(key,dark)>=8);
                }
            }
            world_grammar_commit(&g,&a,a.index,now);
            assert(!world_grammar_due(&g,now));
            assert(!world_grammar_due(&g,g.next_ms-1));
            now=g.next_ms;
            ++total;
        }
        assert(g.notes>40 && g.episodes>0);
        if (world!=WORLD_COAST) assert(rests>10);
        if (world==WORLD_WOODLAND) assert(g.answers>5);
    }
    /* Activity changes time only, preserving pitch, contour and velocity. */
    for (int w=0;w<CORE_WORLD_COUNT;++w) {
        world_grammar_t g; world_grammar_init(&g,w,13);
        world_offer_t low=world_grammar_propose(&g,0,0), high=world_grammar_propose(&g,0,1);
        assert(low.gap_ms>high.gap_ms);
        assert(low.index==high.index && low.hold_ms==high.hold_ms && low.velocity==high.velocity);
    }
    printf("World grammars: %u deterministic transactions, heard answers, register and timer wrap PASS\n",total);
    return 0;
}
