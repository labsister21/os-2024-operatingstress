#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "header/stdlib/string.h"
#include "header/filesystem/fat32.h"
#include "header/driver/disk.h"

static struct FAT32DriverState fat32_driver_state;

const uint8_t fs_signature[BLOCK_SIZE] = {
    'C',
    'o',
    'u',
    'r',
    's',
    'e',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    'D',
    'e',
    's',
    'i',
    'g',
    'n',
    'e',
    'd',
    ' ',
    'b',
    'y',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    'L',
    'a',
    'b',
    ' ',
    'S',
    'i',
    's',
    't',
    'e',
    'r',
    ' ',
    'I',
    'T',
    'B',
    ' ',
    ' ',
    'M',
    'a',
    'd',
    'e',
    ' ',
    'w',
    'i',
    't',
    'h',
    ' ',
    '<',
    '3',
    ' ',
    ' ',
    ' ',
    ' ',
    '-',
    '-',
    '-',
    '-',
    '-',
    '-',
    '-',
    '-',
    '-',
    '-',
    '-',
    '2',
    '0',
    '2',
    '4',
    '\n',
    [BLOCK_SIZE - 2] = 'O',
    [BLOCK_SIZE - 1] = 'k',
};

void initialize_filesystem_fat32(void)
{
    if (is_empty_storage())
    {
        create_fat32();
    }
    else
    {
        read_clusters(&fat32_driver_state, 1, 1);
    }
}

bool is_empty_storage(void)
{
    uint8_t buffer[BLOCK_SIZE];
    read_blocks(buffer, 0, 1);
    return memcmp(buffer, fs_signature, BLOCK_SIZE) != 0;
}

void create_fat32(void)
{
    uint8_t boot_sector[BLOCK_SIZE];
    memset(boot_sector, 0, BLOCK_SIZE);
    memcpy(boot_sector, fs_signature, BLOCK_SIZE);
    write_blocks(boot_sector, 0, 1);

    fat32_driver_state.fat_table.cluster_map[0] = CLUSTER_0_VALUE;
    fat32_driver_state.fat_table.cluster_map[1] = CLUSTER_1_VALUE;
    fat32_driver_state.fat_table.cluster_map[2] = FAT32_FAT_END_OF_FILE;

    write_clusters(&fat32_driver_state.fat_table.cluster_map, 1, 1);

    struct FAT32DirectoryTable root_dir_table = {0};
    init_directory_table(&root_dir_table, "ROOT\0\0\0", 2);
    write_clusters(&root_dir_table, 2, 1);
}

void read_clusters(void *ptr, uint32_t cluster_number, uint8_t cluster_count)
{
    read_blocks(ptr, cluster_number * 4, cluster_count * CLUSTER_BLOCK_COUNT);
}

void write_clusters(const void *ptr, uint32_t cluster_number, uint8_t cluster_count)
{
    write_blocks(ptr, cluster_number * 4, cluster_count * CLUSTER_BLOCK_COUNT);
}

void init_directory_table(struct FAT32DirectoryTable *dir_table, char *name, uint32_t parent_dir_cluster)
{
    // Baru tulis di ram tapi belum dicluster
    dir_table->table[0].cluster_high = (uint16_t)(parent_dir_cluster >> 16);
    dir_table->table[0].cluster_low = (uint16_t)(parent_dir_cluster & 0xFFFF);

    // baca karakter nama table
    for (int i = 0; i < 8; i++)
    {
        dir_table->table[0].name[i] = name[i];
    }

    // masukin data ke dir_table
    dir_table->table[0].attribute = ATTR_SUBDIRECTORY;
    dir_table->table[0].user_attribute = UATTR_NOT_EMPTY;
}

uint32_t divceil(uint32_t pembilang, uint32_t penyebut)
{
    uint32_t cmp = pembilang / penyebut;

    if (pembilang % penyebut == 0)
        return cmp;

    return cmp + 1;
}

bool is_dir_table_valid(void)
{
    return fat32_driver_state.dir_table_buf.table[0].user_attribute == UATTR_NOT_EMPTY;
}

