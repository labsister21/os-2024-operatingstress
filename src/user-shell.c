#include <stdint.h>
#include "header/filesystem/fat32.h"
#include "header/stdlib/string.h"

// Color
#define BIOS_LIGHT_GREEN 0b1010
#define BIOS_LIGHT_BLUE 0b1001
#define BIOS_WHITE 0b1111
#define BIOS_BLACK 0b0000
#define BIOS_GREY 0b0111
#define BIOS_DARK_GREY 0b1000
#define BIOS_RED 0b1100
#define BIOS_PINK 0b1101
#define BIOS_BROWN 0b0110
#define BIOS_PURPLE 0b1101

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx)
{
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    // Note : gcc usually use %eax as intermediate register,
    //        so it need to be the last one to mov
    __asm__ volatile("int $0x30");
}

void printStr(char *buf, uint8_t color)
{
    syscall(6, (uint32_t)buf, strlen(buf), color);
}

void splash()
{
    int delay = 2800;
    for(int n = 0 ; n < delay ; n++){
        printStr("\n", BIOS_BLACK);
        printStr("\n", BIOS_BLACK);
        printStr("\n", BIOS_BLACK);
        printStr("\n", BIOS_BLACK);
        printStr("\n", BIOS_BLACK);
        printStr("\n", BIOS_BLACK);
        printStr("\n", BIOS_BLACK);
        printStr("                      ____                    __  _          \n", BIOS_LIGHT_GREEN);
        printStr("                     / __ \\___  ___ _______ _/ /_(_)__  ___ _\n", BIOS_LIGHT_GREEN);
        printStr("                    / /_/ / _ \\/ -_) __/ _ `/ __/ / _ \\/ _ `/\n", BIOS_LIGHT_GREEN);
        printStr("                    \\____/ .__/\\__/_/  \\_,_/\\__/_/_//_/\\_, / \n", BIOS_LIGHT_GREEN);
        printStr("                        /_/  ____                     /___/  \n", BIOS_LIGHT_GREEN);
        printStr("                            / __/ /________ ___ ___                \n", BIOS_LIGHT_GREEN);
        printStr("                           _\\ \\/ __/ __/ -_|_-<(_-<                \n", BIOS_LIGHT_GREEN);
        printStr("                          /___/\\__/_/  \\__/___/___/                \n", BIOS_LIGHT_GREEN);
        printStr("\n", BIOS_BLACK);
        printStr("                                   loading...                \n", BIOS_WHITE);
        printStr("\n", BIOS_BLACK);
        printStr("\n", BIOS_BLACK);
        printStr("\n", BIOS_BLACK);
        printStr("\n", BIOS_BLACK);
        printStr("\n", BIOS_BLACK);
        printStr("\n", BIOS_BLACK);
        syscall(5,0,0,0);
    }
}

