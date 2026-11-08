/* The Alps foreground must carry its played root without a motor-like
 * sub-octave or an upper-harmonic blare. Test the rendered source, not knobs. */
#include "horn.h"
#include "shape.h"
#include "dsp.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    const int midi[]={50,55,62,69};
    const float amp[]={.18f,.42f,.58f};
    int failures=0;
    dsp_init();shape_init();
    for(int p=0;p<4;++p)for(int a=0;a<3;++a) {
        float hz=dsp_midi_to_hz((float)midi[p]);
        dsp_svf_t band[8];
        const float partial[]={.5f,1,3,4,5,6,7,8};
        for(int k=0;k<8;++k){dsp_svf_reset(&band[k]);dsp_svf_set(&band[k],hz*partial[k],25);}
        horn_init();horn_note_on(15,hz,amp[a]);
        double energy[8]={0},early=0,body=0;
        float l[256],r[256],sl[256],sr[256];
        for(int start=0;start<4*44100;start+=256) {
            int n=4*44100-start;if(n>256)n=256;
            memset(l,0,sizeof l);memset(r,0,sizeof r);memset(sl,0,sizeof sl);memset(sr,0,sizeof sr);
            horn_render_mix(l,r,sl,sr,n,.5f);
            for(int i=0;i<n;++i) {
                int t=start+i;float x=.5f*(l[i]+r[i]);
                if(t<4410)early+=(double)x*x;
                if(t>=2*44100)body+=(double)x*x;
                for(int k=0;k<8;++k){float y=dsp_svf_bp(&band[k],x);if(t>=2*44100)energy[k]+=(double)y*y;}
            }
        }
        double upper=0;for(int k=2;k<8;++k)upper+=energy[k];
        double sub_db=10*log10(energy[0]/energy[1]);
        double upper_db=10*log10(upper/energy[1]);
        double onset_db=10*log10((early/4410)/(body/(2*44100)));
        double root_gain=sqrt(energy[1]/(2*44100))/amp[a];
        printf("horn MIDI %d amp %.2f: sub/root %.2f dB, upper/root %.2f dB, first100ms/body %.2f dB, root gain %.4f\n",
               midi[p],amp[a],sub_db,upper_db,onset_db,root_gain);
        /* BP stopband leakage limits this estimate: -25 dB is not a claim
         * of spectral absence. Upper group is harmonics 3..8. */
        if(!isfinite(sub_db)||!isfinite(upper_db)||!isfinite(onset_db)||!isfinite(root_gain)||sub_db>-25||upper_db>-10||onset_db>-12||root_gain<.09||root_gain>.14)++failures;
    }
    printf("horn body: 12 probes, %d failures\n",failures);
    return failures ? 1:0;
}