int8_t read(struct FAT32DriverRequest request)
{
    // Baca terlebih dahulu directory table dari parent cluster
    read_clusters(&fat32_driver_state.dir_table_buf, request.parent_cluster_number, 1);

    // Referensi ke directory table
    struct FAT32DirectoryEntry *table = fat32_driver_state.dir_table_buf.table;

    // Loop melalui directory entries
    for (uint32_t i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++)
    {
        // Periksa apakah nama dan ekstensi cocok
        if (!memcmp(table[i].name, request.name, 8) && !memcmp(table[i].ext, request.ext, 3))
        {
            // Periksa apakah entry adalah direktori
            if (table[i].attribute & ATTR_SUBDIRECTORY)
            {
                return 1; // Entry adalah direktori, bukan file
            }

            // Periksa apakah buffer cukup untuk menampung file
            if (request.buffer_size < table[i].filesize)
            {
                return -1; // Buffer tidak cukup
            }

            // Hitung jumlah cluster yang diperlukan
            uint16_t cluster_count = divceil(table[i].filesize, CLUSTER_SIZE);
            uint16_t cluster = table[i].cluster_low | (table[i].cluster_high << 16);

            // Baca file dari setiap cluster
            for (uint16_t j = 0; j < cluster_count; j++)
            {
                read_clusters(request.buf + j * CLUSTER_SIZE, cluster, 1);
                cluster = fat32_driver_state.fat_table.cluster_map[cluster];

                // Periksa apakah mencapai end of cluster chain
                if (cluster >= 0xFFF8)
                {
                    break;
                }
            }

            return 0; // Operasi berhasil
        }
    }

    return 2; // File tidak ditemukan
}

int8_t read_directory(struct FAT32DriverRequest request)
{
    // Cek validitas parent folder
    if (fat32_driver_state.fat_table.cluster_map[request.parent_cluster_number] == FAT32_FAT_EMPTY_ENTRY)
    {
        return 2; // Parent folder tidak valid
    }

    // Baca directory table dari parent cluster
    read_clusters(&fat32_driver_state.dir_table_buf, request.parent_cluster_number, 1);
    struct FAT32DirectoryEntry *table = fat32_driver_state.dir_table_buf.table;

    // Loop melalui directory entries
    for (unsigned int i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++)
    {
        if (table[i].user_attribute == UATTR_NOT_EMPTY)
        {
            if (!memcmp(table[i].name, request.name, 8))
            {
                if (table[i].attribute == ATTR_SUBDIRECTORY)
                {
                    // Kalo ketemu direktorinya
                    read_clusters(request.buf, table[i].cluster_low, 1);
                    return 0; // Operasi berhasil
                }
                else
                {
                    // Entry ditemukan tapi bukan folder
                    return 1; // Bukan sebuah folder
                }
            }
        }
    }

    // Kalo ga nemu
    return 2; // Folder tidak ditemukan pada parent folder
}

int8_t write(struct FAT32DriverRequest request)
{
    read_clusters(&fat32_driver_state.dir_table_buf, request.parent_cluster_number, 1);
    read_clusters(&fat32_driver_state.fat_table, 1, 1);

    // Cek validitas parent folder
    if (fat32_driver_state.fat_table.cluster_map[request.parent_cluster_number] == FAT32_FAT_EMPTY_ENTRY)
    {
        return 2; // Parent folder tidak valid
    }

    uint16_t cluster = 0;
    uint16_t next_cluster = 0;
    uint16_t cluster_table = 0;

    while (fat32_driver_state.fat_table.cluster_map[cluster] != 0)
    {
        cluster++;
    }

    // cek nama yang sama
    for (int i = 0; i < 64; i++)
    {
        if (memcmp(fat32_driver_state.dir_table_buf.table[i].name, request.name, 8) == 0)
        {
            if (request.buffer_size != 0 && memcmp(fat32_driver_state.dir_table_buf.table[i].ext, request.ext, 3) == 0)
                return 1;
        }
    }

    // cari cluster kosong dari fat32 table
    while (fat32_driver_state.dir_table_buf.table[cluster_table].user_attribute == UATTR_NOT_EMPTY)
    {
        cluster_table++;
    }

    if (request.buffer_size == 0)
    {

        fat32_driver_state.fat_table.cluster_map[cluster] = FAT32_FAT_END_OF_FILE;
        write_clusters(fat32_driver_state.fat_table.cluster_map, FAT_CLUSTER_NUMBER, 1);
        struct FAT32DirectoryTable dir_baru = {0};
        init_directory_table(&dir_baru, request.name, cluster);
        write_clusters(dir_baru.table, cluster, 1);

        fat32_driver_state.dir_table_buf.table[cluster_table] = dir_baru.table[0];

        write_clusters(fat32_driver_state.dir_table_buf.table, request.parent_cluster_number, 1);
    }
    else
    {
        uint32_t total_cluster = request.buffer_size / CLUSTER_SIZE;
        if (total_cluster * CLUSTER_SIZE < request.buffer_size)
        {
            total_cluster++;
        }

        memcpy(fat32_driver_state.dir_table_buf.table[cluster_table].name, request.name, 8);
        memcpy(fat32_driver_state.dir_table_buf.table[cluster_table].ext, request.ext, 3);
        fat32_driver_state.dir_table_buf.table[cluster_table].attribute = 0;
        fat32_driver_state.dir_table_buf.table[cluster_table].user_attribute = UATTR_NOT_EMPTY;
        fat32_driver_state.dir_table_buf.table[cluster_table].cluster_low = cluster;

        for (uint32_t i = 0; i < total_cluster; i++)
        {
            cluster = 0;
            while (fat32_driver_state.fat_table.cluster_map[cluster] != 0)
            {
                cluster++;
            }
            next_cluster = cluster + 1;
            while (fat32_driver_state.fat_table.cluster_map[next_cluster] != 0)
            {
                next_cluster++;
            }

            if (i < total_cluster - 1)
            {
                fat32_driver_state.fat_table.cluster_map[cluster] = next_cluster;
                fat32_driver_state.dir_table_buf.table[cluster_table].filesize += CLUSTER_MAP_SIZE * CLUSTER_BLOCK_COUNT;
                write_clusters(request.buf + CLUSTER_SIZE * i, cluster, 1);
            }
            else
            {
                fat32_driver_state.fat_table.cluster_map[cluster] = FAT32_FAT_END_OF_FILE;
                fat32_driver_state.dir_table_buf.table[cluster_table].filesize += request.buffer_size - CLUSTER_SIZE * i;
                uint8_t sisaBuf[CLUSTER_SIZE];
                memcpy(sisaBuf, request.buf + CLUSTER_SIZE * i, request.buffer_size - (CLUSTER_SIZE * i));

                // isi bagian yang ga kepake dari sisa jadi 0
                for (int j = request.buffer_size - (CLUSTER_SIZE * i); j < CLUSTER_SIZE; j++)
                {
                    sisaBuf[j] = 0;
                }

                write_clusters(sisaBuf, cluster, 1);
            }

            write_clusters(fat32_driver_state.fat_table.cluster_map, FAT_CLUSTER_NUMBER, 1);
        }

        write_clusters(fat32_driver_state.dir_table_buf.table, request.parent_cluster_number, 1);
    }

    return 0;
}

