"""Source in GDB to DEFINE ambient-capture. Does not connect, halt or resume."""
import json
from pathlib import Path
import sys
import gdb

sys.path.insert(0,str(Path(__file__).resolve().parent))
from device_capture import collect

class AmbientCapture(gdb.Command):
    def __init__(self):
        super().__init__('ambient-capture',gdb.COMMAND_USER)

    def invoke(self,arg,from_tty):
        try:
            args=gdb.string_to_argv(arg)
            if len(args)!=3:
                raise ValueError('ambient-capture MANIFEST OUTPUT.json "scenario label"')
            inferior=gdb.selected_inferior()
            threads=inferior.threads()
            if not threads or not all(t.is_stopped() for t in threads):
                raise ValueError('Target must already be stopped; this command will not halt it')
            manifest=json.loads(Path(args[0]).read_text())
            names=('pc','sp','xpsr')+ (('control','msp') if manifest.get('device_bench') else ())
            registers={n:int(gdb.parse_and_eval('$'+n)) for n in names}
            result=collect(manifest,
                           inferior.read_memory,registers,args[2])
            if not all(t.is_stopped() for t in inferior.threads()):
                raise ValueError('Target resumed during capture')
            # Never silently overwrite a previous device record.
            with Path(args[1]).open('x') as out:
                json.dump(result,out,indent=2);out.write('\n')
            gdb.write('Saved observation; device acceptance remains pending.\n')
        except (ValueError,OSError,KeyError,TypeError,gdb.error) as error:
            raise gdb.GdbError(str(error)) from error

AmbientCapture()
