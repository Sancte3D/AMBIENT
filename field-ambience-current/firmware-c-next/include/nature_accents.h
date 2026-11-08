/* Private implementation, included only by nature.c in the complete sound
 * hearing candidate. Unpitched procedural weather, never a musical source.
 * Main publishes three independent 32-bit targets; all weather state and RNG
 * belong to audio. No allocation, sample buffers or per-sample transcendental. */
#ifndef FAM_NATURE_ACCENTS_H
#define FAM_NATURE_ACCENTS_H

#define NA_SR ((float)DSP_SAMPLE_RATE_HZ)
#ifdef FAM_NATURE_ACCENTS_TEST
extern void nature_accent_event(unsigned kind,unsigned profile,uint32_t frames);
extern void nature_accent_thunder_sample(float value);
#define NA_EVENT(k,p,n) nature_accent_event(k,p,n)
#define NA_THUNDER(x) nature_accent_thunder_sample(x)
#else
#define NA_EVENT(k,p,n) ((void)0)
#define NA_THUNDER(x) ((void)0)
#endif

typedef struct {
    uint32_t age,up,hold,down,total,gap;
    float inv_up,inv_down,peak;
} na_pulse_t;
typedef struct {
    uint32_t age,length,rise,rng;
    float inv_up,inv_down,peak,pan;
} na_drop_t;
static struct {
    uint32_t seed,wind_rng,surf_rng,rain_rng,thunder_rng,ctrl;
    int world;
    bool idle;
    float amount,transition,wind_cutoff,wind_target,eddy,eddy_target,pan;
    float wind_dc[2],surf_dc[2],rain_dc[2],thunder_dc[2],foam;
    na_pulse_t wind,surf,rain,thunder;
    dsp_pink_t wind_noise[2],water_noise[2],thunder_noise[2];
    dsp_svf_t wind_lp[2],surf_lp[2],foam_lp[2],rain_lp[2],thunder_lp[2];
    na_drop_t drops[8];
} na;
static _Atomic uint32_t na_amount_bits,na_requested_seed;
static _Atomic int na_requested_world;
static struct {
    float dc[3][2],power[3],gain,goal;
    uint32_t tick,quiet;
    bool active;
} na_music;

/* Return-only hierarchy: about -28dB RMS under an ongoing direct Cell.
 * Fast attenuation, two-second recovery; the source never changes gain or
 * length. After the actual reference has died away, weather returns gently.
 * Per-sample state and a /64 goal clock keep all render partitions identical. */
static float na_background_gain(float dl,float dr,float ml,float mr,float wl,float wr) {
    float input[3][2]={{dl,dr},{wl,wr},{ml,mr}},power[3]={0,0,0};
    for(int k=0;k<3;++k)for(int c=0;c<2;++c) {
        na_music.dc[k][c]+=.00228f*(input[k][c]-na_music.dc[k][c]);
        float x=input[k][c]-na_music.dc[k][c];power[k]+=.5f*x*x;
    }
    for(int k=0;k<3;++k)na_music.power[k]+=(power[k]-na_music.power[k])*(1.0f/(.020f*NA_SR));
    /* Keep a surviving Freeze/room return in front after the direct sources
     * retire. While Dry is present, its level remains the stricter reference. */
    float reference=na_music.power[0]>=1e-12f ? na_music.power[0] : na_music.power[2];
    if(reference>1e-10f) {na_music.active=true;na_music.quiet=0;}
    else if(reference<1e-12f) {
        if(na_music.quiet<(uint32_t)(.250f*NA_SR))++na_music.quiet;
        else na_music.active=false;
    } else na_music.quiet=0;
    if((na_music.tick++&63u)==0) {
        na_music.goal=1;
        if(na_music.active && na_music.power[1]>1e-20f)
            na_music.goal=fminf(1,.04f*sqrtf(reference/na_music.power[1]));
    }
    float seconds=na_music.goal<na_music.gain ? .010f : 2;
    na_music.gain+=(na_music.goal-na_music.gain)*(1.0f/(seconds*NA_SR));
    return na_music.gain;
}

static uint32_t na_next(uint32_t *rng) {
    *rng=*rng*1664525u+1013904223u;return *rng;
}
static float na_random(uint32_t *rng) {return (float)(na_next(rng)>>8)*(1.0f/16777216.0f);}
static float na_white(uint32_t *rng) {return (float)(int32_t)na_next(rng)*(1.0f/2147483648.0f);}
static uint32_t na_seconds(uint32_t *rng,float lo,float range) {
    return (uint32_t)(NA_SR*(lo+range*na_random(rng)));
}
static float na_round(float x) {return x*x*(3.0f-2.0f*x);}
/* Soft, explicit amplitude ceilings, including simultaneous weather events.
 * The complete layer stays strictly below .004 (-48 dBFS) before shared
 * Master/DC, even at Nature100%. Thunder's own ceiling is .0007 (-63 dBFS).
 * Normal events stay well below these bounds; no automatic makeup gain. */
