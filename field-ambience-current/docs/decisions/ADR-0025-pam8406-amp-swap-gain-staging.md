# ADR-0025 — PAM8406 replaces NRND PAM8403; speaker gain-staging + HPF (r19.37)

**Status:** Accepted — 2026-07-20
**Trigger:** (1) PAM8403 (U4) is **NRND** (Not Recommended for New Designs,
confirmed on the Diodes/DigiKey lifecycle field) — a hard no-go for a board that
is not yet routed. (2) External review flagged that the speaker amp is gain-staged
to clip its analog output well below DAC full-scale. Both are fixed together here.

## Problem

1. **NRND part.** Building an unrouted design around a not-recommended amplifier
   is exactly the kind of avoidable respin risk our sourcing rules exist to catch.
2. **Gain staging.** The speaker input was `PCM_VOUT → C_in 1 µF → R_VOL 20 kΩ (RI)
   → INL`. PAM8403/8406 gain = `2 × RF/RI` with RF ≈ 142 kΩ internal, so
   RI = 20 kΩ → **≈ +23 dB**. On a 5 V BTL amp into 8 Ω the clean output ceiling is
   only ~3.1 Vrms (~1.2 W). With +23 dB, the DAC's 2.1 Vrms full-scale asks the amp
   for ~30 Vrms — i.e. the analog stage saturates far below digital full-scale. The
   firmware limiter prevents PCM clipping but cannot prevent amp saturation.
3. **Speaker vs. content.** The 40 mm 8 Ω driver (CMS-402811-28SP / AS04008PS,
   fs ≈ 450 Hz) is fed the same full-range signal as the line-out, so bass content
   drives excursion the driver can't reproduce (ADR-0010 already noted sub-bass
   belongs on the line-out only).

## Decision

**Diodes PAM8406DR (LCSC C86270, lifecycle Active)** replaces PAM8403 (C17337),
plus a per-channel input-network re-value:

| Element | Was | Now | Why |
|---|---|---|---|
| U4 | PAM8403DR-H (C17337, **NRND**) | **PAM8406DR (C86270, Active)** | same 16-SOIC; newer same-family successor |
| R_VOL_L/R (RI) | 20 kΩ (C4184) | **174 kΩ (C22890)** | gain `2×142k/174k` = **+4.3 dB** |
| C_in_L/R | 1 µF (C15849) | **10 nF (C57112)** | with RI=174k → **speaker HPF ≈ 91 Hz** |
| Pin 9 | NC | **MODE → +5V** | PAM8406 pin 9 is MODE; High = Class-D |

Both new values **reuse LCSC parts already on the BOM** (C22890 = R_ILIM, C57112 =
C_BAT_FILT/C_COMP) — no new line items; net BOM goes 60 → 59 parts (the 20 kΩ
C4184 is no longer used anywhere).

### Gain budget (verified)

- DAC full-scale: **2.1 Vrms** (PCM5102A, TI datasheet).
- Amp clean ceiling on 5 V BTL / 8 Ω: differential swing ≈ 4.4 Vpp → **~3.1 Vrms
  → ~1.2 W**.
- +4.3 dB (×1.63): 2.1 Vrms × 1.63 = **3.4 Vrms** — the amp reaches its clean
  ceiling right around digital full-scale, clipping only in the top ~1 dB (above
  the firmware limiter). Proper staging: max digital ≈ max clean analog.

### Speaker high-pass (verified)

- `fc = 1/(2π · RI · C_in) = 1/(2π · 174k · 10n) ≈ 91 Hz` — protects the 8 Ω 40 mm
  driver from sub-bass excursion.
- **Line-out / headphone are untouched**: the TPA6132A2 branch taps the DAC through
  its own `C_HP_IN 1 µF` and stays full-range. The speaker treatment is local to
  the U4 input, exactly as required (no global HPF thinning the line-out).

## Verification (r19.37)

- **PAM8406 pinout** extracted directly from the Diodes PAM8406 datasheet
  (Rev 1-0, March 2013), Pin Descriptions p.2:
  `1 +OUT_L · 2 PGNDL · 3 -OUT_L · 4 PVDDL · 5 MUTE · 6 VDD · 7 INL · 8 VREF ·
  9 MODE · 10 INR · 11 GND · 12 SHDN · 13 PVDDR · 14 -OUT_R · 15 PGNDR · 16 +OUT_R`.
  Delta vs PAM8403: **pin 9 NC → MODE** (tied +5V) and output-polarity labels
  (pin 1 is +OUT_L, not -OUT_L) — both corrected in the generator symbol + nets.
- **Lifecycle Active** confirmed on DigiKey; supply 2.5–5.5 V (abs max 6.0 V);
  package **16-SOIC 3.90 mm** = identical to the PAM8403 footprint
  (`Package_SO:SOIC-16_3.9x9.9mm_P1.27mm`), so the land pattern is unchanged.
- Generator regenerated; `export_jlc_bom.py` → 59 parts / 200 placements, 0 NO-LCSC.
- Firmware host suite `./test/run_tests.sh` → **PASS** (no firmware change).

## Consequences

- **Footprint:** ⚠ confirm the C86270 land against the KiCad `SOIC-16_3.9x9.9` at
  GUI-ERC (identical package to the verified PAM8403, but buzz-check at bring-up).
- **MODE hard-tied +5V** = Class-D only (no runtime Class-AB). Acceptable — Class-D
  is the intended efficient mode; Class-AB was never used.
- **Loudness:** dropping +23 → +4.3 dB lowers max SPL, but max SPL now equals the
  amp's *clean* output; previously the top ~18 dB was distortion, not usable level.
- **Speaker tonality:** the 91 Hz HPF thins deep bass on the speaker (intended); the
  full range remains on J8 line-out / headphones.
- BOM cost: PAM8406 (~$0.19) ≈ PAM8403; net one fewer unique part.