uint32_t id = 0;
uint32_t depth = 0;
uint32_t listCluster[100];
char *listDir[100];
struct ClusterBuffer cl = {0};
void parseCommand(uint32_t command)
{
    // cd
    if (memcmp((char *)command, "cd", 2) == 0)
    { // change directory
        printStr((char *)command, BIOS_LIGHT_BLUE);
        if (memcmp("..", (void *)command + 3, 2) == 0)
        {
            if (depth == 0)
            {
                printStr("Udah paling ujung bos", BIOS_PINK);
                return;
            }
            depth--;
            printStr("Berhasil pindah ke", 0x2);
            printStr(listDir[depth], 0xF);
            return;
        }
        else
        {
            // jika ingin masuk ke direktori
            struct FAT32DriverRequest request = {
                .buf = &cl,
                .parent_cluster_number = listCluster[depth],
                .buffer_size = 0};
            memcpy(request.name, listDir[depth], 8);
            int32_t retcode;
            struct FAT32DirectoryTable table = {};
            request.buf = &table;
            syscall(1, (uint32_t)&request, (uint32_t)&retcode, 0);
            for (int i = 0; i < 64; i++)
            {
                if (memcmp(table.table[i].name, (char *)command + 3, 8) == 0)
                {
                    if (table.table[i].attribute == ATTR_SUBDIRECTORY)
                    {
                        depth += 1;
                        listCluster[depth] = table.table[i].cluster_low | (table.table[i].cluster_high << 16);
                        listDir[depth] = table.table[i].name;
                        // printStr("Berhasil Pindah Direktori\n", BIOS_LIGHT_GREEN);
                        return;
                    }
                    else
                    {
                        printStr("Bukan sebuah folder\n", BIOS_RED);
                        return;
                    }
                }
            }
            printStr("Folder tidak ditemukan\n", BIOS_RED);
        }

        // ls
    }
    else if (memcmp((char *)command, "ls", 2) == 0)
    {
        printStr((char *)command, BIOS_LIGHT_BLUE);
        struct FAT32DriverRequest request = {
            .buf = &cl,
            .buffer_size = 0};
        if (!depth)
        {
            request.parent_cluster_number = listCluster[depth];
        }
        else
        {
            request.parent_cluster_number = listCluster[depth - 1];
            uint32_t temp = listCluster[depth - 1];
            listCluster[depth - 1] = temp;
        }
        memcpy(request.name, listDir[depth], 8);
        int32_t retcode;
        struct FAT32DirectoryTable table = {};
        request.buf = &table;
        syscall(1, (uint32_t)&request, (uint32_t)&retcode, 0);
        switch (retcode)
        {
        case 0:
            for (int i = 0; i < 64; i++)
            {
                printStr(table.table[i].name, BIOS_LIGHT_GREEN);
                if (table.table[i].name[0] == '\0')
                {
                    memcpy(listDir[depth], request.name, 8);
                }
            }
            break;
        case 1:
            printStr("Not a folder", BIOS_RED);
            break;
        case 2:
            printStr("Not found", BIOS_RED);
            break;
        default:
            printStr("Error", BIOS_RED);
            break;
        }

        // mkdir
    }
    else if (memcmp((char *)command, "mkdir", 5) == 0)
    {
        struct ClusterBuffer cbuf[5];
        struct FAT32DriverRequest request =
            {
                .buf = cbuf,
                .name = "",
                .ext = "\0\0\0",
                .parent_cluster_number = listCluster[id],
                .buffer_size = 0,
            };
        memcpy(request.name, (void *)(command + 6), 8);
        int32_t retcode;
        syscall(2, (uint32_t)&request, (uint32_t)&retcode, 0);
        if (retcode == 0)
            printStr("Berhasil membuat folder", BIOS_LIGHT_GREEN);
        else if (retcode == 1)
            printStr("File/Folder sudah ada", BIOS_RED);
        else if (retcode == 2)
            printStr("Parent tidak valid", BIOS_RED);
        else
            printStr("idk", BIOS_RED);
    }
    else if (memcmp((char *)command, "cat", 3) == 0)
    {
        struct FAT32DriverRequest request = {
            .buf = &cl,
            .parent_cluster_number = listCluster[depth],
            .buffer_size = 0,
        };
        request.buffer_size = 5 * CLUSTER_SIZE;
        int nameLen = 0;
        char *itr = (char *)command + 4;
        for (int i = 0; i < strlen(itr); i++)
        {
            if (itr[i] == '.')
            {
                request.ext[0] = itr[i + 1];
                request.ext[1] = itr[i + 2];
                request.ext[2] = itr[i + 3];
                break;
            }
            else
            {
                nameLen++;
            }
        }
        memcpy(request.name, (void *)(command + 4), nameLen);
        int32_t retcode;
        syscall(0, (uint32_t)&request, (uint32_t)&retcode, 0);
        if (retcode == 0)
        {
            printStr(request.buf, 0xF);
            printStr("", 0xF);
        }
        else if (retcode == 1)
            printStr("Bukan File ey", BIOS_RED);
        else if (retcode == 2)
            printStr("Gaada filenya", BIOS_RED);
        else
            printStr("Teuing error naon", BIOS_RED);

        // cp
    }
    else if (memcmp((char *)command, "cp", 2) == 0)
    {
        printStr((char *)command, BIOS_LIGHT_BLUE);
        // char *command;

        struct FAT32DriverRequest request = {
            .buf = &cl,
            .parent_cluster_number = listCluster[depth],
            .buffer_size = 0,
        };
        request.buffer_size = 5 * CLUSTER_SIZE;
        int nameLen = 0;
        char *itr = (char *)command + 3;
        for (int i = 0; i < strlen(itr); i++)
        {
            if (itr[i] == '.')
            {
                request.ext[0] = itr[i + 1];
                request.ext[1] = itr[i + 2];
                request.ext[2] = itr[i + 3];
                break;
            }
            else
            {
                nameLen++;
            }
        }
        memcpy(request.name, (void *)(command + 3), nameLen);
        int32_t retcode;
        syscall(0, (uint32_t)&request, (uint32_t)&retcode, 0);
        if (retcode == 0)
        {
            printStr(request.buf, 0xF);
            printStr("", 0xF);
        }
        else if (retcode == 1)
            printStr("Bukan File ey", BIOS_RED);
        else if (retcode == 2)
            printStr("Gaada filenya", BIOS_RED);
        else
            printStr("Teuing error naon", BIOS_RED);

        // direktori tujuan
        struct FAT32DriverRequest request2 = {
            .buf                   = &cl,
            .parent_cluster_number = ROOT_CLUSTER_NUMBER,
            .buffer_size           = 0,
        };
        memcpy(request2.name, listDir[id], 8);

        int32_t retcode2;
        struct FAT32DirectoryTable table = {};
        request2.buf = &table;    

        // baca direktori
        syscall(1, (uint32_t) &request2, (uint32_t) &retcode, 0);

        // write file (copy)
        for(int i = 0; i < 64; i++){
            char* name = table.table[i].name;
            char* gatau = (char *) (command + 3 + nameLen + 5);
            if(memcmp(name, (void * )(gatau), strlen((char *) gatau)) == 0){
                request.parent_cluster_number = (table.table[i].cluster_high << 16) | table.table[i].cluster_low;
                syscall(2, (uint32_t) &request, (uint32_t) &retcode2, 0);
                break;
            }
        }

        if (retcode2 == 0)
            printStr("Write success", 0x2);
        else if (retcode2 == 1)
            printStr("File/Folder already exist", 0x4);
        else if (retcode2 == 2)
            printStr("Invalid parent cluster", 0x4);
        else
            printStr("Target folder not found", 0x4);

    
    }
    // rm
    else if (memcmp((char *)command, "rm", 3) == 0)
    {
        printStr((char *)command, BIOS_LIGHT_BLUE);
        // mv
    }
    else if (memcmp((char *)command, "mv", 3) == 0)
    {
        printStr((char *)command, BIOS_LIGHT_BLUE);
        // find
    }
    else
    {
        printStr("Masukkin command yang bener dong", BIOS_LIGHT_BLUE);
    }
}

