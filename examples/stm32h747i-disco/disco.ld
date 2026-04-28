/* disco.ld - Section layout for STM32H747I-DISCO (CM7).
 *
 * PLAT-02a §5.4. Mirrors cortex-m-rt's section ordering with C++
 * static-constructor support added. The vector table is anchored
 * first in FLASH so the boot-time MSP load (word 0) and Reset jump
 * (word 1) hit the right targets.
 */

INCLUDE memory.ld

ENTRY(Reset_Handler)

/* Stack grows downward from the top of the DTCM RAM region. */
_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
  /* Vector table — must be first in FLASH.                       */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } > FLASH

  /* Code.                                                         */
  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)
    KEEP (*(.init))
    KEEP (*(.fini))
    . = ALIGN(4);
  } > FLASH

  /* Read-only data.                                               */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } > FLASH

  /* ARM unwind info (kept for fault dumping; trimmed by --gc-sections
   * when unused).                                                 */
  .ARM.extab :
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } > FLASH

  .ARM.exidx :
  {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } > FLASH

  /* C++ pre-init / init / fini arrays (static ctors).             */
  .preinit_array :
  {
    PROVIDE_HIDDEN(__preinit_array_start = .);
    KEEP(*(.preinit_array*))
    PROVIDE_HIDDEN(__preinit_array_end = .);
  } > FLASH

  .init_array :
  {
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array*))
    PROVIDE_HIDDEN(__init_array_end = .);
  } > FLASH

  .fini_array :
  {
    PROVIDE_HIDDEN(__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array*))
    PROVIDE_HIDDEN(__fini_array_end = .);
  } > FLASH

  /* Initialised data: load address in FLASH, virtual address in RAM. */
  __sidata = LOADADDR(.data);
  .data :
  {
    . = ALIGN(4);
    __sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    __edata = .;
  } > RAM AT > FLASH

  /* Zero-initialised data.                                        */
  .bss :
  {
    . = ALIGN(4);
    __sbss = .;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    __ebss = .;
  } > RAM

  /* Heap (newlib-nano sbrk uses __end as the heap base; we expose
   * _end_of_heap so consumers can sanity-check the region).       */
  .heap (NOLOAD) :
  {
    . = ALIGN(8);
    PROVIDE(__end = .);
    PROVIDE(_end_of_heap = ORIGIN(RAM) + LENGTH(RAM) - 0x4000); /* 16K stack */
  } > RAM

  /* Discard .ARM.attributes, .comment, etc.                       */
  /DISCARD/ :
  {
    *(.note.GNU-stack)
    *(.gnu.glibc-stub.*)
  }

  /* Sanity: PROVIDE _flash_image_end so a downstream tool can size
   * the image without re-deriving the symbols.                    */
  PROVIDE(_flash_image_end = __sidata + (__edata - __sdata));
}
