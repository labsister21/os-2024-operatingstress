#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "header/memory/paging.h"

__attribute__((aligned(0x1000))) struct PageDirectory _paging_kernel_page_directory = {
    .table = {
        [0] = {
            .flag.present_bit       = 1,
            .flag.write_bit         = 1,
            .flag.use_pagesize_4_mb = 1,
            .lower_address          = 0,
        },
        [0x300] = {
            .flag.present_bit       = 1,
            .flag.write_bit         = 1,
            .flag.use_pagesize_4_mb = 1,
            .lower_address          = 0,
        },
    }
};

static struct PageManagerState page_manager_state = {
    .page_frame_map = {
        [0]                            = true,
        [1 ... PAGE_FRAME_MAX_COUNT-1] = false
    },
    // TODO: Initialize page manager state properly
    .free_page_frame_count = PAGE_FRAME_MAX_COUNT,
};

void update_page_directory_entry(
    struct PageDirectory *page_dir,
    void *physical_addr, 
    void *virtual_addr, 
    struct PageDirectoryEntryFlag flag
) {
    uint32_t page_index = ((uint32_t) virtual_addr >> 22) & 0x3FF;
    page_dir->table[page_index].flag          = flag;
    page_dir->table[page_index].lower_address = ((uint32_t) physical_addr >> 22) & 0x3FF;
    flush_single_tlb(virtual_addr);
}

void flush_single_tlb(void *virtual_addr) {
    asm volatile("invlpg (%0)" : /* <Empty> */ : "b"(virtual_addr): "memory");
}



/* --- Memory Management --- */
// TODO: Implement
bool paging_allocate_check(uint32_t amount) {
    // TODO: Check whether requested amount is available

    // Hitung jumlah halaman yang diperlukan untuk jumlah memori yang diminta
    uint32_t required_pages = (amount + PAGE_FRAME_SIZE - 1) / PAGE_FRAME_SIZE;

    // Periksa apakah ada cukup banyak halaman kosong
    return page_manager_state.free_page_frame_count >= required_pages;
}


bool paging_allocate_user_page_frame(struct PageDirectory *page_dir, void *virtual_addr) {
    /*
     * TODO: Find free physical frame and map virtual frame into it
     * - Find free physical frame in page_manager_state.page_frame_map[] using any strategies
     * - Mark page_manager_state.page_frame_map[]
     * - Update page directory with user flags:
     *     > present bit    true
     *     > write bit      true
     *     > user bit       true
     *     > pagesize 4 mb  true
     */ 

    // Find a free physical frame
    uint32_t frame_index;
    for (frame_index = 0; frame_index < PAGE_FRAME_MAX_COUNT; ++frame_index) {
        if (!page_manager_state.page_frame_map[frame_index]) {
            // Mark the physical frame as used
            page_manager_state.page_frame_map[frame_index] = true;
            page_manager_state.free_page_frame_count--;

            // Map the virtual frame to the physical frame in the page directory
            uint32_t page_index = ((uint32_t) virtual_addr >> 22) & 0x3FF;
            page_dir->table[page_index].flag.present_bit       = 1;
            page_dir->table[page_index].flag.write_bit         = 1;
            page_dir->table[page_index].flag.use_pagesize_4_mb = 1;
            page_dir->table[page_index].lower_address          = frame_index;

            return true; // Allocation successful
        }
    }

    return false; // No free frame available
}

bool paging_free_user_page_frame(struct PageDirectory *page_dir, void *virtual_addr) {
    /* 
     * TODO: Deallocate a physical frame from respective virtual address
     * - Use the page_dir.table values to check mapped physical frame
     * - Remove the entry by setting it into 0
     */
    
    // Hitung indeks halaman dari alamat virtual
    uint32_t page_index = ((uint32_t) virtual_addr >> 22) & 0x3FF;

    // Dapatkan alamat fisik yang terkait dengan halaman virtual
    void *physical_addr = (void *)((page_dir->table[page_index].lower_address << 22) | ((uint32_t) virtual_addr & 0x3FFFFF));

    // Hitung indeks halaman fisik
    uint32_t frame_index = (uint32_t) physical_addr / PAGE_FRAME_SIZE;

    // Pastikan halaman fisik yang tersedia
    if (page_manager_state.page_frame_map[frame_index]) {
        // Hapus tanda halaman fisik sebagai digunakan
        page_manager_state.page_frame_map[frame_index] = false;
        page_manager_state.free_page_frame_count++;

        // Hapus entri dalam direktori halaman
        page_dir->table[page_index].flag.present_bit = 0;

        return true;
    }

    return false; // Halaman tidak ditemukan
}