#include "lib-header/stdtype.h"
#include "lib-header/gdt.h"
#include "lib-header/interrupt.h"

/**
 * global_descriptor_table, predefined GDT.
 * Initial SegmentDescriptor already set properly according to GDT definition in Intel Manual & OSDev.
 * Table entry = [{Null Descriptor}, {Kernel Code}, {Kernel Data (variable, etc)}, ...].
 */
static struct GlobalDescriptorTable global_descriptor_table = {
        .table = {
                {
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
                },
                {
                        // Kernel mode code segment
                        .segment_low =  0xffff,
                        .base_low=      0,
                        .base_mid=      0,
                        .type=          0b1010,
                        .non_system=    0b1,
                        .ring=          0,
                        .present=       1,
                        .segment_high=  0xf,
                        .avl=           0,
                        .mode64=        0,
                        .db_bit=        1,
                        .granularity=   1,
                        .base_high=     0
                },
                {       // Kernel mode data segment
                        .segment_low =  0xffff,
                        .base_low=      0,
                        .base_mid=      0,
                        .type=          0b0010,
                        .non_system=    0b1,
                        .ring=          0,
                        .present=       1,
                        .segment_high=  0xf,
                        .avl=           0,
                        .mode64=        0,
                        .db_bit=        1,
                        .granularity=   1,
                        .base_high=     0
                },
                {       // User mode code segment
                        .segment_low =  0xffff,
                        .base_low=      0,
                        .base_mid=      0,
                        .type=          0b1010,
                        .non_system=    0b1,
                        .ring=          0b11,
                        .present=       1,
                        .segment_high=  0xf,
                        .avl =          0,
                        .mode64 =       0,
                        .db_bit =       1,
                        .granularity =  1,
                        .base_high =    0
                },
                {       // User mode data segment
                        .segment_low=   0xffff,
                        .base_low=      0,
                        .base_mid=      0,
                        .type=          0b0010,
                        .non_system=    0b1,
                        .ring=          0b11,
                        .present=       1,
                        .segment_high=  0xf,
                        .avl=           0,
                        .mode64=        0,
                        .db_bit=        1,
                        .granularity=   1,
                        .base_high=     0
                },
                {
                        .segment_high      = (sizeof(struct TSSEntry) & (0xF << 16)) >> 16,
                        .segment_low       = sizeof(struct TSSEntry),
                        .base_high         = 0,
                        .base_mid          = 0,
                        .base_low          = 0,
                        .non_system        = 0,    // S bit
                        .type              = 0x9,
                        .ring              = 0,    // DPL
                        .present           = 1,    // P bit
                        .db_bit            = 1,    // D/B bit
                        .mode64            = 0,    // L bit
                        .granularity       = 0,    // G bit
                },
                {0}
        }
};

void gdt_install_tss(void) {
    uint32_t base = (uint32_t) &_interrupt_tss_entry;
    global_descriptor_table.table[5].base_high = (base & (0xFF << 24)) >> 24;
    global_descriptor_table.table[5].base_mid  = (base & (0xFF << 16)) >> 16;
    global_descriptor_table.table[5].base_low  = base & 0xFFFF;
}


/**
 * _gdt_gdtr, predefined system GDTR.
 * GDT pointed by this variable is already set to point global_descriptor_table above.
 * From= https=//wiki.osdev.org/Global_Descriptor_Table, GDTR.size is GDT size minus 1.
 */
struct GDTR _gdt_gdtr = {
        sizeof(global_descriptor_table)-1,
        &global_descriptor_table
};