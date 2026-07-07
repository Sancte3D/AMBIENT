#ifndef FAM_DIAG_H
#define FAM_DIAG_H

/* Bring-up diagnostics screen (diag_h743.c). Draws live profiler / PSRAM /
 * battery / voice state to the framebuffer. `psram_ran`/`psram_pass` reflect
 * whether the QSPI-PSRAM self-test has been triggered and its result. */
void diag_draw(int psram_pass, int psram_ran);

#endif /* FAM_DIAG_H */
