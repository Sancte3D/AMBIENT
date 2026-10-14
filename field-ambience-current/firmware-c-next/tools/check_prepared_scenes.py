"""Keep exact baseline guards while scenes.c gains disabled prepared hooks."""
import subprocess
import tempfile
from pathlib import Path

def compare_inputs(repo,root,base,compiled):
    changed=[]
    scene_path=str(root.relative_to(repo)/'src/scenes.c')
    for p in compiled:
        before=subprocess.check_output(['git','show',f'{base}:{p}'],cwd=repo)
        if (repo/p).read_bytes()==before:
            continue
        assert p==scene_path, f'Unexpected existing input change: {p}'
        with tempfile.TemporaryDirectory(prefix='scene-preprocess-') as folder:
            old=Path(folder)/'scenes.c'; old.write_bytes(before)
            for profile in ([],['-DFAM_SOUND_PRODUCT']):
                flags=['cc','-E','-P','-std=c11','-I'+str(root/'include'),*profile]
                current=subprocess.check_output([*flags,str(repo/p)])
                original=subprocess.check_output([*flags,str(old)])
                assert current==original, f'Default Scene translation unit changed: {profile}'
        changed.append('src/scenes.c')
    return dict(existing_inputs_byte_equal=len(compiled)-len(changed),
                changed_inputs=changed,disabled_scene_translation_units_equal=['reference','product'],
                prepared_flag='FAM_SCENE_SAVE_ASYNC_PREPARED',prepared_flag_default=False)
