#!/usr/bin/env python3
"""Prueft die Identitaetsregel der World-Kandidaten (docs/worlds/world_candidates.json).

Regel: Klangquelle und Signatur-Effekt gibt es je nur einmal. In den uebrigen
vier Achsen (Tonvorrat, Zeitgefuehl, Raumklasse, Natur) duerfen sich zwei
Worlds hoechstens in zweien gleichen. Zusaetzlich: jeder Tonvorrat muss die
Harmonie-Zulassung bestehen (Nachbau von compatible() in engine_product.c:
kein Paar im Abstand einer Halbton- oder Tritonus-Klasse, +-45 Cent).

    check_world_identity.py [json] [--heute]   # --heute zeigt die Verstoesse des Ist-Stands
Exit 1 bei einem Verstoss der Kandidaten. --heute meldet nur.
"""
import itertools, json, pathlib, sys
REST = ('tonvorrat', 'zeit', 'raumklasse', 'natur')
def blocked(a, b):
    c = abs(a - b) % 1200; c = min(c, 1200 - c)
    return abs(c - 100) < 45 or abs(c - 600) < 45 or (3 <= abs(a - b) < 90)
def check(worlds, label):
    bad = []
    for axis in ('quelle', 'signatur'):
        seen = {}
        for w in worlds:
            if w[axis] == 'keine': bad.append(f'{w["name"]}: kein {axis}'); continue
            if w[axis] in seen: bad.append(f'{axis} doppelt: {seen[w[axis]]} und {w["name"]} ({w[axis]})')
            seen[w[axis]] = w['name']
    worst = 0
    for a, b in itertools.combinations(worlds, 2):
        same = [x for x in REST if a[x] == b[x]]; worst = max(worst, len(same))
        if len(same) > 2: bad.append(f'{a["name"]} / {b["name"]} gleichen sich in {len(same)} Achsen: {", ".join(same)}')
    print(f'{label}: {len(worlds)} Worlds, groesste Ueberdeckung {worst} von 4 Achsen, {len(bad)} Verstoesse')
    for line in bad: print('  -', line)
    return bad
if __name__ == '__main__':
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    path = pathlib.Path(args[0]) if args else pathlib.Path(__file__).resolve().parents[2] / 'docs/worlds/world_candidates.json'
    data = json.loads(path.read_text())
    fail = False
    for name, cents in data['tonvorraete'].items():
        tones = [c + 1200 * o for o in range(3) for c in cents]
        pairs = [(a, b) for a, b in itertools.combinations(tones, 2) if blocked(a, b)]
        if pairs: print(f'Tonvorrat {name}: {len(pairs)} gesperrte Paare'); fail = True
    print(f'Tonvorraete: {len(data["tonvorraete"])} geprueft, alle Paare zulaessig' if not fail else 'Tonvorraete: FEHLER')
    for w in data['worlds']:
        if w['tonvorrat'] not in data['tonvorraete']: print('unbekannter Tonvorrat', w['name']); fail = True
    if '--heute' in sys.argv: check(data['heute'], 'Ist-Stand V3 (nur Bericht)')
    fail |= bool(check(data['worlds'], 'Kandidaten'))
    sys.exit(1 if fail else 0)
