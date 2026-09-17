#ifndef FAM_PITCH_MODES_H
#define FAM_PITCH_MODES_H
#include <stdint.h>
/* 6 church modes, semitone intervals over the octave. */
static const int8_t PITCH_MODES[6][7] = {
    { 0, 2, 4, 5, 7, 9, 11 },   /* ionian     */
    { 0, 2, 3, 5, 7, 9, 10 },   /* dorian     */
    { 0, 1, 3, 5, 7, 8, 10 },   /* phrygian   */
    { 0, 2, 4, 6, 7, 9, 11 },   /* lydian     */
    { 0, 2, 4, 5, 7, 9, 10 },   /* mixolydian */
    { 0, 2, 3, 5, 7, 8, 10 },   /* aeolian    */
};

#endif
