/*
 * scenes_flash_h743.c — Scenes-Persistenz im internen STM32H743-Flash.
 *
 * Ablage: LETZTER Sektor von Bank 2 (0x081E0000, 128 KB) — die Firmware
 * wird im Product-Linkaudit vollstaendig in Bank 1 verlangt. Dual-bank
 * read-while-write ist die Architektur fuer weiterhin bedienbare Audio-IRQs,
 * kein gemessener Nachweis. Erase/Program blockiert weiterhin den Main-Loop:
 * UI und Generate-Planung koennen sich verzoegern. Save-Latenz, Interrupt-
 * Reserve und Power-loss-Verhalten bleiben ein Geraet-/Storage-Gate.
 *
 * Layout: das rohe scene_store_t-Blob, auf 32-Byte-Flashwords
 * aufgerundet. Gueltigkeit prueft scenes.c selbst (Magic).
 *
 * BENCH-PENDING wie alles Geraeteseitige: compile-verifiziert (CI-Cross-
 * Build), nie auf Silizium gelaufen.
 */

#include "scenes.h"
#include "stm32h7xx_hal.h"
#include <string.h>

#define SCENES_FLASH_ADDR   0x081E0000u          /* Bank 2, Sektor 7 */
#define SCENES_FLASH_SECTOR FLASH_SECTOR_7
#define FLASHWORD_BYTES     32u

bool scenes_flash_write(const void *blob, unsigned len) {
    /* Auf Flashword-Granularitaet aufrunden, Rest mit 0xFF fuellen. */
    static uint8_t buf[512] __attribute__((aligned(32)));
    if (!blob || len == 0 || len > sizeof buf) return false;
    memset(buf, 0xFF, sizeof buf);
    memcpy(buf, blob, len);
    unsigned words = (len + FLASHWORD_BYTES - 1u) / FLASHWORD_BYTES;

    if (HAL_FLASH_Unlock() != HAL_OK) return false;

    FLASH_EraseInitTypeDef er;
    uint32_t bad_sector = 0;
    er.TypeErase    = FLASH_TYPEERASE_SECTORS;
    er.Banks        = FLASH_BANK_2;
    er.Sector       = SCENES_FLASH_SECTOR;
    er.NbSectors    = 1;
    er.VoltageRange = FLASH_VOLTAGE_RANGE_4;
    if (HAL_FLASHEx_Erase(&er, &bad_sector) != HAL_OK) {
        HAL_FLASH_Lock();
        return false;
    }

    bool ok = true;
    /* Commit the magic-containing first word last. Product SCN7 also has
     * a store CRC; a failed/power-interrupted erase is not a successful Save.
     * Atomic preservation of the preceding flash version needs a journal
     * and remains a storage/device gate, not promised by this single sector. */
    for (unsigned i = 0; i < words && ok; ++i) {
        unsigned w = i + 1 < words ? i + 1 : 0;
        ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD,
                               SCENES_FLASH_ADDR + w * FLASHWORD_BYTES,
                               (uint32_t)(buf + w * FLASHWORD_BYTES)) == HAL_OK;
    }
    HAL_FLASH_Lock();
    return ok;
}

bool scenes_flash_read(void *blob, unsigned len) {
    if(!blob || len == 0 || len > 512) return false;
    /* Bank 2 ist memory-mapped — einfach kopieren. Ob der Inhalt gueltig
     * ist (Magic), entscheidet scenes.c. */
    /* Flash is memory-mapped with the M7 data cache enabled. A read after
     * programming must not reuse a preceding cached Scene. Touch only the
     * aligned reserved Flash range, never the audio DMA buffer. CMSIS defines
     * this helper in vendor/CMSIS/Include/cachel1_armv7.h. */
    SCB_InvalidateDCache_by_Addr((void *)SCENES_FLASH_ADDR,
                               (int32_t)((len+31u)&~31u));
    memcpy(blob, (const void *)SCENES_FLASH_ADDR, len);
    return true;
}
