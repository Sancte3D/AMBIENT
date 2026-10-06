#!/usr/bin/env python3
"""Build actual product core, render <=30s files, fixed-gain hearing copies."""
import array
import hashlib
import json
import math
from pathlib import Path
import re
import shutil
import subprocess
import sys
import wave
import tempfile

root=Path(__file__).resolve().parents[1]
out=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else root/'build-product-audio'
out.mkdir(parents=True,exist_ok=True)
source_names=['engine_product','world_grammar','bowed','horn','pluck','ambient_room','nature','dsp','shape','tuning','brain','cells']
source_paths=[root/'src'/f'{s}.c' for s in source_names]
binary=out/'render_product_preview'
compiler=shutil.which('cc')
ffmpeg=shutil.which('ffmpeg')
assert compiler and ffmpeg,'cc and ffmpeg required'
subprocess.run([compiler,'-std=c11','-O2','-Wall','-Wextra','-Werror','-DFAM_SOUND_PRODUCT',
                '-I'+str(root/'include'),str(root/'tools/render_product_preview.c'),
                *map(str,source_paths),'-lm','-o',str(binary)],check=True)
def probe(path):
    with wave.open(str(path),'rb') as w:
        assert w.getnchannels()==2 and w.getsampwidth()==2 and w.getframerate()==44100
        frames=w.getnframes();assert 0<frames<=44100*30
        samples=array.array('h',w.readframes(frames))
        if sys.byteorder!='little':samples.byteswap()
    energy=sum(x*x for x in samples)
    peak=max(abs(x) for x in samples)
    mean_l=sum(samples[::2])/frames/32768
    mean_r=sum(samples[1::2])/frames/32768
    mono=sum(((samples[i]+samples[i+1])/2)**2 for i in range(0,len(samples),2))
    p=subprocess.run([ffmpeg,'-hide_banner','-nostats','-i',str(path),'-af',
                      'loudnorm=I=-26:TP=-6:LRA=11:print_format=json','-f','null','-'],
                     capture_output=True,text=True,check=True)
    match=re.findall(r'\{\s*"input_i"[\s\S]*?\}',p.stderr)
    assert match,p.stderr
    m=json.loads(match[-1])
    return {'frames':frames,'seconds':frames/44100,'rms_dbfs':10*math.log10(energy/len(samples)/32768**2) if energy else -999,
            'sample_peak_dbfs':20*math.log10(peak/32768) if peak else -999,
            'mean_L':mean_l,'mean_R':mean_r,'mono_energy_ratio':mono/(energy/2) if energy else 1,
            'LUFS':float(m['input_i']),'true_peak_dbfs':float(m['input_tp']),
            'sha256':hashlib.sha256(path.read_bytes()).hexdigest()}
manifest={'commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip(),
          'source_sha256':{str(s.relative_to(root)):hashlib.sha256(s.read_bytes()).hexdigest() for s in source_paths},
          'compiler':subprocess.check_output([compiler,'--version'],text=True).splitlines()[0],
          'ffmpeg':subprocess.check_output([ffmpeg,'-version'],text=True).splitlines()[0],
          'profile':'product','sample_rate':44100,'block':512,'seed':1234,
          'reference':{'volume':.6,'color':.5,'room':.5,'activity':.5,'attack':.5,'release':.5,'tuning':'Equal','key':'D'},
          'onset_trace':'DSP acknowledgement frame; onset lies in the preceding render block, not exact attack sample',
          'matched_method':'one constant file gain, target -26 LUFS, max +12 dB, true-peak ceiling -6 dBFS; no AGC',
          'files':[]}
for mode in ('dry','world','nature','color','shape'):
    directory=out/mode;directory.mkdir(exist_ok=True)
    for world,name in enumerate(('COAST','WOODLAND','HIGHLANDS')):
        raw=directory/f'{name}_{mode}_raw_27s.wav'
        subprocess.run([str(binary),str(world),'1234',mode,str(raw)],check=True)
        data=probe(raw);assert data['true_peak_dbfs']<=-6, data
        gain=min(12.0,-26-data['LUFS'],-6-data['true_peak_dbfs'])
        assert math.isfinite(gain) and data['rms_dbfs']>-120,(name,mode,data)
        listen=directory/f'{name}_{mode}_listen_27s.wav'
        subprocess.run([ffmpeg,'-v','error','-y','-i',str(raw),'-af',f'volume={gain:.8f}dB',
                        '-ar','44100','-ac','2','-c:a','pcm_s16le',str(listen)],check=True)
        matched=probe(listen);assert matched['true_peak_dbfs']<=-5.9,matched
        row={'world':name,'mode':mode,'raw':data,'listen':matched,'constant_gain_db':gain,
             'raw_file':str(raw.relative_to(out)),'listen_file':str(listen.relative_to(out)),
             'dry_midi':[50,62,69] if mode=='dry' else [62]*3 if mode in ('color','shape') else None,
             'segment_values':[0,.5,1] if mode in ('color','shape') else None,
             'nature_amount':.7 if mode=='nature' else 0}
        manifest['files'].append(row)
        print(f"AUDIO {name} {mode}: raw LUFS={data['LUFS']:.2f} TP={data['true_peak_dbfs']:.2f}; "
              f"listen gain={gain:.2f}dB LUFS={matched['LUFS']:.2f} TP={matched['true_peak_dbfs']:.2f}",flush=True)
