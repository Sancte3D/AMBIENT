/* Actual Product engine. Only Room's memset symbol is observed, in its
 * separately compiled test object; the observer performs the real writes. */
#include "engine.h"
#include "engine_product.h"
#include "ambient_room.h"
#include "dsp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
static unsigned room_bytes,room_calls,max_clear_bytes,total_clear_bytes;
static unsigned case_frames,case_number,block_size;
static FILE *capture,*events;
static bool require_silence;
static int16_t pcm[1024];
typedef struct {unsigned world,block,kind,mute,max_bytes,total_bytes,repeat_bytes,frames;} result_t;
static result_t results[36];
#define CHECK(x) do {if(!(x)){fprintf(stderr,"clear work case=%u line=%d: %s\n",case_number,__LINE__,#x);exit(1);}}while(0)
void *room_clear_observe_memset(void *p,int value,size_t bytes) {
    room_bytes+=(unsigned)bytes;++room_calls;return memset(p,value,bytes);
}
static void hook(int on,uint8_t owner,float hz,float velocity) {
    if(events)fprintf(events,"%u,%u,%d,%u,%.9g,%.9g\n",case_number,case_frames,on,owner,(double)hz,(double)velocity);
}
static void audio(unsigned frames,bool measured) {
    while(frames) {
        unsigned n=frames>block_size?block_size:frames;room_bytes=room_calls=0;
        engine_render(pcm,(int)n);
        if(require_silence)for(unsigned i=0;i<2*n;++i)CHECK(pcm[i]==0);
        if(measured) {
            if(room_bytes>max_clear_bytes)max_clear_bytes=room_bytes;
            total_clear_bytes+=room_bytes;
        }
        if(capture)CHECK(fwrite(pcm,sizeof(int16_t)*2,n,capture)==n);
        case_frames+=n;frames-=n;
        CHECK(engine_nonfinite_samples()==0 && engine_output_limited_samples()==0);
        (void)engine_active_voices();
    }
}
static void one(unsigned world,unsigned block,unsigned kind,bool mute) {
    block_size=block;case_frames=0;max_clear_bytes=total_clear_bytes=0;require_silence=false;
    engine_init();engine_set_world((int)world);engine_set_note_hook(hook);
    engine_set_nature(1);engine_set_room(.5f);
    CHECK(engine_try_note_on(0,dsp_midi_to_hz(50),.8f));audio(4410,false);
    if(kind==1){engine_set_room(0);audio(4410,false);} /* already cold */
    else if(kind==0)engine_set_room(0); /* zero transition coincides with gate */
    else engine_set_fx_mode(0);       /* Dry transition coincides with gate */
    if(mute)engine_set_muted(true);else engine_all_off();
    audio(2646,true);CHECK(!engine_clear_pending());
#ifdef FAM_CLEAR_BASELINE
    CHECK(max_clear_bytes<=121440);
#else
    CHECK(max_clear_bytes<=60720);
#endif
    if(kind==1) {
#ifdef FAM_CLEAR_BASELINE
        CHECK(total_clear_bytes==60720);
#else
        CHECK(total_clear_bytes==0);
#endif
    } else {
#ifdef FAM_CLEAR_BASELINE
        CHECK(total_clear_bytes==121440);
#else
        CHECK(total_clear_bytes==60720);
#endif
    }
    require_silence=true;audio(512,true);require_silence=false;
    room_bytes=room_calls=0;
    for(int i=0;i<20;++i)ambient_room_clear();
    unsigned repeat=room_bytes;
#ifdef FAM_CLEAR_BASELINE
    CHECK(repeat==20*60720u && room_calls==140);
#else
    CHECK(repeat==0 && room_calls==0);
#endif
    if(mute)engine_set_muted(false);
    engine_set_fx_mode(1);engine_set_room(.5f);
    CHECK(engine_try_note_on(0,dsp_midi_to_hz(57),.8f));audio(4410,false);
    CHECK(engine_active_voices()==1);bool audible=false;
    for(unsigned i=0;i<1024;++i)if(pcm[i])audible=true;
    CHECK(audible);
    /* A warm explicit clear still performs all real writes exactly once. */
    room_bytes=room_calls=0;ambient_room_clear();
    CHECK(room_bytes==60720 && room_calls==7);
    results[case_number]=(result_t){world,block,kind,mute,max_clear_bytes,total_clear_bytes,repeat,case_frames};
    ++case_number;
}
int main(int argc,char **argv) {
    CHECK(argc==1 || argc==3);
    if(argc==3){capture=fopen(argv[1],"wb");events=fopen(argv[2],"w");CHECK(capture&&events);}
    for(unsigned w=0;w<3;++w)for(unsigned b=64;b<=512;b*=8)
        for(unsigned kind=0;kind<3;++kind)for(unsigned mute=0;mute<=1;++mute)one(w,b,kind,mute);
    if(capture){CHECK(!fclose(capture));CHECK(!fclose(events));}
    printf("{\"cases\":%u,\"work\":[",case_number);
    for(unsigned i=0;i<case_number;++i){result_t *r=&results[i];
        printf("%s{\"world\":%u,\"block\":%u,\"kind\":%u,\"mute\":%u,\"max_clear_bytes\":%u,\"total_clear_bytes\":%u,\"repeat_clear_bytes\":%u,\"frames\":%u}",i?",":"",r->world,r->block,r->kind,r->mute,r->max_bytes,r->total_bytes,r->repeat_bytes,r->frames);
    }
    puts("]}");return 0;
}
