/* One shared floating-point Householder room, reusing the eight delay lengths
 * and orthogonal matrix of ambient_effects.c. Two input allpasses per side
 * diffuse the onset. Fixed delays: no pitch shift, chorus or deterministic
 * swells. No other effect storage is linked by the reduced product profile. */
#include "ambient_room.h"
#include "dsp.h"
#include <stdatomic.h>
#include <string.h>
enum { LINES=8, SAMPLES=13838, DIFF_SAMPLES=1324 };
static const uint16_t lengths[LINES]={1117,1277,1429,1601,1789,1999,2203,2423};
static const uint16_t diff_len[4]={239,389,277,419};
static float tank[SAMPLES],diffusion[DIFF_SAMPLES];
static uint16_t at[LINES],diff_at[4];
static float low[LINES],gains[LINES],hp_x[2],hp_y[2],amount_cur,enable_cur,peak;
static _Atomic uint32_t gain_bits[LINES],amount_bits,t60_bits;
static _Atomic bool enabled;
static uint32_t bits(float v) { uint32_t b; memcpy(&b,&v,4); return b; }
static float value(uint32_t b) { float v; memcpy(&v,&b,4); return v; }
void ambient_room_set(float amount) {
    if(!isfinite(amount)) return;
    amount=dsp_clampf(amount,0,1);
    float t60=1.2f+3.6f*amount*amount;
    for(int i=0;i<LINES;++i) {
        float g=expf(-6.9077553f*(float)lengths[i]/(DSP_SAMPLE_RATE_HZ*t60));
        atomic_store_explicit(&gain_bits[i],bits(g),memory_order_release);
    }
    atomic_store_explicit(&amount_bits,bits(amount),memory_order_release);
    atomic_store_explicit(&t60_bits,bits(t60),memory_order_release);
}
void ambient_room_enable(bool on) { atomic_store_explicit(&enabled,on,memory_order_release); }
void ambient_room_clear(void) {
    memset(tank,0,sizeof tank); memset(diffusion,0,sizeof diffusion);
    memset(at,0,sizeof at); memset(diff_at,0,sizeof diff_at);
    memset(low,0,sizeof low); memset(hp_x,0,sizeof hp_x); memset(hp_y,0,sizeof hp_y); peak=0;
}
void ambient_room_init(void) {
    ambient_room_clear(); ambient_room_set(.5f); ambient_room_enable(true);
    amount_cur=.5f; enable_cur=1;
    for(int i=0;i<LINES;++i) gains[i]=value(atomic_load(&gain_bits[i]));
}
float ambient_room_tail_seconds(void) { return value(atomic_load_explicit(&t60_bits,memory_order_acquire)); }
float ambient_room_peak(void) { return peak; }
size_t ambient_room_storage_bytes(void) { return sizeof tank+sizeof diffusion+sizeof low+sizeof at+sizeof diff_at+sizeof gains+sizeof hp_x+sizeof hp_y; }
static float allpass(int k,float x) {
    int offset=0; for(int i=0;i<k;++i) offset+=diff_len[i];
    float d=diffusion[offset+diff_at[k]], y=d-.62f*x;
    diffusion[offset+diff_at[k]]=x+.62f*y;
    if(++diff_at[k]==diff_len[k]) diff_at[k]=0;
    return y;
}
void ambient_room_process(float *l,float *r,const float *sl,const float *sr,int frames) {
    float targets[LINES];
    for(int i=0;i<LINES;++i) targets[i]=value(atomic_load_explicit(&gain_bits[i],memory_order_acquire));
    float target=value(atomic_load_explicit(&amount_bits,memory_order_acquire));
    float on=atomic_load_explicit(&enabled,memory_order_acquire) ? 1 : 0;
    static const float il[LINES]={1,-1,.72f,-.72f,.45f,-.45f,.88f,-.88f};
    static const float ir[LINES]={.45f,.88f,-1,-.72f,1,.72f,-.45f,-.88f};
    peak=0;
    for(int n=0;n<frames;++n) {
        amount_cur+=(target-amount_cur)*(1.0f/(.080f*DSP_SAMPLE_RATE_HZ));
        enable_cur+=(on-enable_cur)*(1.0f/(.040f*DSP_SAMPLE_RATE_HZ));
        float in[2]={sl[n]*enable_cur,sr[n]*enable_cur};
        for(int c=0;c<2;++c) {
            float y=in[c]-hp_x[c]+.98585f*hp_y[c];
            hp_x[c]=in[c]; hp_y[c]=y; in[c]=allpass(2*c+1,allpass(2*c,y));
        }
        float line[LINES],sum=0; int offset=0;
        for(int i=0;i<LINES;++i) {
            float x=tank[offset+at[i]];
            low[i]+=(x-low[i])*.25f; line[i]=low[i]; sum+=line[i]; offset+=lengths[i];
        }
        offset=0;
        for(int i=0;i<LINES;++i) {
            gains[i]+=(targets[i]-gains[i])*(1.0f/(.120f*DSP_SAMPLE_RATE_HZ));
            /* Orthogonal feedback, gain <1, convex damping. Floating-point
             * tank avoids int16 truncation creating a premature silent tail. */
            float x=(line[i]-.25f*sum)*gains[i]+.105f*(in[0]*il[i]+in[1]*ir[i]);
            if(fabsf(x)<1e-20f) x=0;
            tank[offset+at[i]]=x;
            if(++at[i]==lengths[i]) at[i]=0;
            offset+=lengths[i];
        }
        float wl=.205f*(line[0]-line[1]+line[2]+line[4]-line[6]+line[7]);
        float wr=.205f*(line[1]+line[3]-line[4]+line[5]+line[6]-line[7]);
        float mid=.5f*(wl+wr),side=.30f*(wl-wr);
        float wet=(.12f+.50f*amount_cur)*amount_cur*enable_cur;
        wl=(mid+side)*wet; wr=(mid-side)*wet;
        peak=fmaxf(peak,fmaxf(fabsf(wl),fabsf(wr)));
        l[n]+=wl; r[n]+=wr;
    }
}