int main(void)
{
    listCluster[0] = ROOT_CLUSTER_NUMBER;
    listDir[0] = (char *)"ROOT\0\0\0";
    struct ClusterBuffer cl[2] = {0};
    struct FAT32DriverRequest request = {
        .buf = &cl,
        .name = "shell",
        .ext = "\0\0\0",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size = CLUSTER_SIZE,
    };
    int32_t retcode;
    syscall(0, (uint32_t)&request, (uint32_t)&retcode, 0);
    // if (retcode == 0)
    //     syscall(6, (uint32_t) "owo\n", 4, 0xF);

    // char *terminal = "OperatingStess ";

    // buat nanti splashscreemn
    splash();
    syscall(7, 0, 0, 0);
    syscall(5, 0, 0, 0);
    while (true)
    {

        char buf[256];
        printStr("OperatingStess@User:~", BIOS_LIGHT_GREEN);
        uint32_t i = 0;
        while (i != depth + 1)
        {
            printStr("/", BIOS_PURPLE);
            printStr(listDir[i], BIOS_PURPLE);
            i++;
        }
        printStr("%", BIOS_LIGHT_GREEN);
        // syscall(5, (uint32_t) terminal, strlen(terminal), 0x2);
        syscall(4, (uint32_t)&buf, 0, 0);
        // printStr(buf, BIOS_PINK);
        parseCommand((uint32_t)&buf);
        printStr("\n", BIOS_BLACK);
    }

    return 0;
}