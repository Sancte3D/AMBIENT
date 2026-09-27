#!/usr/bin/env python3
"""Real C firmware renders and sample-exact FLAC verification (numpy + ffmpeg).
One process per clip keeps all file-static oscillator and random state reproducible.
"""
import argparse,ctypes as C,hashlib,json,math,subprocess,tempfile,wave
from pathlib import Path
import numpy as np
from render_musical_review import Instrument,SR,BLOCK,worlds

def verify_flac(path,frames,expected=None):
    h=hashlib.sha256();size=0
    with tempfile.TemporaryFile() as errors:
        p=subprocess.Popen(['ffmpeg','-v','error','-i',str(path),'-f','s16le','-acodec','pcm_s16le','-'],stdout=subprocess.PIPE,stderr=errors)
        while True:
            b=p.stdout.read(262144)
            if not b:break
            h.update(b);size+=len(b)
        code=p.wait();errors.seek(0);err=errors.read()
    if code or err or size!=frames*4 or (expected and h.hexdigest()!=expected):
        raise RuntimeError(f'Invalid FLAC {path}: frames {size//4}/{frames}; {err!r}')
    return h.hexdigest()

def encode(wav,out,frames,digest):
    tmp=out.with_suffix('.partial.flac')
    p=subprocess.run(['ffmpeg','-v','error','-y','-i',str(wav),'-c:a','flac','-compression_level','5','-sample_fmt','s16',str(tmp)],capture_output=True,check=True)
    if p.stderr:raise RuntimeError(p.stderr.decode())
    verify_flac(tmp,frames,digest);tmp.replace(out)

