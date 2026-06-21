// =========================================================================
// FIELD AMBIENCE v29-p — Master-Vol/Mute Verification Test
// =========================================================================
//
// Dieser Test verifiziert dass nach v29-p Fix:
//   1. setMasterVol(0) wirklich ALLE Voices mutet (auch dry)
//   2. JackDetect + ampShutdown den Output silencen
//   3. Limiter den Final-Output begrenzt
//
// Usage:
//   1. SuperCollider öffnen
//   2. field_ambience_v29o.scd laden + ~famStart.()
//   3. Dann diesen Test ausführen:
//      thisProcess.interpreter.executeFile("field_ambience_v29p_test.scd");
//
// Expected:
//   - Bei masterVol=0 muss die Server-Peak-Meter-Anzeige auf 0 fallen
//   - Bei jackdetect=1 muss der Output silent sein (Kopfhörer-Detect)
//   - Bei ampshutdown=0 muss der Output silent sein (Amp aus)
//   - Bei masterVol=0.5 darf der Peak-Pegel nicht über -1 dBFS (Limiter @ 0.85)
// =========================================================================

(
"=== v29-p Master-Vol/Mute Verification ===".postln;
"".postln;

// Test 1: Spawn a loud test tone direkt via voice-spawn-API
"TEST 1: Spawning test cell hold...".postln;
~famStart.();
0.5.wait;
~famHold.(1, 1);   // start cell 1 hold
0.5.wait;
"  → expect: audio output, peak meter visible".postln;
"".postln;

// Test 2: Master Vol auf 0 setzen
"TEST 2: setMasterVol(0) — expect SILENCE within 250ms (Lag.kr smoothing)".postln;
~famSetMasterVol.(0);
1.0.wait;
"  → check: peak meter at 0?".postln;
"  → if NOT 0: v29-p DryBus fix did not work — dry signal bypassing masterVol".postln;
"".postln;

// Test 3: Master Vol back to 0.5
"TEST 3: setMasterVol(0.5) — restore audio".postln;
~famSetMasterVol.(0.5);
1.0.wait;
"  → expect: audio back at ~50%".postln;
"".postln;

// Test 4: Jack Detect (Kopfhörer eingesteckt → Amp aus)
"TEST 4: jackdetect=1 (Kopfhörer-Insertion) — expect SILENCE".postln;
~famJackInserted = true;
~famSetMasterVol.(~famMasterVol);   // re-apply (jack-detect modifier kicks in)
1.0.wait;
"  → check: peak meter at 0?".postln;
"".postln;

// Test 5: Jack ausgesteckt
"TEST 5: jackdetect=0 — audio back on".postln;
~famJackInserted = false;
~famSetMasterVol.(~famMasterVol);
1.0.wait;
"  → expect: audio resumes".postln;
"".postln;

// Test 6: Amp-Shutdown
"TEST 6: ampShutdown — expect SILENCE".postln;
~famAmpEnabled = false;
~famSetMasterVol.(~famMasterVol);
1.0.wait;
"  → check: peak meter at 0?".postln;
"".postln;

// Test 7: Amp wieder an
"TEST 7: ampEnabled=true".postln;
~famAmpEnabled = true;
~famSetMasterVol.(~famMasterVol);
1.0.wait;
"".postln;

// Test 8: Limiter (loud bursts)
"TEST 8: Limiter sanity check — spawn 5 holds simultaneously".postln;
~famHold.(2, 1);
~famHold.(3, 1);
~famHold.(4, 1);
~famHold.(5, 1);
0.5.wait;
"  → expect: NO digital clipping (Limiter @ 0.85 prevents peaks > -1.4 dBFS)".postln;
1.0.wait;
~famHoldClear.();
"".postln;

"=== Test fertig — Stop with: ~famStop.() ===".postln;
)
