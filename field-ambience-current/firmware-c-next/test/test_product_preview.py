#!/usr/bin/env python3
"""An exported window must observe the same running C/PCM stream."""
import csv
from pathlib import Path
import subprocess
import sys
import tempfile
import wave

binary = Path(sys.argv[1]).resolve()
SR = 44100


def read(path):
    with wave.open(str(path), 'rb') as wav:
        assert (wav.getnchannels(), wav.getsampwidth(), wav.getframerate(), wav.getnframes()) == (2, 2, SR, 27 * SR)
        return wav.readframes(wav.getnframes())


with tempfile.TemporaryDirectory(prefix='ambient-preview-test-') as directory:
    temp = Path(directory)
    for world in range(3):
        for mode in ('world', 'activity_low', 'activity_high'):
            first, later = temp / f'{world}_{mode}_first.wav', temp / f'{world}_{mode}_later.wav'
            subprocess.run([str(binary), str(world), '0x1234', mode, str(first)], check=True, capture_output=True)
            subprocess.run([str(binary), str(world), '0x1234', mode, str(later), '3'], check=True, capture_output=True)
            full, excerpt = read(first), read(later)
            # No reset, fast-forwarded clock, changed tick cadence or sample gap
            # may be hidden by a later export. This includes a partial first block.
            assert full[3 * SR * 4:] == excerpt[:24 * SR * 4], (world, mode)
            with open(str(later) + '.events.csv') as stream:
                for event in csv.DictReader(stream):
                    frame, absolute = int(event['frame']), int(event['absolute_frame'])
                    assert 0 <= frame < 27 * SR and absolute == frame + 3 * SR
    off, on = temp / 'off.wav', temp / 'on.wav'
    for mode, path in [('world', off), ('world_nature', on)]:
        subprocess.run([str(binary), '0', '1731', mode, str(path), '9'], check=True, capture_output=True)
        read(path)
    assert Path(str(off) + '.events.csv').read_bytes() == Path(str(on) + '.events.csv').read_bytes()
    assert off.read_bytes() != on.read_bytes(), 'The optional Nature fixture must actually change PCM'
    invalid = temp / 'invalid.wav'
    cases = [
        ['-1', '1', 'world'], ['3', '1', 'world'], ['1x', '1', 'world'],
        ['0', 'bad', 'world'], ['0', '4294967296', 'world'], ['0', '1', 'unknown'],
        ['0', '1', 'dry', '1'], ['0', '1', 'world', '1801'],
        ['0', '1', 'world', '-1'], ['0', '1', 'world', '1.5'],
        ['0', '1', 'transition'], ['0', '1', 'transition', '0', '0'],
        ['0', '1', 'world', '0', '1'],
    ]
    for args in cases:
        command = [str(binary), *args[:3], str(invalid), *args[3:]]
        result = subprocess.run(command, capture_output=True)
        assert result.returncode == 2 and not invalid.exists(), command

print('PASS: later exports are byte-identical observations, Nature preserves events, invalid fixtures reject before writing')
