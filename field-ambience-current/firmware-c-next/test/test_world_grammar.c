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
    /* If admission selects an adjusted pitch, the answer remembers what
     * actually sounded rather than the unplayed proposed degree. */
    world_grammar_t adjusted; world_grammar_init(&adjusted,WORLD_WOODLAND,9);
    world_offer_t first=world_grammar_propose(&adjusted,0,0.5f);
    int heard=first.index==7 ? 6 : first.index+1;
    world_grammar_commit(&adjusted,&first,heard,0);
    assert(adjusted.motif[0]==heard && adjusted.last==heard);
    while(adjusted.phase==0) {
        world_offer_t next=world_grammar_propose(&adjusted,adjusted.next_ms,0.5f);
        world_grammar_commit(&adjusted,&next,next.index,adjusted.next_ms);
    }
    world_offer_t answer=world_grammar_propose(&adjusted,adjusted.next_ms,0.5f);
    assert(answer.index==heard);
    /* Heard intervals (including real admission delay) feed the answer.
     * Rejected proposals neither manufacture memory nor advance return counts. */
    assert(adjusted.memory_len==adjusted.length && adjusted.memory[0]==heard);
    assert(adjusted.rhythm[0]>=1400);
    uint32_t returns=0;
    for(int w=1;w<CORE_WORLD_COUNT;++w) for(uint32_t seed=1;seed<=24;++seed) {
        world_grammar_t g;world_grammar_init(&g,w,seed);
        uint32_t now=0;
        for(int i=0;i<400;++i) {
            world_grammar_t before=g;
            world_offer_t o=world_grammar_propose(&g,now,.5f);
            assert(!memcmp(&before,&g,sizeof g));
            if(g.phase==0 && g.pos==0 && o.next.recalled && !o.rest)
                assert(o.index==g.memory[0]+o.next.transpose);
            world_grammar_commit(&g,&o,o.index,now);
            if(g.memory_len) {
                assert(g.memory_len>=2 && g.memory_len<=4);
                for(int k=0;k<g.memory_len;++k)assert(g.memory[k]>=0 && g.memory[k]<=7);
            }
            now=g.next_ms;
        }
        assert(g.returns>0);returns+=g.returns;
    }
    /* A real deadline equal to zero at wrap must not act as an init sentinel. */
    world_grammar_t wrapped;world_grammar_init(&wrapped,WORLD_COAST,7);
    wrapped.episode_valid=1;wrapped.episode_until=0;wrapped.phase=1;
    world_offer_t rest=world_grammar_propose(&wrapped,0,.5f);
    assert(rest.rest && rest.next.episodes==1);
    printf("World memory: %u heard figure returns, interval memory and exact-zero timer wrap PASS\\n",returns);
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
