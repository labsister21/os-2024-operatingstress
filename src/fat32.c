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
    dir_table->table[0].cluster_high = 0;
    dir_table->table[0].cluster_low = 2;
}

/* CRUD Helper */
double ceil(double x)
{
    int intPart = (int)x;
    return (x > intPart) ? (double)(intPart + 1) : (double)intPart;
}

bool is_dir_table_valid(void)
{
    return fat32_driver_state.dir_table_buf.table[0].user_attribute == UATTR_NOT_EMPTY;
}

int8_t read(struct FAT32DriverRequest request)
{
    // baca dulu DirectoryTable
    read_clusters(&fat32_driver_state.dir_table_buf, request.parent_cluster_number, 1);

    int idx = -1;
    for (uint32_t i = 0; i < 64; i++)
    {
        if (memcmp(fat32_driver_state.dir_table_buf.table[i].name, request.name, 8) == 0 && memcmp(fat32_driver_state.dir_table_buf.table[i].ext, request.ext, 3) == 0)
        {
            idx = i;
            break;
        }
    }

    // Kondisi Not Found
    if (idx == -1)
    {
        return 2;
    }
    // Kondisi ketemu subdirektori
    else if (fat32_driver_state.dir_table_buf.table[idx].attribute == 1 || request.buffer_size == 0)
    {
        return 1;
    }

    // Kondisi Valid
    else
    {
        uint32_t cluster_count = fat32_driver_state.dir_table_buf.table[idx].filesize / CLUSTER_SIZE;
        if (cluster_count * CLUSTER_SIZE < fat32_driver_state.dir_table_buf.table[idx].filesize)
        {
            cluster_count++;
        }
        uint32_t cluster_number = (fat32_driver_state.dir_table_buf.table[idx].cluster_high << 16) | fat32_driver_state.dir_table_buf.table[idx].cluster_low;
        for (uint32_t j = 0; j < cluster_count; j++)
        {
            if (j == 0)
            {
                read_clusters(request.buf, cluster_number, 1);
                cluster_number = fat32_driver_state.fat_table.cluster_map[cluster_number];
            }
            else
            {
                read_clusters(request.buf + (j * CLUSTER_SIZE), cluster_number, 1);
                cluster_number = fat32_driver_state.fat_table.cluster_map[cluster_number];
            }
        }
        return 0;
    }

    // Kondisi ngaco
    return -1;
}

int8_t read_directory(struct FAT32DriverRequest request)
{
    read_clusters(&fat32_driver_state.dir_table_buf, request.parent_cluster_number, 1);
    int idx = -1;
    for (uint32_t i = 0; i < 64; i++)
    {
        if (memcmp(fat32_driver_state.dir_table_buf.table[i].name, request.name, 8) == 0)
        {
            idx = i;
            break;
        }
    }
    // Kondisi Not Found
    if (idx == -1)
    {
        return 2;
    }

    // Kondisi bukan folder
    else if (fat32_driver_state.dir_table_buf.table[idx].attribute == 1)
    {
        return 1;
    }
    else
    {
        uint32_t entry = fat32_driver_state.dir_table_buf.table[idx].cluster_low | fat32_driver_state.dir_table_buf.table[idx].cluster_high << 16;
        read_clusters(request.buf, entry, 1);
        return 0;
    }
    return -1;
}

