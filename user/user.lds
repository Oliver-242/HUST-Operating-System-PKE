OUTPUT_ARCH( "riscv" )

ENTRY(main)

SECTIONS
{
  . = 0x81000000;
  . = ALIGN(0x1000);
  .text : { *(.text) }
  . = ALIGN(16);
  .data : { *(.data) }
  . = ALIGN(16);
  .bss : { *(.bss) }
}
