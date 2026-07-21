/*
 * scenes_flash_h743.c — Scenes-Persistenz im internen STM32H743-Flash.
 *
 * Ablage: LETZTER Sektor von Bank 2 (0x081E0000, 128 KB) — die Firmware
 * (~203 KB) liegt komplett in Bank 1 (Sektoren 0..1). H7 ist dual-bank
 * read-while-write: Erase/Program in Bank 2 blockiert Code- und Daten-
 * zugriffe in Bank 1 NICHT — der Audio-ISR laeuft beim Speichern einer
 * Scene ununterbrochen weiter. Der Erase (~1-2 s) blockiert nur den
 * Main-Loop (UI friert kurz ein — akzeptiert, Scene-Save ist selten).
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
    if (len > sizeof buf) return false;
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
    for (unsigned w = 0; w < words && ok; ++w) {
        ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD,
                               SCENES_FLASH_ADDR + w * FLASHWORD_BYTES,
                               (uint32_t)(buf + w * FLASHWORD_BYTES)) == HAL_OK;
    }
    HAL_FLASH_Lock();
    return ok;
}

bool scenes_flash_read(void *blob, unsigned len) {
    /* Bank 2 ist memory-mapped — einfach kopieren. Ob der Inhalt gueltig
     * ist (Magic), entscheidet scenes.c. */
    memcpy(blob, (const void *)SCENES_FLASH_ADDR, len);
    return true;
}
