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
 * Default: rohes scene_store_t-Blob, auf 32-Byte-Flashwords aufgerundet.
 * Der explizite FAM_SCENE_JOURNAL_CANDIDATE reserviert Sektoren 6 und 7;
 * ECC-sichere Reads und Geraeteabnahme stehen noch aus.
 * Gueltigkeit des Scene-Wireformats prueft scenes.c selbst.
 *
 * BENCH-PENDING wie alles Geraeteseitige: compile-verifiziert (CI-Cross-
 * Build), nie auf Silizium gelaufen.
 */

#include "scenes.h"
#include "stm32h7xx_hal.h"
#include "h743_hal.h"   /* h743_watchdog_feed() */
#include <string.h>

#ifdef FAM_SCENE_JOURNAL_CANDIDATE
#include "scenes_journal.h"

/* Explicit experimental build only. The memory-mapped reader below does not
 * recover Cortex-M7 faults from an ECC-damaged flashword. A validated safe
 * read adapter and power-cut/voltage/latency acceptance are required before
 * this option can become the product default. No audio-IRQ call sites. */
#define JOURNAL_SECTOR_BYTES (128u*1024u)
#define JOURNAL_BASE 0x081C0000u /* Bank 2, sectors 6 and 7 */
static scene_journal_t s_journal;
static bool s_configured;

static uint32_t journal_address(unsigned sector,unsigned offset) {
    return JOURNAL_BASE+sector*JOURNAL_SECTOR_BYTES+offset;
}
static scene_journal_read_result_t journal_read(void *context,unsigned sector,unsigned offset,
                                               void *out,unsigned length) {
    (void)context;
    if (sector>=2 || offset>JOURNAL_SECTOR_BYTES || length>JOURNAL_SECTOR_BYTES-offset)
        return SCENE_JOURNAL_READ_ERROR;
    uint32_t address=journal_address(sector,offset);
    uint32_t aligned=address&~31u;
    unsigned bytes=((address-aligned)+length+31u)&~31u;
    SCB_InvalidateDCache_by_Addr((void *)aligned,(int32_t)bytes);
    /* TODO(device gate): safely classify an ECC-corrupt read as READ_CORRUPT;
     * ordinary memcpy cannot establish recovery from torn H743 flashwords. */
    memcpy(out,(const void *)address,length);
    return SCENE_JOURNAL_READ_OK;
}
static bool journal_erase(void *context,unsigned sector) {
    (void)context;
    if (sector>=2 || !h743_supply_ok()) return false;   /* below 2.7 V: no x64 erase */
    FLASH_EraseInitTypeDef erase={0}; uint32_t bad_sector=0;
    erase.TypeErase=FLASH_TYPEERASE_SECTORS;
    erase.Banks=FLASH_BANK_2;
    erase.Sector=FLASH_SECTOR_6+sector;
    erase.NbSectors=1;
    erase.VoltageRange=FLASH_VOLTAGE_RANGE_4;
    return HAL_FLASHEx_Erase(&erase,&bad_sector)==HAL_OK;
}
static bool journal_program(void *context,unsigned sector,unsigned offset,const uint8_t *word) {
    (void)context;
    if (sector>=2 || offset%32u || offset+32u>JOURNAL_SECTOR_BYTES || !h743_supply_ok()) return false;
    return HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD,journal_address(sector,offset),
                             (uint32_t)word)==HAL_OK;
}
static void journal_configure(void) {
    if (s_configured) return;
    scene_journal_io_t io={NULL,JOURNAL_SECTOR_BYTES,journal_read,journal_erase,journal_program};
    scene_journal_configure(&s_journal,&io);
    s_configured=true;
}
bool scenes_flash_write(const void *blob,unsigned length) {
    if (!blob || length==0 || length>SCENE_JOURNAL_PAYLOAD_BYTES) return false;
    journal_configure();
    if (HAL_FLASH_Unlock()!=HAL_OK) return false;
    bool ok=scene_journal_write(&s_journal,blob,length);
    HAL_FLASH_Lock();
    return ok;
}
bool scenes_flash_read(void *blob,unsigned length) {
    if (!blob || length==0 || length>SCENE_JOURNAL_PAYLOAD_BYTES) return false;
    journal_configure();
    if (!s_journal.initialized && !scene_journal_recover(&s_journal)) return false;
    if (s_journal.has_record) return scene_journal_read(&s_journal,blob,length);
    /* Keep old raw SCN5/6/7 bytes at sector 7 until the first journal commit.
     * Once a valid journal exists, never resurrect the old raw store because
     * a request uses a different payload length. scenes.c validates its wire. */
    return journal_read(NULL,1,0,blob,length)==SCENE_JOURNAL_READ_OK;
}
#else

