#!/usr/bin/env python3
"""Only long WOODLAND output Color changes; exact PR157/actual ensemble comparison."""
import argparse
import ctypes as C
import hashlib
import io
import json
from pathlib import Path
import shutil
import subprocess
import tarfile
import tempfile
import numpy as np
from check_pluck_tuning import pitch
from package_musical_review import measure
from review_activity_comparison import complete_pcm, pcm_metrics
from review_color_comparison import spectrum, trace
from review_room_comparison import ROOT, SR, SOURCES, sha, write

BASE='ab673f49de056154d985290d5bc1f8a7f5bf43a5'
TREE='f76ba92fa14a90846b3804b21704cf26f2b98888'
WORLDS=('COAST','WOODLAND','HIGHLANDS')
VALUES=(0.,.5,1.)
SECONDS=112

def solo(lib,freq,color):
    e=C.CDLL(str(lib));e.pluck_set_ambient.argtypes=[C.c_bool]
    e.pluck_set_damp.argtypes=[C.c_float];e.pluck_note_on.argtypes=[C.c_uint8,C.c_float,C.c_float]
    e.pluck_note_on.restype=C.c_bool;p=C.POINTER(C.c_float);e.pluck_render_mix.argtypes=[p]*4+[C.c_int]
    e.dsp_init();e.shape_set_attack(C.c_float(.5));e.shape_set_release(C.c_float(.5))
    e.pluck_init();e.pluck_set_ambient(True)
    e.pluck_set_damp(float(np.float32(np.float32(.025)-np.float32(.020)*np.float32(.5))))
    if hasattr(e,'pluck_set_color'):
        e.pluck_set_color.argtypes=[C.c_float];e.pluck_set_color(color)
    assert e.pluck_note_on(6,freq,.4)
    x=np.zeros((4,SR*4),np.float32)
    for at in range(0,SR*4,100):
        e.pluck_render_mix(*[r[at:].ctypes.data_as(p) for r in x],min(100,SR*4-at))
    assert np.isfinite(x).all() and np.max(np.abs(x))<1
    return x[:2].T.copy()

