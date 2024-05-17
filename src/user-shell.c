#include <stdint.h>
#include "header/filesystem/fat32.h"


// Color
#define BIOS_LIGHT_GREEN 0b1010
#define BIOS_LIGHT_BLUE  0b1001
#define BIOS_WHITE       0b1111
#define BIOS_BLACK       0b0000
#define BIOS_GREY        0b0111
#define BIOS_DARK_GREY   0b1000
#define BIOS_RED         0b1100
#define BIOS_PINK        0b1101
#define BIOS_BROWN       0b0110

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

int strlen(char* str){
    int len = 0;
    while (str[len] != '\0')
        len++;
    return len;
}

void printStr(char* buf, uint8_t color){
    syscall(6, (uint32_t) buf, strlen(buf), color);
}

void splash(){
    printStr("                              _______ _______ _     _\n", BIOS_RED);
    printStr("                              |_____| |______ |     |\n", BIOS_RED);
    printStr("                              |     | ______| |_____|\n\n", BIOS_RED);
}

int main(void)
{
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

    char buf;
    syscall(7, 0, 0, 0);
    // char *terminal = "OperatingStess ";

    // buat nanti splashscreemn
    splash();
    while (true)
    {   
        printStr("OperatingStess", BIOS_LIGHT_GREEN);
        printStr(":/ ", BIOS_GREY);
        // syscall(5, (uint32_t) terminal, strlen(terminal), 0x2);
        syscall(4, (uint32_t)&buf, 0, 0);
        // syscall(5, (uint32_t)&buf, 0xF, 0);
    }

    return 0;
}
