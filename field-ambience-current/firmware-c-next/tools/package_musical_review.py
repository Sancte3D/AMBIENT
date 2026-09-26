#!/usr/bin/env python3
"""Package two render directories. Requires numpy and ffmpeg on the host.

python3 tools/package_musical_review.py /tmp/ambient-listening
Input: raw_before/ (worlds), raw_after/ (all). Output: FLAC pack, WAV previews,
measurements and listening guide. Gain matching is constant, never compression.
"""
import concurrent.futures
import json
from pathlib import Path
import re
import subprocess
import sys
import wave
import zipfile

import numpy as np

SR=44100


def measure(path):
    p=subprocess.run(['ffmpeg','-hide_banner','-nostats','-i',str(path),'-af',
                      'ebur128=peak=true:framelog=verbose','-f','null','-'],capture_output=True,text=True,check=True)
    summary=p.stderr.rsplit('Summary:',1)[1]
    return {'integrated_lufs':float(re.search(r'I:\s*([-\d.]+) LUFS',summary)[1]),
            'true_peak_dbfs':float(re.search(r'Peak:\s*([-\d.]+) dBFS',summary)[1])}


def read(path):
    with wave.open(str(path)) as f:
        assert (f.getframerate(),f.getnchannels(),f.getsampwidth())==(SR,2,2)
        return np.frombuffer(f.readframes(f.getnframes()),np.int16).reshape(-1,2).astype(np.float64)/32768


def write(path,x):
    assert np.max(np.abs(x))<1
    with wave.open(str(path),'wb') as f:
        f.setnchannels(2);f.setsampwidth(2);f.setframerate(SR)
        f.writeframes(np.rint(np.clip(x,-1,1)*32767).astype(np.int16).tobytes())