def render(libpath,out,kind,index,seconds):
    inst=Instrument(libpath);lib=inst.lib
    for control,value in [('master_volume',.5),('drive',.1),('attack',.5),('release',.5)]:inst.call('set_'+control,value)
    inst.call('set_tuning',1);lib.tuning_hz.argtypes=[C.c_float];lib.tuning_hz.restype=C.c_float
    frame=0;notes=[];states=[];events=[];collisions=0
    if hasattr(lib,'engine_sounding_notes'):lib.engine_sounding_notes.argtypes=[C.POINTER(C.c_int),C.c_int]
    def hook(on,source,hz,amp):
        nonlocal collisions
        midi=round(69+12*math.log2(hz/440)) if hz>0 else None
        row={'seconds':round(frame/SR,4),'on':on,'source':source,'midi':midi,'hz':hz,'amp':amp}
        if on==1 and source in [5,6,7,8,15] and hasattr(lib,'engine_sounding_notes'):
            a=(C.c_int*128)();n=lib.engine_sounding_notes(a,128);row['sounding_before']=list(a[:n])
            for m in a[:n]:
                d=abs(midi-m);ic=min(d%12,12-d%12)
                if ic in [1,6] or (ic==2 and d<12 and min(m,midi)<60):collisions+=1
        notes.append(row)
    hook_type=C.CFUNCTYPE(None,C.c_int,C.c_uint8,C.c_float,C.c_float);callback=hook_type(hook)
    lib.engine_set_note_hook.argtypes=[hook_type];lib.engine_set_note_hook(callback)
    def event(t,name,*args):events.append((round(t*SR/BLOCK)*BLOCK,name,args))
    def note(t,source,midi,amp,length):event(t,'note',source,midi,amp);event(t+length,'note_off',source)
    if kind=='autoplay':
        w=worlds()[index];inst.call('set_world',index);inst.call('set_voice',w['voice']);inst.call('set_brightness',w['brightness_hz'])
        for m,f in [('space','space_pct'),('atmosphere','atmos_pct'),('motion','motion_pct'),('age','age_pct'),('echo','echo_pct'),('blur','blur_pct'),('shimmer','shimmer_pct')]:inst.call('set_'+m,w[f]/100)
        inst.call('generative_new_field',0xA6B13);inst.call('set_generative',True,-1)
    elif kind=='steal':
        inst.call('set_world',2 if index==3 else 0);inst.call('set_voice',index)
        inst.call('set_atmosphere',.4);inst.call('set_space',.6);inst.call('set_echo',.35)
        for t in [1,13,25]:
            for i,m in enumerate([57,64,69,72,76]):note(t+i*.35,i,m,.14+i*.025,5.5-i*.3)
    elif kind=='macro':
        inst.call('set_synth',index);inst.call('set_atmosphere',.38);inst.call('set_space',.55);inst.call('set_echo',.3);inst.call('set_motion',.2)
        for t in [1,10,19,28]:note(t,0,57,.55,4.2);note(t+4.8,1,64,.45,1)
        for t,m,v in [(9,'brightness',650),(9,'resonance',.6),(18,'brightness',0),(18,'resonance',0),(18,'sweep',.8),(18,'envmod',.7),(27,'sweep',0),(27,'envmod',0),(27,'attack',.9),(27,'release',.85)]:event(t,'set_'+m,v)
    else:
        inst.call('set_atmosphere',.55);inst.call('set_space',.6);inst.call('set_echo',.4)
        for i,core in enumerate([0,3,5,2,4,6,1,0]):
            t=1+i*6;event(t,'set_synth',core);note(t+.1,0,57+(i%3)*7,.32,4.5)
    events.sort(key=lambda x:x[0]);cursor=0;prev=-1;total=seconds*SR
    block=np.zeros((BLOCK,2),dtype=np.int16);h=hashlib.sha256();peak=clips=0;e=dc=0.
    out=Path(out);out.parent.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='pcm-',dir=out.parent) as tmp:
        wav=Path(tmp)/'raw.wav'
        with wave.open(str(wav),'wb') as f:
            f.setnchannels(2);f.setsampwidth(2);f.setframerate(SR)
            for frame in range(0,total,BLOCK):
                while cursor<len(events) and events[cursor][0]<=frame:
                    _,name,args=events[cursor]
                    if name=='note':source,midi,amp=args;inst.call('note_on',source,lib.tuning_hz(midi),amp)
                    else:inst.call(name,*args)
                    cursor+=1
                if kind=='autoplay':
                    inst.call('generative_tick',frame*1000//SR);state=lib.composer_state()
                    if state!=prev:states.append({'seconds':round(frame/SR,3),'state':state});prev=state
                n=min(BLOCK,total-frame);lib.engine_render(block.ctypes.data_as(C.POINTER(C.c_int16)),n)
                data=block[:n].astype('<i2',copy=False).tobytes();f.writeframesraw(data);h.update(data)
                x=block[:n].astype(np.float64);peak=max(peak,int(np.max(np.abs(x))))
                clips+=int(np.count_nonzero(np.abs(x)>=32767));e+=float(np.sum(x*x));dc+=float(np.sum(x))
        encode(wav,out,total,h.hexdigest())
    melody=[x for x in notes if x['on']==1 and x['source']==15]
    row={'file':out.name,'kind':kind,'index':index,'seconds':seconds,'frames':total,'sample_rate':SR,'bits':16,
         'peak_dbfs':20*math.log10(max(peak,1)/32768),'rms_dbfs':10*math.log10(max(e/(2*total),1e-12)/(32768**2)),
         'dc':dc/(total*2*32768),'clipped_samples':clips,'collision_failures':collisions,'pcm_sha256':h.hexdigest(),
         'melody_notes':len(melody),'melody_unique':len({x['midi'] for x in melody}),'events':notes,'states':states,'actions':events}
    out.with_suffix('.json').write_text(json.dumps(row,indent=2)+'\n')
    print(json.dumps({k:v for k,v in row.items() if k not in ['events','states','actions']}),flush=True)
    if clips or collisions:raise RuntimeError('Render quality gate failed')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--engine',required=True);p.add_argument('--output',required=True)
    p.add_argument('--kind',choices=['autoplay','steal','macro','switch'],required=True);p.add_argument('--index',type=int,default=0);p.add_argument('--seconds',type=int,required=True)
    a=p.parse_args();render(a.engine,a.output,a.kind,a.index,a.seconds)
