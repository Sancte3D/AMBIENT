#!/usr/bin/env python3
"""Controlled native-firmware sonic A/B; no external sounds or DSP dependencies.
Host-only requirements: numpy, ffmpeg. One subprocess per clip resets all state.
Usage: python3 tools/render_sonic_review.py --before old.so --after new.so --output DIR
"""
import argparse,ctypes as C,json,subprocess,sys,wave,zipfile
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import numpy as np
from render_musical_review import Instrument,render,SR,BLOCK
from package_musical_review import read,write,excerpt,measure


def noise(lib,output,kind):
    inst=Instrument(lib); module='ambience' if kind=='Wind' else 'texture'
    getattr(inst.lib,module+'_set_'+('level' if kind=='Wind' else 'amount')).argtypes=[C.c_float]
    getattr(inst.lib,module+'_set_'+('level' if kind=='Wind' else 'amount'))(0.8)
    f=getattr(inst.lib,module+'_render_mix')
    ptr=C.POINTER(C.c_float);f.argtypes=[ptr,ptr,ptr,ptr,C.c_int,C.c_float]
    x=np.empty((120*SR,2),np.float32)
    for n in range(0,len(x),BLOCK):
        count=min(BLOCK,len(x)-n);b=np.zeros((4,count),np.float32)
        f(*(v.ctypes.data_as(ptr) for v in b),count,0.0)
        x[n:n+count]=b[:2].T
    write(output,x)


def main():
    p=argparse.ArgumentParser();p.add_argument('--before',type=Path);p.add_argument('--after',type=Path)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--one',nargs=4)
    a=p.parse_args()
    if a.one:
        lib,kind,idx,wet=a.one
        if kind in ('Wind','Noise'):noise(lib,a.output,kind)
        else:render(lib,a.output,kind,int(idx),wet=='1')
        return
    a.output.mkdir(parents=True,exist_ok=True);raw=a.output/'raw';raw.mkdir(exist_ok=True)
    jobs=[]
    names=['Acid','FM','Mist','Storm','Orbit','Bamboo']
    for version,lib in [('Vorher',a.before),('Nachher',a.after)]:
        jobs.extend((f'{kind}_{version}',lib,kind,0,0) for kind in ('Wind','Noise'))
        for i,name in enumerate(names):
            for wet in (0,1):jobs.append((f'{name}_{"Raum" if wet else "Trocken"}_{version}',lib,'synth',i,wet))
        for i,name in enumerate(['Alps','Open_Sea','Fjords','Moss','Desert']):
            jobs.append((f'{name}_{version}',lib,'world',i,1))
    def run(j):
        name,lib,kind,i,wet=j
        subprocess.run([sys.executable,__file__,'--output',str(raw/(name+'.wav')),
                        '--one',str(lib.resolve()),kind,str(i),str(wet)],check=True,capture_output=True)
    with ThreadPoolExecutor(max_workers=4) as pool:list(pool.map(run,jobs))
    paths=[raw/(j[0]+'.wav') for j in jobs]
    with ThreadPoolExecutor(max_workers=4) as pool:metrics=dict(zip(paths,pool.map(measure,paths)))
    gains={};rows=[];approved=[];pack=a.output/'Hoerpaket';pack.mkdir(exist_ok=True)
    for path in paths:
        peer=raw/(path.stem.rsplit('_',1)[0]+'_Nachher.wav')
        old=raw/(path.stem.rsplit('_',1)[0]+'_Vorher.wav')
        target=min(-25.0,*(metrics[q]['integrated_lufs']-3.1-metrics[q]['true_peak_dbfs'] for q in (old,peer)))
        gain=target-metrics[path]['integrated_lufs'];gains[path.stem]=gain
        # Quantize once to a WAV, encode FLAC and require byte-identical decode.
        x=read(path);assert np.max(np.abs(x))<0.9999
        out=pack/path.name;write(out,x*10**(gain/20))
        dest=out.with_suffix('.flac')
        subprocess.run(['ffmpeg','-v','error','-y','-i',str(out),'-c:a','flac','-compression_level','5',str(dest)],check=True)
        decoded=subprocess.run(['ffmpeg','-v','error','-i',str(dest),'-f','s16le','-'],capture_output=True,check=True)
        with wave.open(str(out)) as f:pcm=f.readframes(f.getnframes())
        assert not decoded.stderr and decoded.stdout==pcm,dest
        out.unlink();approved.append(dest)
        rows.append({'file':dest.name,**metrics[path],'gain_db':gain,'seconds':len(x)/SR})
    # Long enough to expose irregular weather, then unchanged performance on two cores.
    chunks=[];chapters=[];position=0
    for stem,start,length in [('Wind_Vorher',12,45),('Wind_Nachher',12,45),
                             ('Noise_Vorher',5,25),('Noise_Nachher',5,25),
                             ('Orbit_Raum_Vorher',0,23),('Orbit_Raum_Nachher',0,23),
                             ('Mist_Raum_Vorher',0,23),('Mist_Raum_Nachher',0,23)]:
        chapters.append(f'{position//60}:{position%60:02d} {stem}')
        chunks.extend([excerpt(read(raw/(stem+'.wav'))*10**(gains[stem]/20),start,length),np.zeros((SR,2))])
        position+=length+1
    tour=a.output/'Ambient_Klangreview_AB.wav';write(tour,np.concatenate(chunks))
    (pack/'Messwerte.json').write_text(json.dumps(rows,indent=2)+'\n')
    guide='''# AMBIENT Klangreview, 10. September 2026

Vorher = PR129 d69ad9e. Nachher = Sonic-Review-Korrekturen.
38 echte Firmware-Render, 44,1 kHz Stereo, 16 Bit. Wind und Noise jeweils
120 Sekunden isoliert, sechs Synths trocken und im Raum, fünf Welten.
Identische Noten/Regler je Paar, frische Prozesse. Keine externen Samples.
Pro Paar konstante Lautheitsanpassung auf höchstens -25 LUFS und mindestens
3 dB True-Peak-Reserve; kein Kompressor, EQ oder nachträglicher Hall.
Rohpegel und Verstärkung stehen in Messwerte.json. Einzelne Flauten werden
nicht hochgezogen. Naturähnlichkeit und bevorzugte Klangfarbe bitte hören;
Messwerte sind kein Qualitätsurteil.

Hörreihenfolge: Wind/Noise auf erkennbare Wiederholung und Pfeifen; Orbit
auf tragenden Grundton; Mist auf klaren Ton trotz Breite. Danach trocken/mit
Raum vergleichen und die fünf Welten auf Abstand zwischen Bett und Stimme.
Unveränderte trockene Synths dienen als Kontrollgruppe.

Kurzvergleich (jeweils Vorher, dann Nachher):
'''+ '\n'.join(chapters)+'\n'
    (pack/'Hoeranleitung.md').write_text(guide)
    approved.extend([pack/'Messwerte.json',pack/'Hoeranleitung.md'])
    archive=a.output/'Ambient_Klangreview_Hoerpaket.zip'
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
        for path in approved:z.write(path,path.name)
    print(json.dumps({'clips':len(rows),'chapters':chapters,'tour_seconds':position,'zip_bytes':archive.stat().st_size}))

if __name__=='__main__':main()