def excerpt(x,start=0,seconds=None):
    out=x[round(start*SR):round((start+seconds)*SR) if seconds else len(x)].copy()
    fade=min(round(.2*SR),len(out)//2)
    out[:fade]*=np.linspace(0,1,fade)[:,None];out[-fade:]*=np.linspace(1,0,fade)[:,None]
    return out


def main(root):
    root=Path(root);pack=root/'Ambient_Hoerpaket';pack.mkdir(exist_ok=True)
    paths=sorted((root/'raw_before').glob('*.wav'))+sorted((root/'raw_after').glob('*.wav'))
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        measured=dict(zip(paths,pool.map(measure,paths)))
    groups={}
    for path in paths:
        group=path.stem
        if group.startswith('Synth_'): group=group.rsplit('_',1)[0]
        if group.startswith('FX_'): group='All_FX'
        groups.setdefault(group,[]).append(path)
    gains={}
    for group,items in groups.items():
        if group=='All_FX':
            # Same excitation gain for all effects preserves their level behaviour.
            gain=min(-3-measured[p]['true_peak_dbfs'] for p in items)
            gains.update({p:gain for p in items})
        else:
            target=min([-23]+[measured[p]['integrated_lufs']-1-measured[p]['true_peak_dbfs'] for p in items])
            gains.update({p:target-measured[p]['integrated_lufs'] for p in items})
    rows=[]
    for p in paths:
        before=p.parent.name=='raw_before'
        folder='01_Welten_AB' if p.name.startswith('World') else '02_Synths' if p.name.startswith('Synth') else '03_Effekte'
        dest=pack/folder;dest.mkdir(exist_ok=True)
        name=p.stem+('_Vorher' if before else '_Nachher')
        output=dest/(name+'.flac')
        subprocess.run(['ffmpeg','-v','error','-y','-i',str(p),'-af',f'volume={gains[p]:.8f}dB',
                        '-c:a','flac','-sample_fmt','s16',str(output)],check=True)
        checked=measure(output)
        row={'file':str(output.relative_to(pack)),**measured[p],'gain_db':round(gains[p],3),
             'delivered_lufs':checked['integrated_lufs'],'delivered_true_peak_dbfs':checked['true_peak_dbfs']}
        assert checked['true_peak_dbfs']<=-0.8,row
        rows.append(row)
    silence=np.zeros((SR,2))
    before=root/'raw_before/World_3_Fjords.wav';after=root/'raw_after/World_3_Fjords.wav'
    write(root/'Ambient_Fjords_AB.wav',np.concatenate([excerpt(read(before)*10**(gains[before]/20)),silence,
                                                    excerpt(read(after)*10**(gains[after]/20))]))
    tour=[];chapters=[];position=0
    picks=[(p,1,13) for p in sorted((root/'raw_after').glob('World*.wav'))]
    picks += [(p,1,9) for p in sorted((root/'raw_after').glob('Synth*Dream.wav'))]
    for path,start,length in picks:
        chapters.append(f'{position//60}:{position%60:02d} — {path.stem}')
        tour.extend([excerpt(read(path)*10**(gains[path]/20),start,length),silence]);position+=length+1
    write(root/'Ambient_Soundtour.wav',np.concatenate(tour))
    (pack/'Messwerte.json').write_text(json.dumps(rows,indent=2)+'\n')
    for version in ['before','after']:
        (pack/f'Engine_Render_{version}.json').write_bytes((root/f'raw_{version}/render_manifest.json').read_bytes())
    guide='''# AMBIENT — Hörpaket

Originale, fest gespielte Passagen aus der C-Firmware: 44,1 kHz, Stereo,
16 Bit, verlustfrei als FLAC. Keine KI-Musik und keine externen Samples.

## Anhören

- **01_Welten_AB:** fünf Welten, jeweils Vorher/Nachher mit identischen Noten,
  Anschlagstärken und Zeitpunkten. Auf gehaltene Melodie, leise Anschläge,
  Abstand zur Fläche und Hall nach dem Loslassen achten.
- **02_Synths:** Acid, FM Glass, Mist, Storm, Orbit, Bamboo, jeweils Dry/Dream.
  Die letzten zwei Noten verändern einen benannten Kernparameter.
- **03_Effekte:** derselbe kurze Bamboo-Impuls durch alle neun Modi. Beim
  Swell zusätzlich ein ausdrücklich vorausgeplanter Anschweller. Hall und
  Shimmer sind subtile Räume; Chorus/Blur/Tape formen vor allem den Klangkörper.

Vorher/Nachher und Dry/Dream sind pro Paar auf gleiche integrierte Lautheit
abgeglichen (Ziel −23 LUFS, bei Bedarf niedriger für mindestens 1 dB Peakreserve).
Dazu dient ausschließlich ein konstanter Gain; kein Kompressor, Limiter oder
dynamisches Loudness-Normalizing. Alle neun Effektbeispiele haben denselben
Gain, damit ihre Pegelwirkung vergleichbar bleibt. Messwerte und angewandte
Gains stehen in Messwerte.json; die Rohmessungen und Ereignisse in Engine_Render_*.

Der Vergleichsstand ist Commit 24d0a2be2a0cd3f2c04e210824ea9eee89a795d5
(Sound-Branch von PR #126), nicht der abweichende Main-Stand.
Die neuen Render entsprechen dem beigefügten Sound-PR. Erzeugung:
tools/render_musical_review.py, Verpackung: tools/package_musical_review.py.

Die zusätzlichen WAV-Hördateien sind Zusammenschnitte mit 200-ms-Randfades:
Ambient_Fjords_AB.wav: 0:00 vorher, 0:51 nachher. Ambient_Soundtour.wav:

'''+''.join(f'- {c}\n' for c in chapters)+'''
## Stand der Prüfung

Host-Render und technische Regressionstests können Klangverhalten und
Ausgangspegel prüfen. DWT-Spitzenlast, DAC/Verstärker und der reale Lautsprecher
müssen am H743-Gerät geprüft werden. Diese Dateien sind Firmware-Render,
keine Aufnahme vom fertigen Instrument. Die endgültige Klangabstimmung bleibt
eine Hörentscheidung, idealerweise mit Kopfhörer und dem vorgesehenen Speaker.
'''
    (pack/'START_HIER.md').write_text(guide)
    with zipfile.ZipFile(root/'Ambient_Hoerpaket.zip','w',compression=zipfile.ZIP_STORED) as z:
        for p in sorted(pack.rglob('*')):
            if p.is_file():z.write(p,p.relative_to(root))
    print(json.dumps({'files':len(rows),'preview_seconds':position,'max_delivered_true_peak':max(r['delivered_true_peak_dbfs'] for r in rows),
                      'zip_bytes':(root/'Ambient_Hoerpaket.zip').stat().st_size},indent=2))


if __name__=='__main__':main(sys.argv[1])
