/* Read-only host observer for the real Product engine. Only the engine object
 * renames its two bus calls; these wrappers call the unmodified implementations.
 * No alternate source, effect, score or target-side hook is introduced. */
#include "engine.h"
#include "engine_product.h"
#include "ambient_room.h"
#include "nature.h"
#include "tuning.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { SR=44100,TICK=441,SECONDS=40,STEMS=3 };
typedef struct {double energy[STEMS],pcm,mono;float peak;unsigned samples;} second_t;
static second_t seconds[SECONDS];
static float direct[2][512],returned[2][512],dc[STEMS][2];
static uint32_t observed,frame;
static unsigned maximum,starts,stops;
static uint64_t hashes[STEMS],events_hash;
static bool collecting=true,pending;
static FILE *pcm_file;
static const uint64_t offset=UINT64_C(14695981039346656037);
static void byte_hash(uint64_t *h,unsigned x) {*h=(*h^(x&255))*UINT64_C(1099511628211);}
static void word_hash(uint64_t *h,uint32_t x) {for(int b=0;b<4;++b)byte_hash(h,x>>(8*b));}
static void float_hash(uint64_t *h,float x) {uint32_t bits;memcpy(&bits,&x,4);word_hash(h,bits);}
static void heard(int on,uint8_t owner,float hz,float velocity) {
    word_hash(&events_hash,frame);word_hash(&events_hash,(uint32_t)on);word_hash(&events_hash,owner);
    float_hash(&events_hash,hz);float_hash(&events_hash,velocity);
    if(on>0)++starts;
    if(on==0)++stops;
}
void hierarchy_room_process(float *l,float *r,const float *sl,const float *sr,int n) {
    if(!collecting) {ambient_room_process(l,r,sl,sr,n);return;}
    assert(n>0 && n<=512 && !pending);
    memcpy(direct[0],l,(size_t)n*sizeof(float));memcpy(direct[1],r,(size_t)n*sizeof(float));
    ambient_room_process(l,r,sl,sr,n);
    for(int i=0;i<n;++i) {returned[0][i]=l[i]-direct[0][i];returned[1][i]=r[i]-direct[1][i];}
    pending=true;
}
void hierarchy_nature_render_music(float *l,float *r,const float *ml,const float *mr,float gain,int n) {
    if(!collecting) {nature_render_music(l,r,ml,mr,gain,n);return;}
    assert(pending && observed+(unsigned)n<=SECONDS*SR);
    float before[2][512];memcpy(before[0],l,(size_t)n*sizeof(float));memcpy(before[1],r,(size_t)n*sizeof(float));
    nature_render_music(l,r,ml,mr,gain,n);
    for(int i=0;i<n;++i)for(int c=0;c<2;++c) {
        float final=c ? r[i] : l[i];
        float value[STEMS]={direct[c][i],returned[c][i],final-before[c][i]};
        for(int k=0;k<STEMS;++k) {
            float_hash(hashes+k,value[k]);
            /* Match the actual engine's 16Hz DC-tracker coefficient, per stem.
             * Ratios are before common Volume/gate and final PCM quantization. */
            dc[k][c]+=.00228f*(value[k]-dc[k][c]);
            double ac=(double)(value[k]-dc[k][c]);
            seconds[(observed+(unsigned)i)/SR].energy[k]+=ac*ac;
        }
        ++seconds[(observed+(unsigned)i)/SR].samples;
    }
    observed+=(unsigned)n;pending=false;
}
static void output(const int16_t *pcm,int n) {
    unsigned char bytes[2048];
    for(int i=0;i<n;++i) {
        second_t *s=seconds+(frame+(unsigned)i)/SR;
        double l=pcm[2*i]/32768.0,r=pcm[2*i+1]/32768.0;
        s->pcm+=l*l+r*r;s->mono+=(l+r)*(l+r)*.25;
        s->peak=fmaxf(s->peak,fmaxf((float)fabs(l),(float)fabs(r)));
        for(int c=0;c<2;++c) {
            uint16_t v=(uint16_t)pcm[2*i+c];bytes[4*i+2*c]=(unsigned char)v;bytes[4*i+2*c+1]=(unsigned char)(v>>8);
        }
    }
    assert(fwrite(bytes,4,(size_t)n,pcm_file)==(size_t)n);
}
static void plain_tick(unsigned ms) {
    int16_t pcm[2*TICK];engine_generative_tick(ms);engine_render(pcm,TICK);
}
static void policy(void) {
    collecting=false;
    puts("{\"source_limits\":[");
    for(int w=0;w<3;++w) {
        engine_init();engine_set_world(w);
        unsigned admitted=0;static const int notes[4]={50,57,62,66};
        for(unsigned k=0;k<4;++k) {
            if(engine_try_note_on((uint8_t)k,tuning_hz((float)notes[k]),.7f))++admitted;
            plain_tick(k*10);
        }
        unsigned expected=w==1 ? 2 : 3;assert(admitted==expected);
        engine_set_generative(true,-1);
        assert(!engine_try_note_on(4,tuning_hz(69),.7f));
        printf("%s{\"world\":%d,\"admitted_of_four\":%u,\"manual_blocked_in_generate\":true}",w ? ",\n" : "",w,admitted);
    }
    puts("],\"silent_freeze\":[");
    for(int freeze=0;freeze<=1;++freeze) {
        engine_init();engine_set_master_volume(.6f);
        sound_parameters_t p=sound_parameters_default();p.delay=p.grain=p.drive=p.crush=p.phaser=0;p.plate=false;
        assert(engine_apply_sound_parameters(&p));assert(engine_try_note_on(0,tuning_hz(50),.5f));
        for(unsigned ms=0;ms<2000;ms+=10)plain_tick(ms);
        engine_note_off(0);p.freeze=freeze!=0;assert(engine_apply_sound_parameters(&p));
        int peak=0;int16_t pcm[2*TICK];
        for(unsigned ms=2000;ms<62000;ms+=10) {
            engine_generative_tick(ms);engine_render(pcm,TICK);
            if(ms>=61000)for(unsigned i=0;i<2*TICK;++i)if(abs(pcm[i])>peak)peak=abs(pcm[i]);
        }
        engine_generative_tick(62000);float ledger[32];int retained=engine_sounding_frequencies(ledger,32);
        int voices=engine_active_voices();engine_set_key_pc(4);
        bool accepted=engine_try_note_on(1,tuning_hz(52),.5f);
        sound_parameters_t actual;engine_get_sound_parameters(&actual);
        assert(peak==0 && voices==0 && retained==0 && accepted && !actual.freeze);
        printf("%s{\"freeze\":%s,\"grain\":0,\"active_voices\":%d,\"retained_pitches\":%d,\"last_second_pcm_peak\":%d,\"new_E3_accepted\":%s}",
               freeze ? ",\n" : "",freeze ? "true" : "false",voices,retained,peak,accepted ? "true" : "false");
    }
    puts("]}");
}
int main(int argc,char **argv) {
    if(argc==2 && !strcmp(argv[1],"--policy")) {policy();return 0;}
    if(argc!=12)return 2;
    int world=atoi(argv[2]),midi=atoi(argv[3]),plate=atoi(argv[5]),profile=atoi(argv[8]),cap=atoi(argv[9]),density=atoi(argv[10]);
    float velocity=strtof(argv[4],NULL),room=strtof(argv[6],NULL),nature=strtof(argv[7],NULL);
    uint32_t seed=(uint32_t)strtoul(argv[11],NULL,10);
    if(world<0 || world>2 || midi<50 || midi>69 || plate<0 || plate>1 || profile<0 || profile>9 || cap<1 || cap>512 || density<1 || density>(world==1 ? 2 : 3))return 2;
    if(!isfinite(velocity) || velocity<=0 || velocity>1 || !isfinite(room) || room<0 || room>1 || !isfinite(nature) || nature<0 || nature>1)return 2;
    pcm_file=fopen(argv[1],"wb");if(!pcm_file)return 2;
    for(int k=0;k<STEMS;++k)hashes[k]=offset;
    events_hash=offset;
    engine_init();engine_set_world(world);engine_set_gen_seed(seed);engine_set_room(room);engine_set_nature(nature);
    engine_set_master_volume(1);engine_set_note_hook(heard);
    sound_parameters_t p=sound_parameters_default();p.plate=plate!=0;
    if(profile==1 || profile==9)p.delay=p.grain=p.drive=p.crush=p.phaser=0;
    if(profile==2)p.delay=1;
    if(profile==3 || profile==8)p.grain=1;
    if(profile==4)p.drive=1;
    if(profile==5)p.crush=1;
    if(profile==6)p.phaser=1;
    if(profile==7) {
        p.delay=p.grain=p.drive=p.crush=p.phaser=1;p.feedback=.5f;
        p.delay_left_ms=40;p.delay_right_ms=700;p.grain_ms=40;p.phaser_hz=.5f;
    }
    if(profile==9)p.drive=p.crush=p.phaser=1;
    assert(engine_apply_sound_parameters(&p));
    for(frame=0;frame<SECONDS*SR;) {
        if(frame%TICK==0) {
            unsigned ms=(unsigned)((uint64_t)frame*1000/SR);
            if(ms==2000)for(int k=0;k<density;++k) {
                int note=density==1 ? midi : (int[]){50,57,62}[k];
                assert(engine_try_note_on((uint8_t)k,tuning_hz((float)note),velocity));engine_set_cell_held((uint8_t)k,true);
            }
            if(ms==10000 && profile==8) {p.freeze=true;assert(engine_apply_sound_parameters(&p));}
            if(ms==26000)for(int k=0;k<density;++k) {engine_set_cell_held((uint8_t)k,false);engine_note_off((uint8_t)k);}
            engine_generative_tick(ms);
            unsigned count=(unsigned)engine_active_voices();if(count>maximum)maximum=count;
            assert(count<=(unsigned)(world==1 ? 2 : 3));
        }
        int n=(int)((frame/TICK+1)*TICK-frame);if(n>cap)n=cap;
        int16_t pcm[1024];engine_render(pcm,n);output(pcm,n);frame+=(unsigned)n;
    }
    assert(starts==(unsigned)density && stops==(unsigned)density);
    assert(!engine_nonfinite_samples());
#ifdef HIERARCHY_OBSERVER
    assert(observed==SECONDS*SR && !pending);
#endif
    assert(!fclose(pcm_file));
    printf("{\"starts\":%u,\"stops\":%u,\"maximum_sources\":%u,\"limited_samples\":%u,\"nonfinite_samples\":%u,\"event_fingerprint\":\"%016llx\",\"stem_fingerprints\":[",
           starts,stops,maximum,engine_output_limited_samples(),engine_nonfinite_samples(),(unsigned long long)events_hash);
    for(int k=0;k<STEMS;++k)printf("%s\"%016llx\"",k ? "," : "",(unsigned long long)hashes[k]);
    fputs("],\"seconds\":[",stdout);
    for(int t=0;t<SECONDS;++t) {
        second_t *s=seconds+t;printf("%s[",t ? "," : "");
        for(int k=0;k<STEMS;++k)printf("%.12g,",s->samples ? sqrt(s->energy[k]/s->samples) : 0);
        printf("%.12g,%.12g,%.12g]",sqrt(s->pcm/(2*SR)),sqrt(s->mono/SR),(double)s->peak);
    }
    puts("]}");return 0;
}
