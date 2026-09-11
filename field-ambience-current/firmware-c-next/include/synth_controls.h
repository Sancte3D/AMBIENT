#ifndef FAM_SYNTH_CONTROLS_H
#define FAM_SYNTH_CONTROLS_H
/* Shared by the menu and sound host. Percent values invert each core's
 * native parameter mapping; selecting a core recalls that core's settings. */
#define SYNTH_CONTROL_COUNT 6
#define SYNTH_CONTROL_CORES 6
static const unsigned char synth_control_defaults[6][6] = {
    {50,35,35,25,18,30}, /* Dusk: cutoff, resonance, decay, drive, glide, env */
    {27,20,33,45,18,45}, /* FM: index, integer ratio, decay, tone, glide, body */
    {22,40,38,38,16,33}, /* Mist: cutoff, detune, chorus, rate, glide, attack */
    {20,50,45,15, 8,31}, /* Storm: cutoff, detune, PWM, drive, glide, rate */
    {50,20,40,100,15,67},/* Orbit: shape, rate, cutoff, spread, glide, sustain */
    {57,22,24, 8,10,44}  /* Bamboo: pluck, decay, metal, tone, glide, send */
};
static const char *const synth_control_names[6][6] = {
    {"Cutoff", "Resonance", "Decay", "Core Drive", "Glide", "Filter Env"},
    {"FM Index", "FM Ratio", "Index Decay", "Tone", "Glide", "FM Body"},
    {"Cutoff", "Detune", "Chorus", "Chorus Rate", "Glide", "Core Attack"},
    {"Cutoff", "Detune", "PWM Depth", "Core Drive", "Glide", "PWM Rate"},
    {"Wave Shape", "Morph Rate", "Cutoff", "Spread", "Glide", "Sustain"},
    {"Pluck Tone", "LPG Decay", "Metal", "Tone Floor", "Glide", "Send"}
};
#endif