int8_t delete(struct FAT32DriverRequest request)
{
    // Cuman file dan direktori kosong yang bisa diapus
    char *filename = request.name;

    read_clusters(&fat32_driver_state.dir_table_buf, request.parent_cluster_number, 1);
    struct FAT32DirectoryEntry *table = fat32_driver_state.dir_table_buf.table;

    for (uint8_t i = 1; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++)
    {
        if (table[i].user_attribute != UATTR_NOT_EMPTY)
            continue;
        if (memcmp(filename, table[i].name, strlen(table[i].name)) == 0)
        {
            // namanya ketemu, cek apakah sebuah file atau direktori
            if (table[i].attribute)
            {
                // kalo direktori
                //      cek apakah direktori kosong
                struct FAT32DirectoryTable dt;
                read_clusters(&dt, table[i].cluster_low, 1);
                for (unsigned int i = 1; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++)
                {
                    if (dt.table[i].user_attribute == UATTR_NOT_EMPTY)
                    {
                        // kalo ga kosong
                        return 2;
                    }
                }
                // kalo kosong
                fat32_driver_state.fat_table.cluster_map[table[i].cluster_low] = 0;
                table[i].user_attribute = !UATTR_NOT_EMPTY;
                table[i].undelete = true;

                // sync parent directory table
                write_clusters(&fat32_driver_state.dir_table_buf, request.parent_cluster_number, 1);

                // sync fat
                write_clusters(&fat32_driver_state.fat_table, 1, 1);
                return 0;
            }
            else
            {
                // Kalo dia sebuah file
                //      cek .ext nya sama apa ngga
                if (memcmp(table[i].ext, request.ext, 3) == 0)
                {
                    // kalo sama ya hapus
                    table[i].user_attribute = !UATTR_NOT_EMPTY;
                    table[i].undelete = true;
                    write_clusters(&fat32_driver_state.dir_table_buf, request.parent_cluster_number, 1);
                    uint16_t cluster_count = divceil(table[i].filesize, CLUSTER_SIZE);
                    uint16_t to_zeros[cluster_count];
                    uint16_t current_cluster = table[i].cluster_low;

                    for (uint16_t i = 0; i < cluster_count; i++)
                    {
                        to_zeros[i] = current_cluster;
                        current_cluster = fat32_driver_state.fat_table.cluster_map[current_cluster];
                    }

                    for (uint16_t i = 0; i < cluster_count; i++)
                    {
                        fat32_driver_state.fat_table.cluster_map[to_zeros[i]] = 0;
                    }

                    // sync fat
                    write_clusters(&fat32_driver_state.fat_table, 1, 1);
                    return 0;
                }
                else
                {
                    // kalo beda ya cek yang lain
                    continue;
                }
            }
        }
    }

    // kalo ga ketemu namanya
    return 1;
}