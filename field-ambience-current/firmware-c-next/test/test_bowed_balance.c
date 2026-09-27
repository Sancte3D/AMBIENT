/* Sustained timbre regression: the fundamental must not repeatedly collapse
 * beneath the octave. Exercise real oscillators, body and sympathetic modes. */
#include "bowed.h"
#include "dsp.h"
#include "shape.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    const int pitches[]={50,62,71,81};
    int failed=0;
    dsp_init(); shape_init();
    for(int color=0;color<2;++color) for(int p=0;p<4;++p) {
        float hz=dsp_midi_to_hz((float)pitches[p]);
        bowed_init(); bowed_set_colour(color); bowed_note_on(15,hz,.42f);
        dsp_svf_t fundamental, octave;
        dsp_svf_reset(&fundamental); dsp_svf_reset(&octave);
        dsp_svf_set(&fundamental,hz,20.0f); dsp_svf_set(&octave,2.0f*hz,20.0f);
        double ef=0,eo=0,low=1e9,high=0,ratio_low=1e9;
        float l[256],r[256],sl[256],sr[256];
        for(int start=0;start<12*44100;start+=256) {
            int count=12*44100-start; if(count>256) count=256;
            memset(l,0,sizeof l); memset(r,0,sizeof r);
            memset(sl,0,sizeof sl); memset(sr,0,sizeof sr);
            bowed_render_mix(l,r,sl,sr,count,.5f);
            for(int n=0;n<count;++n) {
                int sample=start+n;
                float mono=.5f*(l[n]+r[n]);
                float f=dsp_svf_bp(&fundamental,mono), o=dsp_svf_bp(&octave,mono);
                ef+=(double)f*f; eo+=(double)o*o;
                if((sample+1)%4410==0) {
                    if(sample>=2*44100 && sample<11*44100) {
                        if(ef<low) low=ef;
                        if(ef>high) high=ef;
                        double ratio=ef/(eo+1e-20);
                        if(ratio<ratio_low) ratio_low=ratio;
                    }
                    ef=eo=0;
                }
            }
        }
        double swing=10*log10(high/low), floor=10*log10(ratio_low);
        printf("bowed color %d MIDI %d: root swing %.2f dB; root/octave floor %.2f dB\n",
               color,pitches[p],swing,floor);
        if(!isfinite(swing) || swing>6.0 || floor<0.0) ++failed;
    }
    printf("bowed balance: 8 probes, %d failures\n",failed);
    return failed ? 1:0;
}
