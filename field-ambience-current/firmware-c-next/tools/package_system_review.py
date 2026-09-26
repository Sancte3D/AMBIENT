#!/usr/bin/env python3
"""Package 21 verified C renders: constant gain, matched comparisons and headroom."""
import concurrent.futures,json,shutil,subprocess,sys,zipfile
from pathlib import Path
import numpy as np
from package_musical_review import measure,excerpt,write
from render_system_review import verify_flac,SR

def sample(path,start,length):
    p=subprocess.run(['ffmpeg','-v','error','-ss',str(start),'-i',str(path),'-t',str(length),'-f','s16le','-acodec','pcm_s16le','-'],capture_output=True,check=True)
    if p.stderr or len(p.stdout)!=length*SR*4:raise RuntimeError('Invalid tour excerpt '+str(path))
    return excerpt(np.frombuffer(p.stdout,dtype='<i2').reshape(-1,2).astype(np.float64)/32768)

def main(root):
    root=Path(root);pack=root/'Ambient_System_Hoerpaket';pack.mkdir(exist_ok=True)
    paths=sorted((root/'raw').glob('*.flac'))
    if len(paths)!=21:raise RuntimeError('Expected 21 complete renders')
    raw={p:json.loads(p.with_suffix('.json').read_text()) for p in paths}
    for p in paths:
        verify_flac(p,raw[p]['frames'],raw[p]['pcm_sha256'])
        if raw[p]['clipped_samples'] or raw[p]['collision_failures']:raise RuntimeError('Failed render '+str(p))
    with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:metrics=dict(zip(paths,pool.map(measure,paths)))
    groups={};gains={}
    for p in paths:
        key=p.stem.replace('_Nachher','').replace('_Vorher','')
        if key in ['Fjords','Fjords_Autoplay']:key='Fjords_AB'
        groups.setdefault(key,[]).append(p)
    for items in groups.values():
        target=min([-23.]+[metrics[p]['integrated_lufs']-3.1-metrics[p]['true_peak_dbfs'] for p in items])
        for p in items:gains[p]=target-metrics[p]['integrated_lufs']
    rows=[];delivered={}
    for p in paths:
        m=raw[p];folder={'autoplay':'01_Autoplay','steal':'02_Stimmwechsel','macro':'03_Regler','switch':'04_Synthwechsel'}[m['kind']]
        dest=pack/folder/p.name;dest.parent.mkdir(exist_ok=True);tmp=dest.with_suffix('.partial.flac')
        proc=subprocess.run(['ffmpeg','-v','error','-y','-i',str(p),'-af',f'volume={gains[p]:.10f}dB','-c:a','flac','-compression_level','5','-sample_fmt','s16',str(tmp)],capture_output=True,check=True)
        if proc.stderr:raise RuntimeError(proc.stderr.decode())
        digest=verify_flac(tmp,m['frames']);checked=measure(tmp)
        if checked['true_peak_dbfs']>-3:raise RuntimeError('Headroom gate failed')
        tmp.replace(dest);delivered[p.stem]=dest
        rows.append({'file':str(dest.relative_to(pack)),'seconds':m['seconds'],'gain_db':gains[p],
                     'native':{k:m[k] for k in ['peak_dbfs','rms_dbfs','dc','clipped_samples','collision_failures','melody_notes','melody_unique']},
                     'delivered':checked,'pcm_sha256':digest})
        print(dest.name,checked,flush=True)
    picks=[('Fjords_Vorher',5,24),('Fjords_Autoplay',5,32),('Fjords_Autoplay',680,26),('Alps_Autoplay',35,18),('Open_Sea_Autoplay',35,18),('Moss_Autoplay',35,18),('Desert_Autoplay',35,18),('Mist_Regler',0,36)]
    tour=[];chapters=[];pos=0
    for name,start,length in picks:
        chapters.append(f'{pos//60}:{pos%60:02d} — {name}, ab Sekunde {start}')
        tour.append(sample(delivered[name],start,length));pos+=length
    write(root/'Ambient_System_Soundtour.wav',np.concatenate(tour))
    shutil.copyfile(delivered['Fjords_Autoplay'],root/'Ambient_Fjords_20_Minuten.flac')
    logs=pack/'Ereignisse';logs.mkdir(exist_ok=True)
    for p in paths:shutil.copyfile(p.with_suffix('.json'),logs/p.with_suffix('.json').name)
    (pack/'Messwerte.json').write_text(json.dumps(rows,indent=2)+'\n')
    guide='''# AMBIENT — System und Klang

21 echte Render aus der C-Firmware, insgesamt 50:10 Minuten. 44,1 kHz,
Stereo, 16 Bit, verlustfreies FLAC. Originale generative bzw. gespielte
Passagen; keine externen Musikaufnahmen. Referenz ist der erste Sound-Pass
(a14f6e3 lokal; gleicher Tree wie GitHub 2d4880b), danach die Systementwicklung.

## Anhören

1. **01_Autoplay:** Alps, Open Sea, Moss und Desert je vier Minuten;
   Fjords als voller 20-Minuten-Verlauf. Fjords_Vorher dauert drei Minuten.
   Gleicher Seed, durch neue Regeln unterschiedliche Noten: Systemvergleich,
   keine identische Performance. Auf Pausen, Wiederkehr und Melodiebewegung achten.
2. **02_Stimmwechsel:** Horn, Bowed, Choir jeweils vorher/nachher mit derselben
   dichten Folge aus fünf Tasten. Auf Anschlüsse und Knackser achten. Jede dieser
   Stimmen bleibt dreistimmig; weitere Tasten ersetzen alte Stimmen sanft.
3. **03_Regler:** alle sechs Synths. 0–9 s neutral; 9–18 s heller/resonanter;
   18–27 s Sweep/EnvMod; ab 27 s langsamer Attack und längerer Release.
   Mist zusätzlich vorher. Die Reglerwirkung darf zum Klangcharakter passen.
4. **04_Synthwechsel:** dieselbe Folge aus acht Instrumentwechseln bei laufendem
   Raum, vorher/nachher. Auf Niveau, Ausklang und Übergänge achten.

## Lautheit und Nachweis

Pro Datei ein konstanter Gain, Ziel −23 LUFS und mindestens 3 dB True-Peak-
Reserve. Vorher/Nachher-Paare haben dieselbe Ziellautheit. Kein zusätzlicher
Kompressor oder Limiter. Reglerabschnitte werden nicht einzeln normalisiert,
so bleibt ihre Pegelwirkung erhalten. Alle Roh-FLACs sind nach Dekodierung
bytegleich zum erzeugten PCM geprüft. Alle gelieferten FLACs sind auf vollständige
Dekodierung, exakte Länge und Headroom geprüft. Noten und Messwerte liegen bei.

Die 3:10-Soundtour verwendet nur 200-ms-Fades an den Schnittkanten:

'''+''.join('- '+c+'\n' for c in chapters)+'''
## Noch offen

Firmware-Render, keine Aufnahme des fertigen Geräts. Technisch geprüft;
eine subjektive Hörabnahme wird damit nicht behauptet. Endgültige Balance auf
Kopfhörer und vorgesehenem Lautsprecher sowie DWT-Spitzenlast und Deadline-
Reserve auf dem STM32H743 bleiben zu prüfen. Das Tonhöhen-Gedächtnis schätzt
Ausklänge konservativ, analysiert den Hall nicht spektral. Modalfarben dürfen
bei dichtem Klang ausbleiben. V2-Synths sind monophon; Autoplay gehört zu Ambient.

Reproduktion: tools/render_system_review.py und tools/package_system_review.py.
Musikalische Architektur: docs/audio/MUSICAL_SYSTEM_2026-09-09.md im Repository.
'''
    (pack/'START_HIER.md').write_text(guide)
    archive=root/'Ambient_System_Hoerpaket.zip'
    with zipfile.ZipFile(archive,'w',compression=zipfile.ZIP_STORED) as z:
        # Explicit manifest: stale/interrupted .partial exports must never ship.
        approved=[pack/r['file'] for r in rows]
        approved += [logs/p.with_suffix('.json').name for p in paths]
        approved += [pack/'START_HIER.md',pack/'Messwerte.json']
        for p in sorted(approved):z.write(p,p.relative_to(root))
    with zipfile.ZipFile(archive) as z:
        if z.testzip():raise RuntimeError('ZIP CRC failure')
    result={'clips':len(rows),'seconds':sum(r['seconds'] for r in rows),'tour_seconds':pos,
            'worst_true_peak_dbfs':max(r['delivered']['true_peak_dbfs'] for r in rows),'zip_mib':archive.stat().st_size/1048576}
    (root/'package_result.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
if __name__=='__main__':main(sys.argv[1])
