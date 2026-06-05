#ifndef FAM_DRONE_H
#define FAM_DRONE_H

/*
 * Drone — Step 12b #2. A single sustained pad-style voice on the key root,
 * toggled by the DRONE modifier. Ported from the webapp `spawnDrone`
 * (_makePadVoice with the slow drone params), but with one deliberate change
 * dictated by the "sound darf nicht konkurrieren" rule:
 *
 *   The webapp captures the key at spawn and only re-roots on a fresh spawn.
 *   Here the drone FOLLOWS the key live with portamento — when the key
 *   changes while the drone sounds, its pitch glides to the new root instead
 *   of clashing. (A frozen C drone under cells now playing in D is the worst
 *   kind of harmonic competition; the glide resolves it.)
 *
 * Two detuned saw sides → resonant lowpass (slow LFO + filter envelope) →
 * Haas widening → slow 6 s bloom / 4 s release. Mono-rooted but stereo-wide.
 * Single voice, never stolen. Mixes into the engine's dry + reverb-send buses
 * with its own fixed send (0.45), like bass/texture.
 */

#include <stdint.h>
#include <stdbool.h>

void drone_init(void);

/* Set the drone root (the key tonic, MIDI). From idle this just stores the
 * target; while sounding the pitch glides there (portamento). */
void drone_set_root_midi(int midi);

/* Toggle the drone. enable=true blooms it in from idle (6 s attack) at the
 * current root; enable=false fades it out (4 s tail). */
void drone_enable(bool on);

/* True while the drone is producing sound (incl. release tail). */
bool drone_active(void);

/* ADD the drone's stereo output into the dry buffers and a send-scaled copy
 * into the reverb-send buffers (float, separate-channel, length frames). */
void drone_render_mix(float *dry_L, float *dry_R,
                      float *send_L, float *send_R, int frames);

#endif
