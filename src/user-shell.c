#include <stdint.h>
#include "header/filesystem/fat32.h"
#include "header/stdlib/string.h"
#include "header/text/framebuffer.h"


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

void clearScreen(){
    syscall(8,0,0,0);
}

void getExt(char* param, int* length, char ext[]){
    while(*param != '.' && *param !='\0'){
        // printf("%d\n", *length);
        *length+=1;
        param++;
    }

    if (*param == '.') {
        for(size_t i=0; i<3; i++){
            ext[i] = param[i+1];
        }
    }
}


void splash()
{
    int delay = 3000;
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

    }else if (memcmp((char *)command, "mkdir", 5) == 0)
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
    }else if(memcmp((char*) command, "cp", 3)==0){
        printStr((char *) command, BIOS_LIGHT_BLUE);
    // rm 
    }else if(memcmp((char*) command, "rm", 3)==0){
        printStr((char *) command, BIOS_LIGHT_BLUE);
    // mv 
    }else if(memcmp((char*) command, "mv", 2)==0){
        printStr((char *) command, BIOS_LIGHT_BLUE);
        struct FAT32DriverRequest request = {
            .buf                   = &cl,
            .parent_cluster_number = ROOT_CLUSTER_NUMBER,
            .buffer_size           = 0,
        };
        request.buffer_size = 5*CLUSTER_SIZE;
        int length = 0;
        char* param = (char *) command + 3;
        char ext[3];
        getExt(param, &length, ext);
        memcpy(request.name, (void *) (command + 3), length);
        
        int32_t readCode;
        struct FAT32DriverRequest requestDst ={
            .buf                   = &cl,
            .parent_cluster_number = ROOT_CLUSTER_NUMBER,
            .buffer_size           = 0,
        };

        memcpy(requestDst.name, listDir[depth], 8);
        syscall(0, (uint32_t)&request, (uint32_t) &readCode, 0);
        // 1 kalo bukan file, -1 kalo buffernya ga cukup, 0 load ke req.buf sebesar hasi divceil, 2 kalo?
        switch (readCode){
            case 0:
                printStr("Berhasil membaca", BIOS_LIGHT_BLUE);
                break;
            case -1:
                printStr("Buffer ga cukup bos", BIOS_LIGHT_BLUE);
                break;
            case 1:
                printStr("Ini bukan file", BIOS_LIGHT_BLUE);
                break;
            case 2:
                printStr("Tidak ditemukan", BIOS_LIGHT_BLUE);
                break;
            default:
                printStr("Error", BIOS_LIGHT_BLUE);
                break;
        }

        int32_t codeWrite = 10;
        struct FAT32DirectoryTable table = {};
        requestDst.buf = &table;
        syscall(1, (uint32_t)&requestDst, (uint32_t)&readCode,0);
        for(int i=0; i<64; i++){
            if(memcmp(table.table[i].name, (void *)(command+3+length+5), len((char *)command+3+length+5))==0){
                request.parent_cluster_number = (table.table[i].cluster_high<<16) | table.table[i].cluster_low;
                syscall(2, (uint32_t) &request, (uint32_t)codeWrite, 0);
                break;
            }
        }
        // 0 sukses, 2 jika invalid parent cluster, 1 jika sudah ada file/folder, -1 penyimpanan tidak cukup 
        switch (codeWrite){
        case 0:
            printStr("Sukses", BIOS_BROWN);
            break;
        case -1:
            printStr("Penyimpanan tidak cukup", BIOS_BROWN);
            break;
        case 1:
            printStr("File/folder sudah ada", BIOS_BROWN);
            break;
        case 2:
            printStr("Invalid parent cluster", BIOS_BROWN);
            break;
        default:
            printStr("Error", BIOS_BROWN);
            break;
        }

        struct FAT32DriverRequest request3 = {
            .buf                   = &cl,
            .parent_cluster_number = listCluster[depth],
            .buffer_size           = 0,
        };
        request3.buffer_size = 5*CLUSTER_SIZE;
        for(size_t i=0;i<3;i++){
            request3.ext[i] = ext[i];
        }
        memcpy(request3.name, (void *) (command+3), 8);
        int32_t delCode;
        syscall(3, (uint32_t) &request3, (uint32_t) &delCode, 0);

        //delcode, 2 artinya direktori yang tidak kosong, kalo 0 direktori kosong dan berhasil 
        // 1 kalo ga ketemu namanya, 0 kalo berhasil hapus
        switch (delCode){
        case 0:
            printStr("Sukses menghapus", BIOS_BROWN);
            break;
        case 1:
            printStr("File/Folder tidak ditemukan", BIOS_BROWN);
            break;
        case 2:
            printStr("Sukses menghapus", BIOS_BROWN);
            break;
        
        default:
            break;
        }
        
        

    // cls 
    }else if(memcmp((char*) command, "cls", 3)==0){
        printStr((char *) command, BIOS_LIGHT_BLUE);
        clearScreen();
    // find
    }else{
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

    syscall(7, 0, 0, 0);

    // splashscreemn
    splash();

    // bersihkan frame
    syscall(8, 0, 0, 0);

    // mulai frame utama
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
        // printStr("%", BIOS_LIGHT_GREEN);
        // syscall(5, (uint32_t) terminal, strlen(terminal), 0x2);
        syscall(4, (uint32_t)&buf, 0, 0);
        // printStr(buf, BIOS_PINK);
        parseCommand((uint32_t)&buf);
        printStr("\n", BIOS_BLACK);
    }

    return 0;
}