#define SCENES_FLASH_ADDR   0x081E0000u          /* Bank 2, Sektor 7 */
#define SCENES_FLASH_SECTOR FLASH_SECTOR_7
#define FLASHWORD_BYTES     32u

/* A Save cut by pulling the power (the device has no battery) can leave a
 * half-programmed Flash word. Reading it raises a double-ECC bus error, a
 * fault at every boot, so the device would restart forever. The read below
 * sets an uncached marker first and clears it after; if a boot finds the
 * marker together with a software or watchdog reset, the previous boot died
 * inside that read: erase the sector and start with defaults. */
#define SCENE_READ_MAGIC 0x5CE4EADu
__attribute__((section(".noinit"))) static volatile uint32_t s_scene_read_marker[2];

static void scene_marker_set(bool on) {
    s_scene_read_marker[0] = on ? SCENE_READ_MAGIC : 0u;
    s_scene_read_marker[1] = on ? ~SCENE_READ_MAGIC : 0u;
    __DSB();
}

static bool scenes_flash_erase(void) {
    if (!h743_supply_ok()) return false;      /* below 2.7 V: no x64 erase */
    if (HAL_FLASH_Unlock() != HAL_OK) return false;
    FLASH_EraseInitTypeDef er;
    uint32_t bad_sector = 0;
    er.TypeErase    = FLASH_TYPEERASE_SECTORS;
    er.Banks        = FLASH_BANK_2;
    er.Sector       = SCENES_FLASH_SECTOR;
    er.NbSectors    = 1;
    er.VoltageRange = FLASH_VOLTAGE_RANGE_4;
    bool ok = HAL_FLASHEx_Erase(&er, &bad_sector) == HAL_OK;
    HAL_FLASH_Lock();
    SCB_InvalidateDCache_by_Addr((void *)SCENES_FLASH_ADDR, 512);
    return ok;
}

/* Call once at boot, before scenes_init(). Returns true if it had to erase. */
bool scenes_flash_boot_recover(void) {
    const uint32_t rsr = RCC->RSR;
    const bool died_reading = s_scene_read_marker[0] == SCENE_READ_MAGIC &&
                              s_scene_read_marker[1] == ~SCENE_READ_MAGIC &&
                              (rsr & (RCC_RSR_SFTRSTF | RCC_RSR_IWDG1RSTF)) != 0u;
    scene_marker_set(false);
    RCC->RSR |= RCC_RSR_RMVF;                 /* next boot sees a fresh cause */
    if (!died_reading) return false;
    (void)scenes_flash_erase();
    return true;
}

bool scenes_flash_write(const void *blob, unsigned len) {
    /* Auf Flashword-Granularitaet aufrunden, Rest mit 0xFF fuellen. */
    static uint8_t buf[512] __attribute__((aligned(32)));
    if (!blob || len == 0 || len > sizeof buf) return false;
    if (!h743_supply_ok()) return false;      /* below 2.7 V: no x64 erase/program */
    memset(buf, 0xFF, sizeof buf);
    memcpy(buf, blob, len);
    unsigned words = (len + FLASHWORD_BYTES - 1u) / FLASHWORD_BYTES;

    if (HAL_FLASH_Unlock() != HAL_OK) return false;
    h743_watchdog_feed();                     /* the erase blocks for seconds */

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
    h743_watchdog_feed();
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
    scene_marker_set(true);                   /* a double-ECC fault ends here */
    memcpy(blob, (const void *)SCENES_FLASH_ADDR, len);
    scene_marker_set(false);
    return true;
}
#endif