# Internal retained-limit probes: no long WAVs and no needless audio uploads.
manifest['limit_probes']=[]
with tempfile.TemporaryDirectory(prefix='ambient-limits-') as temp:
    for world,name in enumerate(('COAST','WOODLAND','HIGHLANDS')):
        for case in (*range(8),8,15,16,23,24,31):
            raw=Path(temp)/f'{world}_{case}.wav'
            subprocess.run([str(binary),str(world),str(case),'limits',str(raw)],check=True,capture_output=True)
            data=probe(raw)
            assert data['frames']==44100*8
            assert data['true_peak_dbfs']<=-6 and data['mono_energy_ratio']>.65,data
            assert max(abs(data['mean_L']),abs(data['mean_R']))<.0002,data
            manifest['limit_probes'].append({'world':name,'case':case,
                'register_bank':case//8,
                'controls':{'color':case&1,'attack':(case>>1)&1,'release':(case>>2)&1,
                            'room':1,'volume':1,'nature':case&1,'velocity':1},
                'notes':([50,57,62],[50,54,57],[62,66,69],[57,62,66])[case//8][:2 if world==1 else 3],
                'steps_seconds':[2,4,5],'raw':data})
    worst=max(x['raw']['true_peak_dbfs'] for x in manifest['limit_probes'])
    mono=min(x['raw']['mono_energy_ratio'] for x in manifest['limit_probes'])
    print(f"PRODUCT LIMITS 42: worst true peak={worst:.2f} dBFS; minimum mono energy={mono:.5f}",flush=True)
# Unknown order, same key/collection and no spatial/nature identification.
blind=out/'blind';blind.mkdir(exist_ok=True)
blind_files=[];blind_key={}
for letter,world in zip('ABC',(2,0,1)):
    raw=blind/f'{letter}_raw_27s.wav'
    subprocess.run([str(binary),str(world),'91267','score_dry',str(raw)],check=True,capture_output=True)
    data=probe(raw);gain=min(12.0,-26-data['LUFS'],-6-data['true_peak_dbfs'])
    listen=blind/f'{letter}_listen_27s.wav'
    subprocess.run([ffmpeg,'-v','error','-y','-i',str(raw),'-af',f'volume={gain:.8f}dB',
                    '-ar','44100','-ac','2','-c:a','pcm_s16le',str(listen)],check=True)
    matched=probe(listen);assert matched['true_peak_dbfs']<=-5.9,matched
    blind_files.append({'label':letter,'raw':data,'listen':matched,'constant_gain_db':gain})
    blind_key[letter]=('COAST','WOODLAND','HIGHLANDS')[world]
(blind/'BLIND_METRICS.json').write_text(json.dumps({'commit':manifest['commit'],
    'source_sha256':manifest['source_sha256'],'seed':91267,'room_enabled':False,'nature':0,
    'reference':{**manifest['reference'],'fx':'Dry'},'files':blind_files},indent=2,allow_nan=False)+'\n')
(out/'BLIND_KEY.json').write_text(json.dumps(blind_key,indent=2)+'\n')
(blind/'README.md').write_text("Three unknown Worlds. Listen to A/B/C listen files, then describe articulation, related notes and rests. All files 27 s, same D major/Equal, Room/Nature off, one documented constant listening gain. Raw files preserve firmware level. This pack has no answer key; BLIND_KEY.json is saved separately. No long-term or calming acceptance is implied.\n")
manifest['blind_files']=blind_files
manifest['blind_key']=blind_key
(out/'PRODUCT_AUDIO_METRICS.json').write_text(json.dumps(manifest,indent=2,allow_nan=False)+'\n')
readme='''# Product sound candidates — short hearing pack

Every WAV is 27 seconds, stereo PCM16 / 44.1 kHz, actual product engine.
Raw is the fixed firmware calibration; listen adds one documented constant
file gain. No AGC or compressor is used to make a source appear better.
Listening copies are comparisons, not a device-volume specification.

dry: D3, D4, A4, one source per nine-second section, same velocity .75,
release after 3 s, Clear at 8.75 s. No Room or Nature.
world: actual autonomous first 27 s, seed 1234, common Key/Room/Color/Activity,
Nature off. A short excerpt cannot approve long-term development.
nature: optional layer alone, amount .7, no phantom tonal source, Room off.
color: one D4 per nine-second segment, Color 0/.5/1, Attack/Release .5.
shape: one D4 per segment, Attack AND Release 0/.5/1, Color .5.
These two endpoint packs use Dry, velocity .75; targets settle before note-on.
limits: 42 eight-second internal PCM probes, only their metrics are retained.
blind: autonomous 27 s, seed 91267, unknown order, Room/Nature off.

Questions: dry — alarm/tube/buzz or loss of body at any register?
world — distinct articulation/time, plausible rests and related tones?
nature — useful place or masking, ticking, homogeneous band or mechanical swell?
Report filename and time of any objection. Hearing/device acceptance is open.
Source hashes, exact commit, levels and constant gains are in the metrics JSON.
'''
(out/'README.md').write_text(readme)
for mode in ('dry','world','nature','color','shape'):
    shutil.copy2(out/'README.md',out/mode/'README.md')
    shutil.copy2(out/'PRODUCT_AUDIO_METRICS.json',out/mode/'PRODUCT_AUDIO_METRICS.json')