static float na_bound(float x,float limit) {return x/(1.0f+fabsf(x)/limit);}
static void na_start(na_pulse_t *p,uint32_t *rng,float up,float up_range,
                     float hold,float hold_range,float down,float down_range,
                     float gap,float gap_range,float peak) {
    p->age=0;p->up=na_seconds(rng,up,up_range);
    p->hold=na_seconds(rng,hold,hold_range);p->down=na_seconds(rng,down,down_range);
    p->total=p->up+p->hold+p->down;p->gap=na_seconds(rng,gap,gap_range);
    p->inv_up=1.0f/(float)p->up;p->inv_down=1.0f/(float)p->down;p->peak=peak;
}
static bool na_ready(const na_pulse_t *p) {return p->age>=p->total && !p->gap;}
static float na_tick(na_pulse_t *p) {
    if(p->age>=p->total) {if(p->gap)--p->gap;return 0;}
    uint32_t age=p->age++;
    if(age<p->up)return p->peak*na_round((float)age*p->inv_up);
    age-=p->up;
    if(age<p->hold)return p->peak;
    return p->peak*(1.0f-na_round((float)(age-p->hold)*p->inv_down));
}
static void na_reset(int world,uint32_t seed) {
    memset(&na,0,sizeof na);na.world=world;na.seed=seed;
    na.wind_rng=seed^0x91BC24E3u;na.surf_rng=seed^0x0C011A57u;
    na.rain_rng=seed^0xB3E27C49u;na.thunder_rng=seed^0xA9175D02u;
    na.wind.gap=na_seconds(&na.wind_rng,.8f,2.2f);
    na.surf.gap=na_seconds(&na.surf_rng,1.5f,2.0f);
    na.rain.gap=na_seconds(&na.rain_rng,1,2);
    na.thunder.gap=na_seconds(&na.thunder_rng,18,12);
    na.wind_cutoff=na.wind_target=800;
    for(int ch=0;ch<2;++ch) {
        dsp_pink_seed(&na.wind_noise[ch],seed^(0xACE12345u+0x3179ABu*(uint32_t)ch));
        dsp_pink_seed(&na.water_noise[ch],seed^(0x56ED12u+0x9E3779B9u*(uint32_t)ch));
        dsp_pink_seed(&na.thunder_noise[ch],seed^(0x7B19F88Au+0x41BC76u*(uint32_t)ch));
        dsp_svf_set(&na.wind_lp[ch],800,.6f);
        dsp_svf_set(&na.surf_lp[ch],ch ? 620 : 580,.6f);
        dsp_svf_set(&na.foam_lp[ch],ch ? 2200 : 2000,.6f);
        dsp_svf_set(&na.rain_lp[ch],ch ? 3000 : 2700,.6f);
        dsp_svf_set(&na.thunder_lp[ch],ch ? 170 : 150,.6f);
    }
    for(unsigned i=0;i<8;++i)na.drops[i].rng=na_next(&na.rain_rng);
}
static float na_wind(void) {
    if(na_ready(&na.wind)) {
        unsigned type=na_next(&na.wind_rng)%3u;
        /* A long soft breath, a warm rolling gust, or a shorter airy breeze.
         * Each has its own fresh duration, colour and subsequent quiet gap. */
        float peak=.45f+.4f*na_random(&na.wind_rng);
        if(type==0) {
            na_start(&na.wind,&na.wind_rng,2,2,0,2,3,3,3,10,peak*.8f);
            na.wind_target=650+350*na_random(&na.wind_rng);
        } else if(type==1) {
            na_start(&na.wind,&na.wind_rng,1.2f,1.8f,.4f,1.6f,2,3,4,10,peak);
            na.wind_target=350+350*na_random(&na.wind_rng);
        } else {
            na_start(&na.wind,&na.wind_rng,.8f,1.5f,0,1,1.5f,2.5f,2,9,peak*.65f);
            na.wind_target=1400+700*na_random(&na.wind_rng);
        }
        NA_EVENT(0,type,na.wind.total+na.wind.gap);
    }
    if((na.ctrl&8191u)==0)na.eddy_target=2*na_random(&na.wind_rng)-1;
    na.eddy+=.00012f*(na.eddy_target-na.eddy);
    na.wind_cutoff+=.00008f*(na.wind_target-na.wind_cutoff);
    na.pan+=.000015f*(na.eddy*.2f-na.pan);
    if((na.ctrl&127u)==0) {
        float fc=na.wind_cutoff*(1+.12f*na.eddy);
        dsp_svf_set(&na.wind_lp[0],fc,.6f);dsp_svf_set(&na.wind_lp[1],fc*1.06f,.6f);
    }
    float e=na_tick(&na.wind);return e*e*(.9f+.1f*na.eddy);
}
static float na_surf(void) {
    if(na_ready(&na.surf)) {
        na_start(&na.surf,&na.surf_rng,2,2.5f,.2f,1.2f,3.5f,3,3,9,
                 .55f+.4f*na_random(&na.surf_rng));
        NA_EVENT(1,0,na.surf.total+na.surf.gap);
    }
    return na_tick(&na.surf);
}
static float na_rain(void) {
    if(na_ready(&na.rain)) {
        na_start(&na.rain,&na.rain_rng,3,3,5,8,4,4,3,10,
                 .35f+.4f*na_random(&na.rain_rng));
        NA_EVENT(2,0,na.rain.total+na.rain.gap);
    }
    float e=na_tick(&na.rain);
    if((na.ctrl&127u)==0 && na_random(&na.rain_rng)<e*.15f)
        for(unsigned i=0;i<8;++i) {
            na_drop_t *d=&na.drops[i];if(d->age<d->length)continue;
            d->age=0;d->rise=na_seconds(&na.rain_rng,.004f,.004f);
            d->length=d->rise+na_seconds(&na.rain_rng,.018f,.035f);
            d->inv_up=1.0f/(float)d->rise;d->inv_down=1.0f/(float)(d->length-d->rise);
            d->peak=e*(.2f+.5f*na_random(&na.rain_rng));d->pan=.2f+.6f*na_random(&na.rain_rng);
            break;
        }
    return e;
}
static float na_thunder(float rain) {
    if(na_ready(&na.thunder)) {
        if(rain>.2f) {
            na_start(&na.thunder,&na.thunder_rng,1.2f,1.4f,.2f,.8f,3,3,28,45,
                     .3f+.2f*na_random(&na.thunder_rng));
            NA_EVENT(3,0,na.thunder.total+na.thunder.gap);
        } else na.thunder.gap=na_seconds(&na.thunder_rng,2,3);
    }
    return na_tick(&na.thunder);
}
static void na_weather(float *left,float *right) {
    float wind=na_wind(),surf=0,rain=0,thunder=0;
    if(na.world==WORLD_COAST)surf=na_surf();
    if(na.world==WORLD_WOODLAND) {rain=na_rain();thunder=na_thunder(rain);}
    na.foam+=.00004f*(surf-na.foam);
    float patter[2]={0,0};
    if(na.world==WORLD_WOODLAND)for(unsigned i=0;i<8;++i) {
        na_drop_t *d=&na.drops[i];if(d->age>=d->length)continue;
        uint32_t age=d->age++;
        float e=age<d->rise ? na_round((float)age*d->inv_up) :
            1.0f-na_round((float)(age-d->rise)*d->inv_down);
        float x=na_white(&d->rng)*e*d->peak;
        patter[0]+=x*(1-d->pan);patter[1]+=x*d->pan;
    }
    float w[2],water[2]={0,0};
    for(int ch=0;ch<2;++ch) {
        float x=dsp_svf_lp(&na.wind_lp[ch],dsp_pink(&na.wind_noise[ch]));
        na.wind_dc[ch]+=.012f*(x-na.wind_dc[ch]);w[ch]=(x-na.wind_dc[ch])*wind*.045f;
        if(na.world==WORLD_COAST) {
            float noise=dsp_pink(&na.water_noise[ch]);
            float body=dsp_svf_lp(&na.surf_lp[ch],noise);
            na.surf_dc[ch]+=.012f*(body-na.surf_dc[ch]);
            float foam=dsp_svf_lp(&na.foam_lp[ch],noise)-body;
            water[ch]=(body-na.surf_dc[ch])*surf*.052f+foam*na.foam*.020f;
        } else if(na.world==WORLD_WOODLAND) {
            float x=dsp_svf_lp(&na.rain_lp[ch],dsp_pink(&na.water_noise[ch])*rain*.5f+patter[ch]);
            na.rain_dc[ch]+=.04f*(x-na.rain_dc[ch]);water[ch]=(x-na.rain_dc[ch])*.035f;
            if(thunder>0) {
                float rumble=dsp_svf_lp(&na.thunder_lp[ch],dsp_pink(&na.thunder_noise[ch]));
                na.thunder_dc[ch]+=.004f*(rumble-na.thunder_dc[ch]);
                float accent=na_bound((rumble-na.thunder_dc[ch])*thunder*.07f,.0007f);
                water[ch]+=accent;NA_THUNDER(accent);
            }
        }
    }
    /* A shared middle with modest sides survives mono. Slow wind movement
     * changes width/pan without delay, pitch, rhythmic pulsation or whistles. */
    /* The quieter decaying WOODLAND Cells need an additional 9.1 dB reserve. */
    float balance=na.world==WORLD_WOODLAND ? .35f : 1;
    *left=balance*na_bound((w[0]*.8f+w[1]*.2f)*(1-na.pan)+water[0]*.8f+water[1]*.2f,.004f);
    *right=balance*na_bound((w[1]*.8f+w[0]*.2f)*(1+na.pan)+water[1]*.8f+water[0]*.2f,.004f);
    ++na.ctrl;
}
void nature_set_amount(float value) {
    if(!isfinite(value))return;
    value=dsp_clampf(value,0,1);uint32_t bits;memcpy(&bits,&value,4);
    atomic_store_explicit(&na_amount_bits,bits,memory_order_release);
}
void nature_set_world(int world) {
    if(world>=0 && world<CORE_WORLD_COUNT)
        atomic_store_explicit(&na_requested_world,world,memory_order_release);
}
void nature_set_seed(uint32_t seed) {
    atomic_store_explicit(&na_requested_seed,seed ? seed : 0xA6B13E7Du,memory_order_release);
}
void nature_clear(void) {
    int world=na.world;uint32_t seed=na.seed;na_reset(world,seed);na.idle=true;
    memset(&na_music,0,sizeof na_music);na_music.gain=na_music.goal=1;
}
void nature_init(void) {
    nature_set_amount(0);nature_set_world(WORLD_COAST);nature_set_seed(0xA6B13E7Du);
    na_reset(WORLD_COAST,0xA6B13E7Du);na.idle=true;
    memset(&na_music,0,sizeof na_music);na_music.gain=na_music.goal=1;
}
static void na_render(float *left,float *right,const float *music_l,const float *music_r,
                       float reference_gain,int frames) {
    uint32_t bits=atomic_load_explicit(&na_amount_bits,memory_order_acquire);
    float target;memcpy(&target,&bits,4);
    int world=atomic_load_explicit(&na_requested_world,memory_order_acquire);
    uint32_t seed=atomic_load_explicit(&na_requested_seed,memory_order_acquire);
    if(na.idle && target==0) {
        if(world!=na.world || seed!=na.seed) {na_reset(world,seed);na.idle=true;}
        return;
    }
    if(na.idle)na_reset(world,seed);
    for(int i=0;i<frames;++i) {
        if(target==0 && na.amount<1e-6f) {na.amount=na.transition=0;na.idle=true;continue;}
        float t=(world==na.world && seed==na.seed) ? 1 : 0;
        float step=1.0f/(NA_SR*(t>0 ? 2 : .08f));
        if(na.transition<t)na.transition=fminf(t,na.transition+step);
        else if(na.transition>t)na.transition=fmaxf(t,na.transition-step);
        if(na.transition==0 && (world!=na.world || seed!=na.seed)) {
            /* A change resets all envelopes behind zero, preserving the
             * separate amount smoother so rapid World changes stay quiet. */
            float amount=na.amount;na_reset(world,seed);na.amount=amount;
        }
        na.amount+=(target-na.amount)*(1.0f/(NA_SR*2));
        float l,r;na_weather(&l,&r);
        l=l*na.amount*na.transition;r=r*na.amount*na.transition;
        float gain=music_l ? na_background_gain(music_l[i]*reference_gain,music_r[i]*reference_gain,left[i],right[i],l,r) : 1;
        left[i]+=l*gain;right[i]+=r*gain;
    }
}
void nature_render(float *left,float *right,int frames) {na_render(left,right,NULL,NULL,1,frames);}
void nature_render_music(float *left,float *right,const float *music_l,const float *music_r,
                         float reference_gain,int frames) {
    na_render(left,right,music_l,music_r,reference_gain,frames);
}
#undef NA_EVENT
#undef NA_THUNDER
#undef NA_SR
#endif