def decay(x,freq):
    amplitudes=[]
    for start in (.90,1.40):
        y=x[round(start*SR):round((start+.25)*SR),0].astype(float)
        phase=np.arange(len(y))*2*np.pi*freq/SR
        amplitudes.append(float(abs(np.dot(y*np.hanning(len(y)),np.exp(-1j*phase)))))
    assert 0<amplitudes[1]<amplitudes[0]
    return float(-3/np.log10(amplitudes[1]/amplitudes[0]))

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('output',type=Path)
    out=parser.parse_args().output.resolve();out.mkdir(parents=True,exist_ok=True)
    repo=Path(subprocess.check_output(['git','rev-parse','--show-toplevel'],cwd=ROOT,text=True).strip())
    prefix=ROOT.relative_to(repo)
    assert subprocess.check_output(['git','rev-parse',BASE+'^{tree}'],cwd=ROOT,text=True).strip()==TREE
    tracked=subprocess.check_output(['git','ls-tree','-r','--name-only',BASE,'--',str(prefix/'src'),
        str(prefix/'include'),str(prefix/'cmake'),str(prefix/'CMakeLists.txt')],cwd=repo,text=True).splitlines()
    compiled=[p for p in tracked if Path(p).suffix.lower() in ('.c','.h','.s','.ld','.cmake') or Path(p).name=='CMakeLists.txt']
    changed=set();core_hashes={}
    for rel in compiled:
        original=subprocess.check_output(['git','show',f'{BASE}:{rel}'],cwd=ROOT);current=(repo/rel).read_bytes()
        if current!=original:changed.add(str(Path(rel).relative_to(prefix)))
        core_hashes[rel]=hashlib.sha256(current).hexdigest()
    assert changed=={'src/pluck.c','src/engine_product.c','include/pluck.h'},changed
    inputs=[ROOT/'src'/f'{n}.c' for n in SOURCES]+list(sorted((ROOT/'include').glob('*.h')))+[
        Path(__file__),ROOT/'tools/render_color_comparison.c',ROOT/'tools/review_color_comparison.py',
        ROOT/'tools/review_activity_comparison.py',ROOT/'tools/review_room_comparison.py',
        ROOT/'tools/package_musical_review.py',ROOT/'tools/check_pluck_tuning.py',ROOT/'test/test_pluck_color.c']
    manifest=dict(scope='Spec0.10 candidate: only long WOODLAND output Color; exact neutral/other Worlds.',
        baseline_commit=BASE,baseline_tree=TREE,hearing_accepted=False,musical_usefulness_accepted=False,
        changed_core_inputs=sorted(changed),tracked_compilation_input_count=len(compiled),
        unchanged_core_input_count=len(compiled)-len(changed),source_sha256={str(p.relative_to(ROOT)):sha(p) for p in inputs},
        parameters=dict(colors=list(VALUES),seeds=[1234,42],activity=.5,room=.5,nature=0,attack=.5,release=.5,
            volume=.6,key='D',collection='major',tuning='equal'),raw_seconds=SECONDS,generate_stop_seconds=80,
        worlds=[],live=[],solo=[],listening=[])
    flags=['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-DFAM_SOUND_PRODUCT']
    with tempfile.TemporaryDirectory(prefix='woodland-color-review-') as value:
        tmp=Path(value);captures=tmp/'captures';captures.mkdir()
        archive=subprocess.check_output(['git','archive',BASE,str(prefix)],cwd=repo)
        with tarfile.open(fileobj=io.BytesIO(archive)) as tar:tar.extractall(tmp/'baseline',filter='data')
        baseline=tmp/'baseline'/prefix;binaries={};libs={}
        for label,source in [('before',baseline),('after',ROOT)]:
            binary=tmp/label
            subprocess.run([*flags,'-I'+str(source/'include'),str(ROOT/'tools/render_color_comparison.c'),
                *[str(source/'src'/f'{n}.c') for n in SOURCES],'-lm','-o',str(binary)],check=True)
            binaries[label]=binary;lib=tmp/f'{label}.so'
            subprocess.run([*flags,'-shared','-fPIC','-I'+str(source/'include'),
                *[str(source/'src'/f'{n}.c') for n in ('pluck','dsp','shape')],'-lm','-o',str(lib)],check=True)
            libs[label]=lib
        check=tmp/'float-contract'
        subprocess.run([*flags,'-I'+str(ROOT/'include'),str(ROOT/'test/test_pluck_color.c'),
            *[str(ROOT/'src'/f'{n}.c') for n in ('pluck','dsp','shape')],'-lm','-o',str(check)],check=True)
        manifest['float_contract']=subprocess.run([str(check)],capture_output=True,text=True,check=True).stdout.strip()
        print(manifest['float_contract'],flush=True)
        for frequency in (60,110,146.832,220,440,880,1320,1760):
            original=solo(libs['before'],frequency,.5)
            for color in VALUES:
                x=solo(libs['after'],frequency,color)
                cents=float(pitch(x,frequency,round(.9*SR),round(1.4*SR)));assert abs(cents)<.3,(frequency,color,cents)
                if color==.5:assert np.array_equal(x,original),'Neutral float source changed'
                old_decay,new_decay=decay(original,frequency),decay(x,frequency);ratio=new_decay/old_decay
                assert abs(ratio-1)<.002,(frequency,color,ratio)
                manifest['solo'].append(dict(hz=frequency,color=color,measured_fundamental_cents=cents,
                    fundamental_decay_two_window_estimate_seconds=new_decay,decay_ratio_to_baseline_default=ratio,
                    neutral_float_exact=color==.5))
        print('24 actual solo pitch/decay cases passed; eight neutral float captures exact',flush=True)
        files={};sealed=[]
        def render(label,world,seed,color,live):
            suffix='live' if live else f'color{round(color*100):03d}'
            path=captures/f'{label}_{WORLDS[world]}_seed{seed}_{suffix}_raw_112s.wav'
            cmd=[str(binaries[label]),str(world),str(seed),str(color),str(int(live))]
            report=subprocess.run([*cmd,'512',str(path)],check=True,capture_output=True,text=True).stderr.strip()
            if label=='after':
                control=tmp/'control.wav';subprocess.run([*cmd,'64',str(control)],check=True,capture_output=True)
                for ext in ('','.events.csv','.state.csv'):
                    assert sha(Path(str(path)+ext))==sha(Path(str(control)+ext)),'Full64/512 PCM or traces differ'
            pcm=complete_pcm(path,SECONDS);states=trace(Path(str(path)+'.state.csv'));events=trace(Path(str(path)+'.events.csv'))
            assert len(states)==SECONDS*100 and not np.any(pcm[-SR:])
            assert max(int(r['active_sources']) for r in states)<=(2 if world==1 else 3)
            assert int(states[-1]['completed_phrases'])>=1
            metric=pcm_metrics(pcm);level=measure(path)
            assert level['true_peak_dbfs']<=-6 and metric['mono_energy_ratio']>.65 and max(map(abs,metric['dc']))<.0002
            row=dict(seed=seed,color=color,live=live,renderer_report=report,raw_level=level,**metric,
                full_sha256={ext:sha(Path(str(path)+ext)) for ext in ('','.events.csv','.state.csv')},
                starts=sum(r['on']=='1' for r in events),maximum_actual_sources=max(int(r['active_sources']) for r in states),
                full_owner_retirement_after_stop_seconds=next(int(r['frame'])/SR for r in states[8000:] if r['active_sources']=='0'),
                held_spectrum=spectrum(np.asarray(pcm[6*SR:round(12.5*SR)],dtype=float)/32768),
                candidate_pcm_events_state_64_512_identical=label=='after',final_second_exact_silence=True)
            if live and label=='after':
                row['live_change_maximum_adjacent_steps']=[]
                for second in (10,20,32,48,64,72):
                    x=np.asarray(pcm[second*SR-SR//50:second*SR+SR//50],dtype=float)/32768
                    step=float(np.max(np.abs(np.diff(x,axis=0))));assert step<.05
                    row['live_change_maximum_adjacent_steps'].append(step)
            sealed.append((path,row['full_sha256']));files[label,world,seed,color,live]=path;return row
        for world,name in enumerate(WORLDS):
            cases=[]
            for seed in (1234,42):
                musical=None
                for color in VALUES:
                    before=render('before',world,seed,color,False);after=render('after',world,seed,color,False)
                    for ext in ('.events.csv','.state.csv'):
                        assert before['full_sha256'][ext]==after['full_sha256'][ext],'Baseline music/ownership changed'
                    hashes=tuple(after['full_sha256'][e] for e in ('.events.csv','.state.csv'))
                    if musical is None:musical=hashes
                    assert musical==hashes,'Color changed holds, notes, retirement or admission'
                    exact=before['full_sha256']['']==after['full_sha256']['']
                    assert exact==(world!=1 or color==.5),'Unexpected sound scope/default change'
                    after.update(baseline=before,full_music_state_identical_to_baseline=True,full_pcm_identical_to_baseline=exact)
                    cases.append(after);print(name,seed,color,'full music/state retained;64/512 exact; scope/default correct',flush=True)
                if world==1:
                    centers=[c['held_spectrum']['power_weighted_centroid_hz'] for c in cases[-3:]]
                    assert centers[0]<centers[1]<centers[2] and (centers[2]-centers[0])/centers[1]>.05
            before=render('before',world,1234,.5,True);after=render('after',world,1234,.5,True)
            for ext in ('.events.csv','.state.csv'):
                assert before['full_sha256'][ext]==after['full_sha256'][ext]==cases[1]['full_sha256'][ext]
            if world!=1:assert before['full_sha256']['']==after['full_sha256']['']
            after.update(world=name,baseline=before,full_live_music_state_identical=True)
            manifest['live'].append(after);manifest['worlds'].append(dict(world=name,cases=cases))
        def package(name,sections,length):
            segments=[];levels=[]
            for label,color in sections:
                x=np.asarray(complete_pcm(files[label,1,1234,color,False],SECONDS)[3*SR:round((3+length)*SR)],dtype=float)/32768
                edge=round(.04*SR);x[:edge]*=np.linspace(0,1,edge)[:,None];x[-edge:]*=np.linspace(1,0,edge)[:,None]
                p=tmp/'segment.wav';write(p,x);levels.append(measure(p));segments.append(x)
            gain=min(18.,-23-max(r['integrated_lufs'] for r in levels),-6-max(r['true_peak_dbfs'] for r in levels))
            parts=[];rows=[]
            for i,((label,color),x) in enumerate(zip(sections,segments)):
                if i:parts.append(np.zeros((SR//2,2)))
                x*=10**(gain/20);parts.append(x);p=tmp/'segment.wav';write(p,x)
                rows.append(dict(checkpoint=label,color=color,source_seconds=[3,3+length],
                    output_seconds=[i*(length+.5),i*(length+.5)+length],delivered_level=measure(p)))
            p=captures/name;write(p,np.concatenate(parts));complete_pcm(p,len(sections)*length+(len(sections)-1)*.5)
            manifest['listening'].append(dict(file=name,sha256=sha(p),level=measure(p),fixed_gain_db_all_sections=gain,
                sections=rows,cut_edge_fade_ms=40,comparison_gap_seconds=.5))
        package('AMBIENT_WOODLAND_Color_calibrated_0_50_100_38p5s.wav',[('after',c) for c in VALUES],12.5)
        package('AMBIENT_WOODLAND_Color_Before_After_27p5s.wav',[('before',0),('after',0),('before',1),('after',1)],6.5)
        for path,hashes in sealed:
            complete_pcm(path,SECONDS)
            for ext,expected in hashes.items():assert sha(Path(str(path)+ext))==expected
        for rel,expected in core_hashes.items():assert sha(repo/rel)==expected
        for rel,expected in manifest['source_sha256'].items():assert sha(ROOT/rel)==expected
        for p in sorted(captures.iterdir()):
            if p.name.endswith(('.events.csv','.state.csv')) or p.name.startswith('AMBIENT_'):
                target=out/p.name;shutil.copyfile(p,target);assert sha(target)==sha(p)
        for row in manifest['listening']:assert sha(out/row['file'])==row['sha256']
    manifest.update(candidate_full_renders=42,baseline_full_renders=21,total_full_render_minutes=63*SECONDS/60)
    (out/'SD33_WOODLAND_COLOR_METRICS.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print('WOODLAND COLOR PASS:63 full renders/117.6min,24 solo pitch/decay cases; neutral/other Worlds exact, '
        'full musical/retirement traces preserved, two short WAVs. Hearing remains open.',flush=True)

if __name__=='__main__':main()