int8_t write(struct FAT32DriverRequest request)
{
    // Baca directory di parent_cluster_number
    read_clusters(&fat32_driver_state.dir_table_buf, request.parent_cluster_number, 1);

    // Mencari first empty di FileAllocationTable
    int index = 0;
    while (fat32_driver_state.fat_table.cluster_map[index] != 0)
    {
        index++;
    }
    if (is_dir_table_valid())
    {
        // Cek apakah request.name dan request.ext ada atau belum
        for (uint32_t i = 0; i < 64; i++)
        {
            if (memcmp(fat32_driver_state.dir_table_buf.table[i].name, request.name, 8) == 0 && memcmp(fat32_driver_state.dir_table_buf.table[i].ext, request.ext, 3) == 0)
            {
                return 1;
            }
        }

        if (request.buffer_size == 0)
        {
            // membuat sub-direktori pada folder parent request.parent_cluster_number dengan nama request.name.
            struct FAT32DirectoryEntry new_entry = {
                .attribute = ATTR_SUBDIRECTORY,
                .user_attribute = UATTR_NOT_EMPTY,
                .filesize = 0,
                .cluster_high = (index >> 16) & 0xFFFF,
                .cluster_low = index & 0xFFFF,
            };

            for (int i = 0; i < 8; i++)
            {
                new_entry.name[i] = request.name[i];
            }

            // cari tempat buat direktori baru
            int dirindex = 0;
            while (fat32_driver_state.dir_table_buf.table[dirindex].user_attribute == UATTR_NOT_EMPTY)
            {
                dirindex++;
            }
            // buat cluster baru untuk membuat suatu direktori
            fat32_driver_state.dir_table_buf.table[dirindex] = new_entry;

            // buat suatu dir table baru di suatu cluster
            struct FAT32DirectoryTable temp =
                {
                    .table =
                        {
                            {
                                .attribute = ATTR_SUBDIRECTORY,
                                .user_attribute = UATTR_NOT_EMPTY,
                                .filesize = request.buffer_size,
                                .cluster_high = request.parent_cluster_number >> 16,
                                .cluster_low = request.parent_cluster_number & 0xFFFF,
                            }}};
            for (int i = 0; i < 8; i++)
            {
                temp.table[0].name[i] = request.name[i];
            }
            fat32_driver_state.fat_table.cluster_map[index] = FAT32_FAT_END_OF_FILE;
            write_clusters(&temp, index, 1);
            write_clusters(&fat32_driver_state.dir_table_buf, request.parent_cluster_number, 1);
            write_clusters(&fat32_driver_state.fat_table, 1, 1);
            return 0;
        }
        else
        {
            // YANG BERHUBUNGAN DENGAN DIRECTORY TABLE
            //  cari yang kosong pertama
            int dirindex = 0;
            while (fat32_driver_state.dir_table_buf.table[dirindex].user_attribute == UATTR_NOT_EMPTY)
            {
                dirindex++;
            }
            int tempindex = index;
            int nextIndex = index;
            for (int i = 0; i < ceil(request.buffer_size / CLUSTER_SIZE); i++)
            {
                // YANG BERHUBUNGAN DENGAN FAT TABLE
                //  cari yang kosong pertama
                while (fat32_driver_state.fat_table.cluster_map[tempindex] != 0)
                {
                    tempindex++;
                }
                // kalo udah ketemu, masukin ke dalam directory tablenya
                if (i == 0)
                {
                    // buat entry baru
                    struct FAT32DirectoryEntry new_entry = {
                        .user_attribute = UATTR_NOT_EMPTY,
                        .filesize = request.buffer_size,
                        .cluster_high = (tempindex >> 16) & 0xFFFF,
                        .cluster_low = tempindex & 0xFFFF,
                    };

                    for (int i = 0; i < 8; i++)
                    {
                        new_entry.name[i] = request.name[i];
                    }
                    for (int i = 0; i < 3; i++)
                    {
                        new_entry.ext[i] = request.ext[i];
                    }
                    fat32_driver_state.dir_table_buf.table[dirindex] = new_entry;
                    write_clusters(&fat32_driver_state.dir_table_buf, request.parent_cluster_number, 1);
                }
                nextIndex = tempindex + 1;
                // cari yang kosong kedua
                while (fat32_driver_state.fat_table.cluster_map[nextIndex] != 0)
                {
                    nextIndex++;
                }
                // buat entry baru
                if (i == ceil(request.buffer_size / CLUSTER_SIZE - 1))
                {
                    fat32_driver_state.fat_table.cluster_map[tempindex] = FAT32_FAT_END_OF_FILE;
                }
                else
                {
                    fat32_driver_state.fat_table.cluster_map[tempindex] = nextIndex;
                }
                write_clusters(request.buf + i * CLUSTER_SIZE, tempindex, 1);
                write_clusters(&fat32_driver_state.fat_table, 1, 1);
            }
            return 0;
        }
    }
    else
    {
        return 2;
    }
    return -1;
}

int8_t delete(struct FAT32DriverRequest request)
{
    if (request.parent_cluster_number == 0)
    {
        return 2;
    }
    else
    {
        // mencari file (request.name, request.ext, dan request.parent_cluster_number menunjuk kepada DirectoryEntry file yang valid )
        read_clusters(&fat32_driver_state.dir_table_buf, request.parent_cluster_number, 1);

        uint16_t hi, lo, idxDir;
        uint32_t found = 0;
        for (uint32_t i = 0; i < (CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++)
        {
            /* Cari di dir_table_buf, untuk c*/
            if (memcmp(fat32_driver_state.dir_table_buf.table[i].name, request.name, 8) == 0)
            {
                hi = fat32_driver_state.dir_table_buf.table[i].cluster_high;
                lo = fat32_driver_state.dir_table_buf.table[i].cluster_low;
                idxDir = i;
                found = 1;
                break;
            }
        }

        if (found == 0)
        {
            return 1;
        }

        struct FAT32DriverState check;
        read_clusters(&check.dir_table_buf, idxDir, 1);

        int counter = 0;
        for (int i = 0; i < 64; i++)
        {
            if (check.dir_table_buf.table[i].user_attribute == UATTR_NOT_EMPTY)
            {
                counter++;
            }
        }

        if (counter <= 1)
        {
            /* Cari di FAT32 */
            /* Ambil dulu idx-nya */
            uint32_t idx = hi << 16 | lo;

            if (request.buffer_size == 0)
            {
                /* Folder */
                fat32_driver_state.fat_table.cluster_map[idx] = 0;
                write_clusters(&fat32_driver_state.fat_table, 1, 1);
            }

            else
            {
                /* Buat file */
                while (fat32_driver_state.fat_table.cluster_map[idx] != FAT32_FAT_END_OF_FILE)
                {
                    uint32_t temp = idx;
                    idx = fat32_driver_state.fat_table.cluster_map[idx];
                    fat32_driver_state.fat_table.cluster_map[temp] = 0;
                }

                fat32_driver_state.fat_table.cluster_map[idx] = 0;
                write_clusters(&fat32_driver_state.fat_table, 1, 1);
            }

            struct FAT32DirectoryTable new;
            for (uint32_t i = 0; i < (CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++)
            {
                if (i != idxDir)
                {
                    memcpy(&new.table[i], &fat32_driver_state.dir_table_buf.table[i], sizeof(fat32_driver_state.dir_table_buf.table[i]));
                }
            }

            memcpy(&fat32_driver_state.dir_table_buf, &new, sizeof(new));
            write_clusters(&fat32_driver_state.dir_table_buf, request.parent_cluster_number, 1);
            return 0;
        }

        else
        {
            return 2;
        }
    }
    return -1;
}
