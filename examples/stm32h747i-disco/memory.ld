/* memory.ld - Memory regions for STM32H747I-DISCO (CM7).
 *
 * PARITY: rlvgl/examples/stm32h747i-disco/memory.x (v0.2.0 @ d99f793).
 * DELTA:  ld syntax instead of cortex-m-rt's `link.x` overlay; region
 *         origins/lengths are byte-for-byte identical.
 *
 * RM0399 §3 (memory map) is authoritative for every region below.
 * Visibility-only regions (D1_CM4, MAILBOX, D3_CM4, QSPI_FLASH) are
 * declared so a consuming linker script can target them via
 * REGION_ALIAS without re-deriving addresses.
 */

MEMORY
{
  /* Bank 1, CM7-only. RM0399 §3.3.1.                              */
  FLASH      (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
  /* DTCM RAM. RM0399 §3.3.5. Bound to CM7 only.                   */
  RAM        (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
  /* D1 AXI SRAM 512K @ 0x2400_0000; 3/4 to CM7, 1/4 to CM4.        */
  D1_CM7     (rwx) : ORIGIN = 0x24000000, LENGTH = 384K
  D1_CM4     (rwx) : ORIGIN = 0x24060000, LENGTH = 128K
  /* Cross-core mailbox in D2 SRAM3.                                */
  MAILBOX    (rw)  : ORIGIN = 0x30047000, LENGTH = 1K
  /* D3 SRAM4 (CM4 retention).                                      */
  D3_CM4     (rwx) : ORIGIN = 0x38000000, LENGTH = 64K
  /* External QSPI flash (MT25TL01G Bank 1, 64 MB) memory-mapped.  */
  QSPI_FLASH (r)   : ORIGIN = 0x90000000, LENGTH = 64M
  /* External SDRAM via FMC Bank 1 (32 MB on H747I-DISCO).          */
  SDRAM      (rwx) : ORIGIN = 0xC0000000, LENGTH = 32M
}

/* Region helpers used by disco.ld (and downstream consumers). Mirrors
 * the PROVIDE() pattern in rlvgl's memory.x. */
PROVIDE(_qspi_flash_start   = ORIGIN(QSPI_FLASH));
PROVIDE(_qspi_flash_size    = LENGTH(QSPI_FLASH));
PROVIDE(_sdram_start        = ORIGIN(SDRAM));
PROVIDE(_sdram_size         = LENGTH(SDRAM));
PROVIDE(_axisram_cm7_start  = ORIGIN(D1_CM7));
PROVIDE(_axisram_cm7_size   = LENGTH(D1_CM7));
PROVIDE(_mailbox_base       = ORIGIN(MAILBOX));
PROVIDE(_mailbox_size       = LENGTH(MAILBOX));
PROVIDE(_retained_d3_start  = ORIGIN(D3_CM4));
PROVIDE(_retained_d3_size   = LENGTH(D3_CM4));
