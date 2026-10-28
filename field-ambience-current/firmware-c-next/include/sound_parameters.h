#ifndef FAM_SOUND_PARAMETERS_H
#define FAM_SOUND_PARAMETERS_H
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
/* Main-owned musical controls, independent of physical assignments. */
typedef struct {
    float delay,grain,drive,crush,phaser;
    bool freeze,plate;
    float delay_left_ms,delay_right_ms,feedback,grain_ms,phaser_hz;
} sound_parameters_t;
/* Control ranges are independent of any future encoder/menu assignment.
 * Feedback <= .5 keeps the longest echo below the existing 14s tail bound. */
static inline sound_parameters_t sound_parameters_default(void) {
    return (sound_parameters_t){.delay=.16f,.grain=.12f,.drive=.04f,.plate=true,
        .delay_left_ms=210,.delay_right_ms=267,.feedback=.32f,
        .grain_ms=8192000.0f/44100.0f,.phaser_hz=.07f};
}
static inline bool sound_parameters_normalize(const sound_parameters_t *in,sound_parameters_t *out) {
    if(!in || !out)return false;
    float v[10]={in->delay,in->grain,in->drive,in->crush,in->phaser,
        in->delay_left_ms,in->delay_right_ms,in->feedback,in->grain_ms,in->phaser_hz};
    static const float lo[10]={0,0,0,0,0,40,40,0,40,.01f};
    static const float hi[10]={1,1,1,1,1,700,700,.5f,260,.5f};
    for(int i=0;i<10;++i)if(!isfinite(v[i]))return false;
    for(int i=0;i<10;++i)v[i]=v[i]<lo[i] ? lo[i] : v[i]>hi[i] ? hi[i] : v[i];
    /* A zero Q16 Grain return cannot hold a hidden capture. Turning Grain
     * down disarms Freeze; audio resumes writing before its pitch guard ends. */
    bool freeze=in->freeze && v[1]>=1.0f/65536.0f;
    *out=(sound_parameters_t){v[0],v[1],v[2],v[3],v[4],freeze,in->plate,
        v[5],v[6],v[7],v[8],v[9]};
    return true;
}
/* FXP1: explicit float32 bits in little endian, flags and CRC32. This is
 * target state only, never effect histories or captured Freeze audio. */
#define SOUND_PARAMETERS_BYTES 52u
static inline uint32_t sound_wire_get32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}
static inline void sound_wire_put32(uint8_t *p,uint32_t n) {
    for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(n>>(8*i));
}
static inline uint32_t sound_wire_crc(const uint8_t *p,unsigned n) {
    uint32_t c=0xffffffffu;
    for(unsigned i=0;i<n;++i) {c^=p[i];for(unsigned b=0;b<8;++b)c=(c>>1)^(0xedb88320u&(0u-(c&1u)));}
    return ~c;
}
static inline bool sound_parameters_encode(const sound_parameters_t *in,uint8_t *bytes) {
    _Static_assert(sizeof(float)==4,"FXP1 needs IEEE float32 targets");
    sound_parameters_t p;if(!bytes || !sound_parameters_normalize(in,&p))return false;
    float v[10]={p.delay,p.grain,p.drive,p.crush,p.phaser,p.delay_left_ms,p.delay_right_ms,p.feedback,p.grain_ms,p.phaser_hz};
    memset(bytes,0,SOUND_PARAMETERS_BYTES);memcpy(bytes,"FXP1",4);
    for(unsigned i=0;i<10;++i) {uint32_t n;memcpy(&n,&v[i],4);sound_wire_put32(bytes+4+4*i,n);}
    bytes[44]=(uint8_t)((p.freeze ? 1 : 0)|(p.plate ? 2 : 0));
    sound_wire_put32(bytes+48,sound_wire_crc(bytes,48));return true;
}
static inline bool sound_parameters_decode(const uint8_t *bytes,sound_parameters_t *out) {
    if(!bytes || !out || memcmp(bytes,"FXP1",4) || bytes[44]>3 || bytes[45] || bytes[46] || bytes[47] ||
       sound_wire_get32(bytes+48)!=sound_wire_crc(bytes,48))return false;
    float v[10];for(unsigned i=0;i<10;++i) {uint32_t n=sound_wire_get32(bytes+4+4*i);memcpy(&v[i],&n,4);}
    sound_parameters_t p={v[0],v[1],v[2],v[3],v[4],(bytes[44]&1)!=0,(bytes[44]&2)!=0,v[5],v[6],v[7],v[8],v[9]},q;
    if(!sound_parameters_normalize(&p,&q))return false;
    float normalized[10]={q.delay,q.grain,q.drive,q.crush,q.phaser,q.delay_left_ms,q.delay_right_ms,q.feedback,q.grain_ms,q.phaser_hz};
    for(unsigned i=0;i<10;++i)if(v[i]!=normalized[i])return false;
    *out=q;return true;
}
bool engine_apply_sound_parameters(const sound_parameters_t *parameters);
void engine_get_sound_parameters(sound_parameters_t *parameters);
#endif
