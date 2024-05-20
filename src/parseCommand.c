void parseCommand(uint32_t buf){
    if (memcmp((char *) buf, "cd", 2) == 0)
    {
        if (memcmp("..", (void *) buf + 3, 2) == 0)
        {
            if (depth == 0)
            {
                printStr("Udah paling ujung bos", BIOS_PINK);
                return;
            }
            depth--;
            printStr("Berhasil pindah ke", 0x2);
            printStr(listName[depth], 0xF);
            return;
        }
        else
        {
            struct FAT32DriverRequest request = {
                .buf = &cl,
                .parent_cluster_number = listCluster[depth],
                .buffer_size = 0
            };
            memcpy(request.name, listName[depth], 8);
            int32_t retcode = listCluster[depth];
            struct FAT32DirectoryTable table = {};
            request.buf = &table;
            syscall(1, (uint32_t) &request, (uint32_t) &retcode, 0);
            for (int i = 0 ; i < 64 ; i++)
            {
                if (memcmp(table.table[i].name, (char *) buf + 3, 8) == 0)
                {
                    depth++;
                    listCluster[depth] = table.table[i].cluster_low | (table.table[i].cluster_high << 16);
                    // memcpy(listName[depth],table.table[i].name,8);
                    listName[depth] = table.table[i].name;
                    // memcpy(listName[depth],cek2, 8);
                    printStr("Change directory success", 0x2);
                    return;
                }
            }
            printStr("No such directory", 0x4);
        }
    } 
    else if (memcmp((char *) buf, "ls", 2) == 0)
    {
        struct FAT32DriverRequest request = {
            .buf = &cl,
            .buffer_size = 0
        };
        if (depth!=0){
            request.parent_cluster_number = listCluster[depth-1];
            uint32_t cek = listCluster[depth-1];
            listCluster[depth-1] = cek;
        }
        else
        {
            request.parent_cluster_number = listCluster[depth];
        }
        memcpy(request.name, listName[depth],8);
        int32_t retcode;
        struct FAT32DirectoryTable table = {};
        request.buf = &table;
        syscall(1, (uint32_t) &request, (uint32_t) &retcode, 0);
        if (retcode == 0){
            for (int i = 0 ; i < 64 ; i++)
            {
                // char* name =table.table[i].name 
                printStr(table.table[i].name, 0xF);
                if (table.table[i].name[0] == '\0')
                {   
                    memcpy(listName[depth], request.name,8);
                    break;
                }
            }
        }
        else if (retcode == 1)
            printStr("Not a folder", 0x4);
        else if (retcode == 2)
            printStr("Not enough buffer", 0x4);
        else if (retcode == 3)
            printStr("Not found", 0x4);
        else
            printStr("Unknown error", 0x4);
    } 
    else if (memcmp((char *) buf, "mkdir", 5) == 0)
    {
        struct FAT32DriverRequest request = {
            .parent_cluster_number = listCluster[depth],
            .buffer_size           = 0,
        };
        memcpy(request.name, (void *) (buf + 6), 8);
        int32_t retcode;
        syscall(2, (uint32_t) &request, (uint32_t) &retcode, 0);
        if (retcode == 0)
            printStr("Write success", 0x2);
        else if (retcode == 1)
            printStr("File/Folder already exist", 0x4);
        else if (retcode == 2)
            printStr("Invalid parent cluster", 0x4);
        else
            printStr("Unknown error", 0x4);
    }
    else if (memcmp((char * ) buf, "cat", 3) == 0)
    {
        struct FAT32DriverRequest request = {
            .buf                   = &cl,
            .parent_cluster_number = listCluster[depth],
            .buffer_size           = 0,
        };
        request.buffer_size = 5*CLUSTER_SIZE;
        int nameLen = 0;
        char* itr = (char * ) buf + 4;
        for(size_t i = 0; i < len(itr) ; i++){
            if(itr[i] == '.'){
                request.ext[0] = itr[i+1];
                request.ext[1] = itr[i+2];
                request.ext[2] = itr[i+3];
                break;
            }else{
                nameLen++;
            }
        }
        memcpy(request.name, (void *) (buf + 4), nameLen);
        int32_t retcode;
        syscall(0, (uint32_t) &request, (uint32_t) &retcode, 0);
        if(retcode == 0){
            printStr(request.buf, 0xF);
            printStr("", 0xF);
        }
        else if (retcode == 1)
            printStr("Not a file", 0x4);
        else if (retcode == 2)
            printStr("Not enough buffer", 0x4);
        else if (retcode == 3)
            printStr("Not found", 0x4);
        else
            printStr("Unknown error", 0x4);

    } 
    else if (memcmp((char * ) buf, "cp", 2) == 0)
    {
        struct FAT32DriverRequest request = {
            .buf                   = &cl,
            .parent_cluster_number = ROOT_CLUSTER_NUMBER,
            .buffer_size           = 0,
        };
        request.buffer_size = 5*CLUSTER_SIZE;
        int nameLen = 0;
        char* itr = (char * ) buf + 3;
        for(size_t i = 0; i < len(itr) ; i++){
            if(itr[i] == '.'){
                request.ext[0] = itr[i+1];
                request.ext[1] = itr[i+2];
                request.ext[2] = itr[i+3];
                break;
            }else{
                nameLen++;
            }
        }

        memcpy(request.name, (void *) (buf + 3), nameLen);
        int32_t retcode;

        struct FAT32DriverRequest request2 = {
            .buf                   = &cl,
            .parent_cluster_number = ROOT_CLUSTER_NUMBER,
            .buffer_size           = 0,
        };
        
        memcpy(request2.name, listName[depth], 8);
        syscall(0, (uint32_t) &request, (uint32_t) &retcode, 0);

        /* Read the Destination Folder to paste */
        if (retcode == 0)
            printStr("Read success", 0x2);
        else if (retcode == 1){
            printStr("Not a file", 0x4);
            return;
        }
        else if (retcode == 2){
            printStr("Not enough buffer", 0x4);
            return;
        }
        else if (retcode == 3){
            printStr("File Not found", 0x4);
            return;
        }
        else{
            printStr("Unknown error", 0x4);
            return;
        }

        int32_t retcode2;
        struct FAT32DirectoryTable table = {};
        request2.buf = &table;
        syscall(1, (uint32_t) &request2, (uint32_t) &retcode, 0);
        // char* tes = (char *)buf + 3 + nameLen + 5;
        // char* its = tes;
        // its += 1;
        for(int i = 0; i < 64; i++){
            if(memcmp(table.table[i].name, (void * )(buf + 3 + nameLen + 5), len((char *) buf + 3 + nameLen + 5)) == 0){
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
    else if (memcmp((char *) buf, "rm", 2) == 0)
    {
        struct FAT32DriverRequest request = {
            .buf                   = &cl,
            .parent_cluster_number = listCluster[depth],
            .buffer_size           = 0,
        };
        request.buffer_size = 5*CLUSTER_SIZE;
        int nameLen = 0;
        char* itr = (char * ) buf + 3;
        for(size_t i = 0; i < len(itr) ; i++){
            if(itr[i] == '.'){
                request.ext[0] = itr[i+1];
                request.ext[1] = itr[i+2];
                request.ext[2] = itr[i+3];
                break;
            }else{
                nameLen++;
            }
        }
        memcpy(request.name, (void *) (buf + 3), 8);
        // memcpy(request.ext, "\0\0\0", 3);
        int32_t retcode;
        syscall(3, (uint32_t) &request, (uint32_t) &retcode, 0);
        if (retcode == 0)
            printStr("Delete Success", 0x2);
        else if (retcode == 1)
            printStr("File/Folder Not Found", 0x4);
        else if (retcode == 2)
            printStr("Folder is empty", 0x4);
        else
            printStr("Unknown Error", 0x4);
    } 













    
    else if (memcmp((char *) buf, "mv", 2) == 0)
    {
        // printStr(buf + 3, cpu.ecx, cpu.edx);
        struct FAT32DriverRequest request = {
            .buf                   = &cl,
            .parent_cluster_number = ROOT_CLUSTER_NUMBER,
            .buffer_size           = 0,
        };
        request.buffer_size = 5*CLUSTER_SIZE;
        int nameLen = 0;
        char* itr = (char * ) buf + 3;
        while()
        for(size_t i = 0; i < len(itr) ; i++){
            if(itr[i] == '.'){
                request.ext[0] = itr[i+1];
                request.ext[1] = itr[i+2];
                request.ext[2] = itr[i+3];
                break;
            }else{
                nameLen++;
            }
        }

        memcpy(request.name, (void *) (buf + 3), nameLen);
        int32_t retcode;

        struct FAT32DriverRequest request2 = {
            .buf                   = &cl,
            .parent_cluster_number = ROOT_CLUSTER_NUMBER,
            .buffer_size           = 0,
        };
        
        memcpy(request2.name, listName[depth], 8);
        syscall(0, (uint32_t) &request, (uint32_t) &retcode, 0);

        /* Read the Destination Folder to paste */
        if (retcode == 0)
            printStr("Read success", 0x2);
        else if (retcode == 1){
            printStr("Not a file", 0x4);
            return;
        }
        else if (retcode == 2){
            printStr("Not enough buffer", 0x4);
            return;
        }
        else if (retcode == 3){
            printStr("File Not found", 0x4);
            return;
        }
        else{
            printStr("Unknown error", 0x4);
            return;
        }

        int32_t retcode2;
        struct FAT32DirectoryTable table = {};
        request2.buf = &table;
        syscall(1, (uint32_t) &request2, (uint32_t) &retcode, 0);
        for(int i = 0; i < 64; i++){
            if(memcmp(table.table[i].name, (void * )(buf + 3 + nameLen + 5), len((char *) buf + 3 + nameLen + 5)) == 0){
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

        struct FAT32DriverRequest request3 = {
            .buf                   = &cl,
            .parent_cluster_number = listCluster[depth],
            .buffer_size           = 0,
        };
        request3.buffer_size = 5*CLUSTER_SIZE;
        int nameLen1 = 0;
        char* itr1 = (char * ) buf + 3;
        for(size_t i = 0; i < len(itr) ; i++){
            if(itr[i] == '.'){
                request3.ext[0] = itr1[i+1];
                request3.ext[1] = itr1[i+2];
                request3.ext[2] = itr1[i+3];
                break;
            }else{
                nameLen1++;
            }
        }
        memcpy(request3.name, (void *) (buf + 3), 8);
        // memcpy(request.ext, "\0\0\0", 3);
        int32_t retcode3;
        syscall(3, (uint32_t) &request3, (uint32_t) &retcode3, 0);
        if (retcode3 == 0)
            printStr("Move Success", 0x2);
        else if (retcode3 == 1)
            printStr("File/Folder Not Found", 0x4);
        else if (retcode3 == 2)
            printStr("Folder is empty", 0x4);
        else
            printStr("Unknown Error", 0x4);
    } 
    // else if (memcmp((char *) buf, "find", 7) == 0)
    // {
    //     char* filename;
    //     char *ext;
        
    //     for (size_t i = 0; i < len((char *) buf + 8); i++)
    //     {
    //         if (((char *) buf + 8)[i] == '.') // FILE
    //         {
    //             filename = (char *) buf + 8;
    //             ext = (char *) buf + 8 + i + 1;
    //             // remove everything after . in filename
    //             for (size_t i = 0; i < len(filename); i++)
    //             {
    //                 if (filename[i] == '.')
    //                 {
    //                     filename[i] = '\0';
    //                     break;
    //                 }
    //             }
    //             break;
    //         }
    //         else // FOLDER
    //         {
    //             filename = (char *) buf + 8;
    //             ext = "";
    //         }
    //     }
        // concat(ext, filename);

        // FINDER
    //     struct FAT32DriverRequest request = {
    //         .buf = &cl,
    //         .parent_cluster_number = listCluster[depth],
    //         .buffer_size = 0,
    //     };
    //     if (id!=0){
    //         request.parent_cluster_number = listCluster[id-1];
    //         uint32_t cek = listCluster[id-1];
    //         listCluster[id-1] = cek;
    //     }
    //     else
    //     {
    //         request.parent_cluster_number = listCluster[depth];
    //     }
    //     memcpy(request.name, listName[depth], 8);

    //     int32_t retcode;
    //     struct FAT32DirectoryTable table = {};
    //     request.buf = &table;
    //     syscall(1, (uint32_t) &request, (uint32_t) &retcode, 0);

    //     if (retcode == 0){
    //         for (int i = 0 ; i < 64 ; i++)
    //         {
    //             char* name = table.table[i].name; 
    //             if(memcmp(name, filename, len(filename)) == 0){
    //                 printStr("Found",0x2);
    //                 return;
    //             }
    //         }
    //     }
    //     printStr("Not found",0x4);

    // } 
    else if (memcmp((char *) buf, "cls", 3) == 0)
    {
        printStr("cls", 0xF);
    }
    else if (memcmp((char *) buf, "touch", 5) == 0)
    {
        struct FAT32DriverRequest request = {
            .buf                   = &cl,
            .parent_cluster_number = listCluster[depth],
            .buffer_size           = 0,
        };
        request.buffer_size = 5*CLUSTER_SIZE;

        /* get the name and ext of the file */
        int nameLen1 = 0;
        char* itr = (char * ) buf + 6;
        for(size_t i = 0; i < len(itr) ; i++){
            if(itr[i] == '.'){
                request.ext[0] = itr[i+1];
                request.ext[1] = itr[i+2];
                request.ext[2] = itr[i+3];
                break;
            }else{
                nameLen1++;
            }
        }

        memcpy(request.name, (void * ) buf + 6, nameLen1);
        uint32_t retcode;
        struct ClusterBuffer cbuf[5] = {};
        
        /* Dapetin isi filenya */
        char* isi = "";
        isi = (char *)buf + 6 + nameLen1 + 2 + 3;

        for (uint32_t i = 0; i < 5; i++)
            for (uint32_t j = 0; j < CLUSTER_SIZE; j++)
                cbuf[i].buf[j] = isi[j];
                
        /* Write to the file */
        request.buf = cbuf;
        syscall(2, (uint32_t) &request, (uint32_t) &retcode, 0);
        if(retcode == 0){
            printStr("Write success", 0x2);
        }else{
            printStr("Write unsuccessful", 0x4);
        }

    }
    else
    {
        printStr("Command not found", 0x4);
    }
}
