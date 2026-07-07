#ifndef FAM_PSRAM_H
#define FAM_PSRAM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*
 * psram.h — 8 MB QSPI PSRAM (APS6404L, ADR-0022) on QUADSPI Bank 2.
 *
 * ⚠⚠ BENCH-PENDING — NOT VALIDATED ON HARDWARE. The board has never existed,
 * so this driver has never talked to a real chip. It is a datasheet-grounded
 * starting point (APS6404L Rev 2.1 command set + STM32H743 QUADSPI, register
 * level) + a self-test hook so first bring-up can prove it. Expect to tune the
 * dummy-cycle count and clock prescaler on the scope/debugger (BRING_UP).
 *
 * Memory model (STM32 QUADSPI limitation): memory-mapped mode is READ-ONLY.
 *   - reads  → the CPU reads psram_base()[..] directly (cacheable, MPU-normal),
 *   - writes → psram_write() uses QUADSPI indirect mode.
 * So this is ideal for read-mostly cold data (samples / IRs / wavetables)
 * streamed through the D-cache — exactly the ADR-0022 contract. Hot per-sample
 * buffers stay in internal SRAM.
 */

#define PSRAM_BASE_ADDR   0x90000000u          /* QUADSPI memory-mapped window */
#define PSRAM_SIZE_BYTES  (8u * 1024u * 1024u) /* 64 Mbit = 8 MB              */

/* Best-effort bring-up: RCC + GPIO (PB2/PC11/PE7-10), APS6404L reset + enter
 * QPI, memory-mapped read, MPU region. Returns false if a step reports an
 * error (host build: no-op returning false). Never blocks indefinitely. */
bool psram_init(void);

const uint8_t *psram_base(void);               /* memory-mapped read pointer */
size_t         psram_size(void);

/* Indirect QUADSPI write (memory-mapped mode is read-only). len bytes from
 * src → PSRAM offset `addr`. Returns false on error / not-initialised. */
bool psram_write(uint32_t addr, const void *src, size_t len);

/* First-power-on self-test (BRING_UP stage): write a walking pattern via
 * psram_write(), read it back through the memory-mapped window, compare.
 * Returns true only if every byte matches. Safe to call after psram_init(). */
bool psram_selftest(void);

#endif /* FAM_PSRAM_H */
