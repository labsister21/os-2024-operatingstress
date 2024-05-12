// #include "lib-header/stdtype.h"
#include "header/cpu/gdt.h"
#include "header/interrupt/interrupt.h"

/**
 * global_descriptor_table, predefined GDT.
 * Initial SegmentDescriptor already set properly according to GDT definition in Intel Manual & OSDev.
 * Table entry : [{Null Descriptor}, {Kernel Code}, {Kernel Data (variable, etc)}, ...].
 */
static struct GlobalDescriptorTable global_descriptor_table = {
    .table = {
        {// NULL DESCRIPTOR
         .segment_low = 0,
         .base_low = 0,
         .base_mid = 0,
         .type_bit = 0,
         .non_system = 0,
         .DPL = 0,
         .present = 0,
         .limit = 0,
         .available = 0,
         .long_mode = 0,
         .default_size = 0,
         .granularity = 0,
         .base_high = 0},
        {// KERNEL CODE
         .segment_low = 0xFFFF,
         .base_low = 0,
         .base_mid = 0,
         .type_bit = 0b1010,
         .non_system = 1,
         .DPL = 0,
         .present = 1,
         .limit = 0b1111,
         .available = 0,
         .long_mode = 1,
         .default_size = 1,
         .granularity = 1,
         .base_high = 0},
        {// KERNEL DATA
         .segment_low = 0xFFFF,
         .base_low = 0,
         .base_mid = 0,
         .type_bit = 0b0010,
         .non_system = 1,
         .DPL = 0,
         .present = 1,
         .limit = 0b1111,
         .available = 0,
         .long_mode = 0,
         .default_size = 1,
         .granularity = 1,
         .base_high = 0},
        {// USER MODE CODE
         .segment_low = 0xFFFF,
         .base_low = 0,
         .base_mid = 0,
         .type_bit = 0b1010,
         .non_system = 1,
         .DPL = 3,
         .present = 1,
         .limit = 0b1111,
         .available = 1,
         .long_mode = 1,
         .default_size = 1,
         .granularity = 1,
         .base_high = 0},
        {// USER MODE DATA
         .segment_low = 0xFFFF,
         .base_low = 0,
         .base_mid = 0,
         .type_bit = 0b0010,
         .non_system = 1,
         .DPL = 3,
         .present = 1,
         .limit = 0b1111,
         .available = 0,
         .long_mode = 0,
         .default_size = 1,
         .granularity = 1,
         .base_high = 0},
        {
            .limit = (sizeof(struct TSSEntry) & (0xF << 16)) >> 16,
            .segment_low = sizeof(struct TSSEntry),
            .base_high = 0,
            .base_mid = 0,
            .base_low = 0,
            .non_system = 0, // S bit
            .type_bit = 0x9,
            .DPL = 0,          // DPL
            .present = 1,      // present bit
            .default_size = 1, // D/B bit
            .long_mode = 0,    // long_mode bit
            .granularity = 0,  // granularity bit
        },
        {0}}};

/**
 * _gdt_gdtr, predefined system GDTR.
 * GDT pointed by this variable is already set properly.
 * From: https://wiki.osdev.org/Global_Descriptor_Table, GDTR.size is GDT size minus 1.
 */
struct GDTR _gdt_gdtr = {
    sizeof(global_descriptor_table) - 1,
    &global_descriptor_table};

void gdt_install_tss(void)
{
    uint32_t base = (uint32_t)&_interrupt_tss_entry;
    global_descriptor_table.table[5].base_high = (base & (0xFF << 24)) >> 24;
    global_descriptor_table.table[5].base_mid = (base & (0xFF << 16)) >> 16;
    global_descriptor_table.table[5].base_low = base & 0xFFFF;
}