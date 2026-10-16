#!/usr/bin/env python3
"""SD47: real Room publication preemption, unchanged whole Product PCM, ARM delta."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
BASE='e2ede28c0ccc653f1d641741285d58eb7d02e587'
SOURCES=['engine_product','world_grammar','bowed','horn','pluck','ambient_room',
         'nature','dsp','shape','tuning','brain','cells']
def run(cmd,**kw): return subprocess.run(cmd,check=True,text=True,**kw)
def capture(cmd,**kw): return subprocess.check_output(cmd,text=True,**kw)
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('output',type=Path)
    ap.add_argument('--sanitize',action='store_true');ap.add_argument('--arm',action='store_true')
    args=ap.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    repo=Path(capture(['git','rev-parse','--show-toplevel'],cwd=ROOT).strip());prefix=ROOT.relative_to(repo)
    paths=capture(['git','ls-tree','-r','--name-only',BASE,'--',str(prefix/'src'),
        str(prefix/'include'),str(prefix/'cmake'),str(prefix/'CMakeLists.txt')],cwd=repo).splitlines()
    compiled=[p for p in paths if Path(p).suffix.lower() in ('.c','.h','.s','.ld','.cmake')
              or Path(p).name=='CMakeLists.txt']
    changed=[p for p in compiled if (repo/p).read_bytes()!=subprocess.check_output(['git','show',f'{BASE}:{p}'],cwd=repo)]
    assert len(compiled)==263 and changed==[str(prefix/'src/ambient_room.c')],changed
    baseline=subprocess.check_output(['git','show',f'{BASE}:{prefix}/src/ambient_room.c'],cwd=repo).decode()
    with tempfile.TemporaryDirectory(prefix='room-handoff-') as directory:
        tmp=Path(directory);old=tmp/'ambient_room.c';old.write_text(baseline)
        hooked=baseline.replace('    float t60=1.2f+3.6f*amount*amount;',
            '    float t60=1.2f+3.6f*amount*amount;\n    PUBLISH_POINT(0);PUBLISH_POINT(1);')
        for name,point in [('gain_bits[i]','(unsigned)i+2u'),('amount_bits','10'),('t60_bits','11')]:
            pattern=r'(atomic_store_explicit\(&'+re.escape(name)+r',[^;]+;)'
            hooked,n=re.subn(pattern,r'\1 PUBLISH_POINT('+point+');',hooked);assert n==1
        hooked=hooked.replace('PUBLISH_POINT(11);','PUBLISH_POINT(11);PUBLISH_POINT(12);')
        hooked='extern void ambient_room_publish_probe(unsigned);\n#define PUBLISH_POINT(p) ambient_room_publish_probe(p)\n'+hooked
        legacy=tmp/'observed_legacy.c';legacy.write_text(hooked)
        flags=['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(ROOT/'include')]
        test=tmp/'preemption';testsrc=str(ROOT/'test/test_room_handoff.c')
        run([*flags,testsrc,'-lm','-o',str(test)])
        native=json.loads(capture([str(test)]))
        run([*flags,'-DFAM_LEGACY_ROOM','-DROOM_IMPLEMENTATION="'+str(legacy)+'"',testsrc,'-lm','-o',str(tmp/'legacy')])
        broken=subprocess.run([str(tmp/'legacy')],capture_output=True,text=True)
        assert broken.returncode==1 and 'incoherent Room:' in broken.stderr,broken
        if args.sanitize:
            run([*flags,'-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer',testsrc,'-lm','-o',str(tmp/'san')])
            run([str(tmp/'san')],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),stdout=subprocess.DEVNULL)
        binaries=[]
        for label,room in [('baseline',old),('candidate',ROOT/'src/ambient_room.c')]:
            binary=tmp/label;binaries.append(binary)
            run([*flags,'-DFAM_SOUND_PRODUCT',str(ROOT/'tools/render_room_handoff.c'),
                *[str(room if s=='ambient_room' else ROOT/'src'/f'{s}.c') for s in SOURCES],'-lm','-o',str(binary)])
        cases=[]
        for world in range(3):
            for generated in (0,1):
                for nature in (0,1):
                    block_hashes=[]
                    for block in (64,512):
                        captures=[]
                        for i,binary in enumerate(binaries):
                            pcm=tmp/f'{i}.pcm';events=tmp/f'{i}.events'
                            run([str(binary),str(world),str(generated),str(block),str(nature),str(pcm),str(events)])
                            captures.append((pcm.read_bytes(),events.read_bytes()))
                        assert captures[0]==captures[1],(world,generated,block,nature)
                        assert len(captures[0][0])==12*44100*4
                        assert captures[0][1],(world,generated,block,nature,'no actual note hooks')
                        h=hashlib.sha256(captures[0][0]).hexdigest();block_hashes.append(h)
                        cases.append(dict(world=world,generated=generated,nature=nature,block=block,
                            frames=12*44100,pcm_sha256=h,events_sha256=hashlib.sha256(captures[0][1]).hexdigest()))
                    assert block_hashes[0]==block_hashes[1],(world,generated,nature,'block partition')
        arm=None
        if args.arm:
            build=out/'firmware'
            run(['cmake','-S',str(ROOT),'-B',str(build),'-DFAM_TARGET=h743','-DFAM_SOUND_PROFILE=product',
                '-DCMAKE_BUILD_TYPE=Release','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON','-UCMAKE_C_FLAGS'],
                stdout=subprocess.DEVNULL)
            run(['cmake','--build',str(build),'-j2'],stdout=subprocess.DEVNULL)
            run(['python3',str(ROOT/'tools/check_product_link.py'),str(build)],stdout=subprocess.DEVNULL)
            entries=json.loads((build/'compile_commands.json').read_text())
            entry=next(e for e in entries if Path(e['file']).name=='ambient_room.c')
            oldfolder=out/'baseline-object';oldfolder.mkdir(exist_ok=True)
            # The actual linker places Room BSS by object basename. Preserve
            # it in the paired link; a differently named .o changes RAM banks.
            command=shlex.split(entry['command']);oldobj=oldfolder/'ambient_room.c.obj'
            assert all(flag in command for flag in ('-mcpu=cortex-m7','-mthumb','-mfloat-abi=hard','-fstack-usage'))
            object_name=command[command.index('-o')+1]
            command[command.index('-o')+1]=str(oldobj)
            command[command.index(entry['file'])]=str(old)
            run(command,cwd=entry['directory'])
            link=shlex.split((build/'CMakeFiles/field_ambience_h743.dir/link.txt').read_text())
            assert object_name in link;link[link.index(object_name)]=str(oldobj)
            oldelf=out/'baseline-product';link[link.index('-o')+1]=str(oldelf)
            link=[('-Wl,-Map='+str(out/'baseline-product.map')) if a.startswith('-Wl,-Map=') else a for a in link]
            run(link,cwd=build,stdout=subprocess.DEVNULL)
            run(['arm-none-eabi-objcopy','-O','binary',str(oldelf),str(out/'baseline-product.bin')])
            def sizes(elf):
                rows=capture(['arm-none-eabi-size','-A',str(elf)]).splitlines()
                return {r.split()[0]:int(r.split()[1]) for r in rows if len(r.split())==3 and r.split()[1].isdigit()}
            previous=sizes(oldelf);current=sizes(build/'field_ambience_h743')
            old_symbols=capture(['arm-none-eabi-nm','-n',str(oldelf)])
            for name in ('tank','diffusion'):
                match=re.search(r'^([0-9a-f]+) [bB] '+name+r'$',old_symbols,re.MULTILINE)
                assert match and 0x30000000<=int(match[1],16)<0x30048000
            assert current['.bss']==previous['.bss'] and current['.dtcm_bss']==previous['.dtcm_bss']
            frames={}
            for path in [*sorted(build.rglob('ambient_room.c.su')),oldfolder/'ambient_room.c.su']:
                for line in path.read_text().splitlines():
                    fn,size,kind=line.rsplit('\t',2);assert kind=='static'
                    frames[('baseline:' if path.parent==oldfolder else 'candidate:')+fn.rsplit(':',1)[-1]]=int(size)
            obj=Path(entry['directory'])/object_name
            undefined=capture(['arm-none-eabi-nm','-u',str(obj)])
            assert '__atomic' not in undefined and 'publish_probe' not in undefined and 'accept_probe' not in undefined
            dis=capture(['arm-none-eabi-objdump','-d',str(obj)])
            process=dis.split('<ambient_room_process>:',1)[1].split('\n\n',1)[0]
            assert 'expf' not in process
            arm=dict(previous_sections=previous,current_sections=current,
                section_deltas={k:current.get(k,0)-previous.get(k,0) for k in current.keys()|previous.keys()
                    if current.get(k,0)!=previous.get(k,0)},
                baseline_bin_bytes=(out/'baseline-product.bin').stat().st_size,
                candidate_bin_bytes=(build/'field_ambience_h743.bin').stat().st_size,
                compiler_frames_bytes=frames,undefined_room_symbols=undefined.splitlines(),
                no_atomic_runtime_helper=True,device_timing_measured=False)
            (out/'room-arm.disassembly').write_text(dis)
    proof=dict(baseline_commit=BASE,compiled_inputs=263,existing_inputs_byte_equal=262,
        changed_input='src/ambient_room.c',native=native,baseline_counterexample=broken.stderr.strip(),
        sanitized=args.sanitize,paired_product_cases=cases,whole_pcm_and_events_byte_equal=True,
        pcm_frames_each_version=sum(c['frames'] for c in cases),arm=arm,
        reader_contract='One attempt, at most 11 SC word loads; busy/changed generation retains whole last target. No retry.',
        added_target_state_bytes=40,single_main_writer=True,device_accepted=False,
        source_sha256={p:sha(ROOT/p) for p in ('src/ambient_room.c','test/test_room_handoff.c',
            'tools/render_room_handoff.c','tools/review_room_handoff.py')})
    path=out/'SD47_ROOM_HANDOFF_METRICS.json';path.write_text(json.dumps(proof,indent=2,sort_keys=True)+'\n')
    artifact=out/'artifact';artifact.mkdir(exist_ok=True);shutil.copyfile(path,artifact/path.name)
    if args.arm:
        for name in ('field_ambience_h743','field_ambience_h743.bin','field_ambience_h743.map','PRODUCT_BUILD_AUDIT.json'):
            shutil.copyfile(out/'firmware'/name,artifact/name)
        shutil.copyfile(out/'room-arm.disassembly',artifact/'room-arm.disassembly')
    print(json.dumps(dict(status='passed',native=native,paired_pcm_cases=len(cases),
        baseline_counterexample=broken.stderr.strip(),arm=bool(arm),proof_sha256=sha(path))))
if __name__=='__main__':main()
