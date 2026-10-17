#!/usr/bin/env python3
"""SD47: actual source-owner reuse under IRQ preemption, whole PCM and ARM."""
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
BASE='6990f11d8ddbe0bf6a9676309b6589acd0b94b9b'
SOURCES=['engine_product','world_grammar','bowed','horn','pluck','ambient_room',
         'nature','dsp','shape','tuning','brain','cells']
def run(cmd,**kw): return subprocess.run(cmd,check=True,text=True,**kw)
def capture(cmd,**kw): return subprocess.check_output(cmd,text=True,**kw)
def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()

def observe_old_source(source):
    """Observation hooks only: keep the baseline's missing command cleanup."""
    before='uint8_t owner=slots[i].owner;engine_note_off(owner);'
    assert source.count(before)==1
    return ('extern void engine_source_handoff_probe(unsigned,unsigned char);\n'+
        source.replace(before,'uint8_t owner=slots[i].owner;'
            'engine_source_handoff_probe(0,owner);engine_note_off(owner);'
            'engine_source_handoff_probe(1,owner);'))

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
    assert len(compiled)==263 and changed==[str(prefix/'src/engine_product.c')],changed
    baseline=subprocess.check_output(['git','show',f'{BASE}:{prefix}/src/engine_product.c'],cwd=repo).decode()
    with tempfile.TemporaryDirectory(prefix='source-handoff-') as directory:
        tmp=Path(directory);old=tmp/'engine_product.c';old.write_text(baseline)
        observed=tmp/'observed_engine.c';observed.write_text(observe_old_source(baseline))
        flags=['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-DFAM_SOUND_PRODUCT','-I'+str(ROOT/'include')]
        other=[str(ROOT/'src'/f'{name}.c') for name in SOURCES if name!='engine_product']
        testsrc=str(ROOT/'test/test_source_handoff.c');test=tmp/'preemption'
        run([*flags,testsrc,*other,'-lm','-o',str(test)])
        native=json.loads(capture([str(test)]))
        legacy=tmp/'legacy'
        run([*flags,'-DENGINE_IMPLEMENTATION="'+str(observed)+'"',testsrc,*other,'-lm','-o',str(legacy)])
        broken=[]
        for world in range(3):
            result=subprocess.run([str(legacy),'--quick-reuse',str(world)],capture_output=True,text=True)
            assert result.returncode==1 and 'engine_active_voices()==1' in result.stderr,result
            broken.append(dict(world=world,returncode=result.returncode,failure=result.stderr.strip()))
        if args.sanitize:
            sanitized=tmp/'sanitized'
            run([*flags,'-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer',
                 testsrc,*other,'-lm','-o',str(sanitized)])
            sanitized_native=json.loads(capture([str(sanitized)],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')))
            assert native==sanitized_native,(native,sanitized_native)
        # Reuse the real 12 s Product scenario, including Dry/Room edits,
        # actual generator/manual notes, Nature, Clear and a fresh restart.
        binaries=[]
        for label,engine in [('baseline',old),('candidate',ROOT/'src/engine_product.c')]:
            binary=tmp/label;binaries.append(binary)
            run([*flags,str(ROOT/'tools/render_room_handoff.c'),str(engine),*other,'-lm','-o',str(binary)])
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
                        assert len(captures[0][0])==12*44100*4 and captures[0][1]
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
            entry=next(e for e in entries if Path(e['file']).name=='engine_product.c')
            oldfolder=out/'baseline-object';oldfolder.mkdir(exist_ok=True)
            command=shlex.split(entry['command']);oldobj=oldfolder/'engine_product.c.obj'
            assert all(flag in command for flag in ('-mcpu=cortex-m7','-mthumb','-mfloat-abi=hard','-fstack-usage'))
            object_name=command[command.index('-o')+1]
            command[command.index('-o')+1]=str(oldobj);command[command.index(entry['file'])]=str(old)
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
            for section in ('.bss','.dtcm_bss','.d2_bss','.data'):
                assert current.get(section,0)==previous.get(section,0),(section,current,previous)
            obj=Path(entry['directory'])/object_name
            render_sections=[]
            for label,path in [('baseline',oldobj),('candidate',obj)]:
                raw=out/f'{label}-render.bin'
                run(['arm-none-eabi-objcopy','--dump-section',f'.text.engine_render={raw}',str(path)])
                assert raw.stat().st_size>0;render_sections.append(raw.read_bytes())
            assert render_sections[0]==render_sections[1],'Audio render instruction bytes changed'
            undefined=capture(['arm-none-eabi-nm','-u',str(obj)])
            assert '__atomic' not in undefined and 'handoff_probe' not in undefined
            frames={}
            for path in [*sorted(build.rglob('engine_product.c.su')),oldfolder/'engine_product.c.su']:
                for line in path.read_text().splitlines():
                    fn,size,kind=line.rsplit('\t',2);assert 'unbounded' not in kind
                    frames[('baseline:' if path.parent==oldfolder else 'candidate:')+fn.rsplit(':',1)[-1]]=int(size)
            assert frames['baseline:engine_render']==frames['candidate:engine_render']
            dis=capture(['arm-none-eabi-objdump','-dr',str(obj)])
            (out/'source-arm.disassembly').write_text(dis)
            arm=dict(previous_sections=previous,current_sections=current,
                section_deltas={k:current.get(k,0)-previous.get(k,0) for k in current.keys()|previous.keys()
                    if current.get(k,0)!=previous.get(k,0)},
                baseline_bin_bytes=(out/'baseline-product.bin').stat().st_size,
                candidate_bin_bytes=(build/'field_ambience_h743.bin').stat().st_size,
                compiler_frames_bytes=frames,audio_render_instruction_bytes_equal=True,
                audio_render_section_bytes=len(render_sections[0]),
                audio_render_sha256=hashlib.sha256(render_sections[0]).hexdigest(),
                no_atomic_runtime_helper=True,no_test_hook_in_arm=True,device_timing_measured=False)
    proof=dict(baseline_commit=BASE,compiled_inputs=263,existing_inputs_byte_equal=262,
        changed_input='src/engine_product.c',native=native,baseline_counterexamples=broken,
        sanitized=args.sanitize,paired_product_cases=cases,whole_pcm_and_events_byte_equal=True,
        pcm_frames_each_version=sum(c['frames'] for c in cases),arm=arm,
        added_state_bytes=0,main_only_owner_command_clears=2,single_main_writer=True,
        ownership_contract='Main alone posts requests. Audio only consumes; clear old bits after owner is free and before publishing its new source.',
        atomic_rmw_limit='Two fixed Main word RMW operations; Cortex-M exclusive-store retries are possible after IRQ. No new audio operation or hardware WCET claim.',
        device_accepted=False,source_sha256={p:sha(ROOT/p) for p in ('src/engine_product.c',
            'test/test_source_handoff.c','tools/render_room_handoff.c','tools/review_source_handoff.py')})
    path=out/'SD47_SOURCE_HANDOFF_METRICS.json';path.write_text(json.dumps(proof,indent=2,sort_keys=True)+'\n')
    artifact=out/'artifact';artifact.mkdir(exist_ok=True);shutil.copyfile(path,artifact/path.name)
    if args.arm:
        for name in ('field_ambience_h743','field_ambience_h743.bin','field_ambience_h743.map','PRODUCT_BUILD_AUDIT.json'):
            shutil.copyfile(out/'firmware'/name,artifact/name)
        shutil.copyfile(out/'source-arm.disassembly',artifact/'source-arm.disassembly')
    print(json.dumps(dict(status='passed',native=native,paired_pcm_cases=len(cases),
        baseline_worlds_failing=len(broken),arm=bool(arm),proof_sha256=sha(path))))

if __name__=='__main__':main()
