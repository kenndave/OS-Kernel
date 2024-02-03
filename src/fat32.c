#include "lib-header/stdtype.h"
#include "lib-header/fat32.h"
#include "lib-header/stdmem.h"
#include "libc-header/str.h"

const uint8_t fs_signature[BLOCK_SIZE] = {
    'C', 'o', 'u', 'r', 's', 'e', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',  ' ',
    'D', 'e', 's', 'i', 'g', 'n', 'e', 'd', ' ', 'b', 'y', ' ', ' ', ' ', ' ',  ' ',
    'L', 'a', 'b', ' ', 'S', 'i', 's', 't', 'e', 'r', ' ', 'I', 'T', 'B', ' ',  ' ',
    'M', 'a', 'd', 'e', ' ', 'w', 'i', 't', 'h', ' ', '<', '3', ' ', ' ', ' ',  ' ',
    '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '2', '0', '2', '3', '\n',
    [BLOCK_SIZE-2] = 'O',
    [BLOCK_SIZE-1] = 'k',
};

/* -- Driver Interfaces -- */

/**
 * Convert cluster number to logical block address
 * 
 * @param cluster Cluster number to convert
 * @return uint32_t Logical Block Address
 */
uint32_t cluster_to_lba(uint32_t cluster){
    //return (cluster-2 * CLUSTER_SIZE + CLUSTER_0_VALUE) / BLOCK_SIZE;
    return (cluster) * (CLUSTER_SIZE / BLOCK_SIZE);
}

/**
 * Initialize DirectoryTable value with parent DirectoryEntry and directory name
 * 
 * @param dir_table          Pointer to directory table
 * @param name               8-byte char for directory name
 * @param parent_dir_cluster Parent directory cluster number
 */
struct FAT32DriverState driver_state;
void init_directory_table(struct FAT32DirectoryTable *dir_table, uint32_t parent_dir_cluster,
                           uint32_t self_dir_cluster){
    char dot[] = ".\0\0\0\0\0\0\0", double_dot[] = "..\0\0\0\0\0\0";

    memcpy(dir_table->table[0].name, double_dot, 8);
    dir_table->table[0].user_attribute = UATTR_NOT_EMPTY;
    dir_table->table[0].attribute = ATTR_SUBDIRECTORY;
    dir_table->table[0].cluster_high = (uint16_t)(parent_dir_cluster >> 16);
    dir_table->table[0].cluster_low = (uint16_t)(parent_dir_cluster & 0xFFFF);

    memcpy(dir_table->table[1].name, dot, 8);
    dir_table->table[1].user_attribute = UATTR_NOT_EMPTY;
    dir_table->table[1].attribute = ATTR_SUBDIRECTORY;
    dir_table->table[1].cluster_high = (uint16_t)(self_dir_cluster >> 16);
    dir_table->table[1].cluster_low = (uint16_t)(self_dir_cluster & 0xFFFF);

    // temporary solution. TODO: change if possible to avoid iteration
    for (unsigned int i = 2; i < DIR_TABLE_LENGTH; ++i) {
        memcpy(dir_table->table[i].name, "\0", 8);
    }
}



/**
 * Checking whether filesystem signature is missing or not in boot sector
 * 
 * @return True if memcmp(boot_sector, fs_signature) returning inequality
 */
bool is_empty_storage(void){
    uint32_t boot_sector[BLOCK_SIZE]; // Initialize boot sector
    read_blocks(boot_sector, BOOT_SECTOR, 1); // read boot_sector pada lokasi LBA = 0 (BOOT_SECTOR)
    return memcmp(boot_sector, fs_signature, BLOCK_SIZE);
}

/**
 * Create new FAT32 file system. Will write fs_signature into boot sector and 
 * proper FileAllocationTable (contain CLUSTER_0_VALUE, CLUSTER_1_VALUE, 
 * and initialized root directory) into cluster number 1
 */
void create_fat32(void){
    // Write file system signature to boot sector
    write_blocks(fs_signature, BOOT_SECTOR, 1);

    // Initialize File Allocation Table
    driver_state.fat_table.cluster_map[0] = CLUSTER_0_VALUE;
    driver_state.fat_table.cluster_map[1] = CLUSTER_1_VALUE;
    driver_state.fat_table.cluster_map[2] = FAT32_FAT_END_OF_FILE; // Root directory = END_OF_FILE

    for (int i = 3; i < CLUSTER_MAP_SIZE; i++) {
        driver_state.fat_table.cluster_map[i] = FAT32_FAT_EMPTY_ENTRY; // assign all fat_table cluster with FAT32_FAT_EMPTY_ENTRY 
    }
    write_clusters(&driver_state.fat_table, 1, 1); // writing fat table to cluster number 1

    // initialize directory table for root directory
    // every \0 fill 1 slot in name to prevent overwrite
    init_directory_table(&driver_state.dir_table_buf, ROOT_CLUSTER_NUMBER, ROOT_CLUSTER_NUMBER); // root
    write_clusters(&driver_state.dir_table_buf, ROOT_CLUSTER_NUMBER, 1);  // writing directory table to root directory
}

/**
 * Initialize file system driver state, if is_empty_storage() then create_fat32()
 * Else, read and cache entire FileAllocationTable (located at cluster number 1) into driver state
 */
void initialize_filesystem_fat32(void){
    if (is_empty_storage()){
        create_fat32();
    } else{
        // Read FileAllocationTable and cache it into driver state  
        read_clusters(&driver_state.fat_table, 1, 1);
    }
}

/**
 * Write cluster operation, wrapper for write_blocks().
 * Recommended to use struct ClusterBuffer
 * 
 * @param ptr            Pointer to source data
 * @param cluster_number Cluster number to write
 * @param cluster_count  Cluster count to write, due limitation of write_blocks block_count 255 => max cluster_count = 63
 */
void write_clusters(const void *ptr, uint32_t cluster_number, uint8_t cluster_count){
    uint32_t lba = cluster_to_lba(cluster_number);
    uint32_t block_count = cluster_count * CLUSTER_BLOCK_COUNT;
    write_blocks(ptr, lba, block_count);
}

/**
 * Read cluster operation, wrapper for read_blocks().
 * Recommended to use struct ClusterBuffer
 * 
 * @param ptr            Pointer to buffer for reading
 * @param cluster_number Cluster number to read
 * @param cluster_count  Cluster count to read, due limitation of read_blocks block_count 255 => max cluster_count = 63
 */
void read_clusters(void *ptr, uint32_t cluster_number, uint8_t cluster_count){
    uint32_t lba = cluster_to_lba(cluster_number);
    uint32_t block_count = cluster_count * CLUSTER_BLOCK_COUNT;
    read_blocks(ptr, lba, block_count);
}
/* -- CRUD Operation -- */

/**
 *  FAT32 Folder / Directory read
 *
 * @param request buf point to struct FAT32DirectoryTable,
 *                name is directory name,
 *                ext is unused,
 *                parent_cluster_number is target directory table to read,
 *                buffer_size must be exactly sizeof(struct FAT32DirectoryTable)
 * @return Error code: 0 success - 1 not a folder - 2 not found - -1 unknown
 */

int8_t read_directory(struct FAT32DriverRequest request){
    if (request.parent_cluster_number > 1){
        // Read DirectoryTable from request.parent_cluster_number.
        read_clusters(&driver_state.dir_table_buf.table, request.parent_cluster_number, 1);

        // Iterating entire DirectoryTable except for index 0
        for (int i = 0; i < (int) (CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++){
            // Folder validation
            if (!memcmp(driver_state.dir_table_buf.table[i].name, request.name, 8) && (driver_state.dir_table_buf.table[i].attribute == ATTR_SUBDIRECTORY || driver_state.dir_table_buf.table[i].cluster_low == (uint16_t)(0x2))){
                if (request.buffer_size == sizeof(struct FAT32DirectoryTable)){

                    read_clusters(request.buf, ((uint32_t)(driver_state.dir_table_buf.table[i].cluster_high << 16) | driver_state.dir_table_buf.table[i].cluster_low), 1);
                    return 0; // success

                }
                else {
                    return -1; // invalid buffer size to read
                }
            }
        }
        return 2; // folder/file not found because loop is over
    }
    else{
        return -1; // INVALID PARENT CLUSTER NUMBER
    }
}

/**
 * FAT32 read, read a file from file system.
 *
 * @param request All attribute will be used for read, buffer_size will limit reading count
 * @return Error code: 0 success - 1 not a file - 2 not enough buffer - 3 not found - -1 unknown
 */
int8_t read(struct FAT32DriverRequest *request){
    // Validating parent_cluster_number
    if (request->parent_cluster_number > 1) {
        // Read DirectoryTable from request.parent_cluster_number.
        read_clusters(&driver_state.dir_table_buf, request->parent_cluster_number, 1);

        for (int i = 1; i < (int) (CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++){

            // File found
            if(!memcmp(driver_state.dir_table_buf.table[i].name, request->name, 8) 
            && !memcmp(driver_state.dir_table_buf.table[i].ext, request->ext, 3) 
            && driver_state.dir_table_buf.table[i].attribute != ATTR_SUBDIRECTORY) {

                if (driver_state.dir_table_buf.table[i].filesize > request->buffer_size) {
                    request->buffer_size = driver_state.dir_table_buf.table[i].filesize;
                    return 2; // NOT ENOUGH BUFFER

                }

                else {
                    if (driver_state.dir_table_buf.table[i].attribute == ATTR_SUBDIRECTORY || driver_state.dir_table_buf.table[i].filesize == 0) {

                        return 1; // NOT A FILE

                    }
                    else {
                        // First Entry
                        uint32_t j = ((uint32_t)driver_state.dir_table_buf.table[i].cluster_high) << 16 | (uint32_t)driver_state.dir_table_buf.table[i].cluster_low; 
                        int k = 0; // Amount of cluster read

                        do {
                            read_clusters(request->buf + CLUSTER_SIZE*k, j, 1);
                            j = driver_state.fat_table.cluster_map[j];
                            k++; 
                        }
                        while(j != FAT32_FAT_END_OF_FILE && j <= CLUSTER_SIZE);

                        return 0;

                    }
                }
            }
        }
        return 3; // NOT FOUND
    }
    else {

        return -1; // INVALID PARENT CLUSTER NUMBER

    }
}

/**
 * FAT32 write, write a file or folder to file system.
 *
 * @param request All attribute will be used for write, buffer_size == 0 then create a folder / directory
 * @return Error code: 0 success - 1 file/folder already exist - 2 invalid parent cluster - -1 unknown
 */
int8_t write(struct FAT32DriverRequest request){
    // Validating request name & ext
    if (strlen(request.name) > 7 || strlen(request.ext) > 2){
        return -1; // length of name or ext is out of bounds.
    }

    // Validating parent_cluster_number
    if (request.parent_cluster_number > 1) {

        read_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);

        if (request.buffer_size == 0) {
            // write a folder
            for (int i = 2; i < (int) (CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {

                if (!memcmp(driver_state.dir_table_buf.table[i].name, request.name, 8) 
                && !memcmp(driver_state.dir_table_buf.table[i].ext, request.ext, 3)) { 
                    return 1; // file/folder already exists.
                }

            } // Making sure the folder doesnt exist in the table.
            for (int i = 2; i < (int) (CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {

                if (driver_state.dir_table_buf.table[i].user_attribute != UATTR_NOT_EMPTY) {
                     // there is empty slot to write.
                    memcpy(driver_state.dir_table_buf.table[i].name, request.name, 8);
                    driver_state.dir_table_buf.table[i].filesize = request.buffer_size;
                    driver_state.dir_table_buf.table[i].attribute = ATTR_SUBDIRECTORY;
                    driver_state.dir_table_buf.table[i].user_attribute = UATTR_NOT_EMPTY;
                    
                    write_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);
                    for (int l = 3; l < CLUSTER_MAP_SIZE; l++) {

                        if (driver_state.fat_table.cluster_map[l] == FAT32_FAT_EMPTY_ENTRY){
                            
                            // Set cluster_number for new folder
                            driver_state.dir_table_buf.table[i].cluster_high = (uint16_t) (l >> 16);
                            driver_state.dir_table_buf.table[i].cluster_low = (uint16_t) (l & 0xFFFF);
                            write_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);
                            
                            // Making a new folder
                            struct FAT32DirectoryTable new_folder;
                            init_directory_table(&new_folder, request.parent_cluster_number, (uint32_t) l);
                            write_clusters(&new_folder, l, 1);
                            driver_state.fat_table.cluster_map[l] = FAT32_FAT_END_OF_FILE;
                            write_clusters(&driver_state.fat_table, 1, 1);

                            return 0;
                        }

                    }
                    return -1; // STORAGE FULL
                }
            }
            return -1; // DIRECTORY TABLE FULL
        }
        else { 
            // write a file
            for (int i = 2; i < (int) (CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) { 
                if (!memcmp(driver_state.dir_table_buf.table[i].name, request.name, 8) 
                    && !memcmp(driver_state.dir_table_buf.table[i].ext, request.ext, 3)) {

                    return 1; // file/folder already exist
                    
                }
            } // Making sure file didnt exist in all entry.
            for (int i = 2; i < (int) (CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {

                if (driver_state.dir_table_buf.table[i].user_attribute != UATTR_NOT_EMPTY) { 
                    // there is empty slot to write.

                    // Set name and ext for the file
                    memcpy(driver_state.dir_table_buf.table[i].name, request.name, 8);
                    memcpy(driver_state.dir_table_buf.table[i].ext, request.ext, 3);

                    driver_state.dir_table_buf.table[i].filesize = request.buffer_size;
                    driver_state.dir_table_buf.table[i].user_attribute = UATTR_NOT_EMPTY;
                    write_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);

                    int x = 0, j = 0, k = 0;
                    do{ // Another iteration to search for empty slot until file reach EOF
                        if (driver_state.fat_table.cluster_map[j] == FAT32_FAT_EMPTY_ENTRY) {
                            // found empty clusters
                            if (k == 0) {

                                driver_state.dir_table_buf.table[i].cluster_high = (uint16_t) (j >> 16);
                                driver_state.dir_table_buf.table[i].cluster_low = (uint16_t) (j & 0xFFFF);
                                write_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);
                                // Writing entry_cluster in the directory table
                                if (request.buffer_size < CLUSTER_SIZE){
                                    driver_state.fat_table.cluster_map[j] = FAT32_FAT_END_OF_FILE;
                                    write_clusters(&driver_state.fat_table, 1, 1);
                                } 
                            }
                            else {

                                driver_state.fat_table.cluster_map[x] = j;
                                if (k == ceil(request.buffer_size, CLUSTER_SIZE)-1) {
                                    driver_state.fat_table.cluster_map[j] = FAT32_FAT_END_OF_FILE;
                                }
                                // Writing file done, updating fat_table.
                                write_clusters(&driver_state.fat_table, 1, 1);
                            }
                            // Writing file from buf
                            write_clusters((uint32_t*)(request.buf + (uint32_t)(CLUSTER_SIZE*k)), j, 1);
                            x = j;
                            k++;
                        }
                        j++;
                    }
                    while (k < ceil(request.buffer_size, CLUSTER_SIZE) && j <= CLUSTER_SIZE);
                    return 0;
                }
            }
            return -1; // Entry Full
        }
    }
    else{
        return 2; // Invalid parent cluster
    }
}

/**
 * FAT32 delete, delete a file or empty directory (only 1 DirectoryEntry) in file system.
 *
 * @param request buf and buffer_size is unused
 * @return Error code: 0 success - 1 not found - 2 folder is not empty - -1 unknown
 */
int8_t delete(struct FAT32DriverRequest request) {

    if (request.parent_cluster_number > 1) {

        read_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);
        for (int i = 2; i < (int) (CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {

            if (!memcmp(driver_state.dir_table_buf.table[i].name, request.name, 8) && 
                !memcmp(driver_state.dir_table_buf.table[i].ext, request.ext, 3) && 
                driver_state.dir_table_buf.table[i].attribute != ATTR_SUBDIRECTORY) { 
                // Searching for file to delete
                uint32_t entry_cluster = ((uint32_t)driver_state.dir_table_buf.table[i].cluster_high) << 16 | (uint32_t)driver_state.dir_table_buf.table[i].cluster_low;

                // Emptying dir_table_buf entry for deleted file
                driver_state.dir_table_buf.table[i].user_attribute = 0;
                memcpy(driver_state.dir_table_buf.table[i].name, "\0\0\0\0\0\0\0\0", 8);
                memcpy(driver_state.dir_table_buf.table[i].ext, "\0\0\0", 3);
                write_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);

                // Emptying fat_table
                do {
                    
                    uint32_t cluster = driver_state.fat_table.cluster_map[entry_cluster]; // previous cluster
                    driver_state.fat_table.cluster_map[entry_cluster] = FAT32_FAT_EMPTY_ENTRY; 
                    entry_cluster = cluster;
                    
                }
                while(entry_cluster != FAT32_FAT_END_OF_FILE);

                driver_state.fat_table.cluster_map[entry_cluster] = FAT32_FAT_EMPTY_ENTRY;
                write_clusters(&driver_state.fat_table, 1, 1); 

                return 0;
            }
            else if (!memcmp(driver_state.dir_table_buf.table[i].name, request.name, 8) && 
                driver_state.dir_table_buf.table[i].attribute == ATTR_SUBDIRECTORY) { 
                // Searching for folder to delete
                uint32_t entry_cluster = ((uint32_t)driver_state.dir_table_buf.table[i].cluster_high) << 16 | (uint32_t)driver_state.dir_table_buf.table[i].cluster_low;
                read_clusters(&driver_state.dir_table_buf, entry_cluster, 1);

                if (is_folder_empty()) { 
                    // if folder only contains one entry then delete
                    read_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);
                    // Remove from fat_table and parent dir table
                    driver_state.dir_table_buf.table[i].user_attribute = 0;
                    driver_state.dir_table_buf.table[i].attribute = 0;
                    driver_state.fat_table.cluster_map[(int)entry_cluster] = FAT32_FAT_EMPTY_ENTRY;
                    memcpy(driver_state.dir_table_buf.table[i].name, "\0\0\0\0\0\0\0\0", 8);
                    write_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);
                    write_clusters(&driver_state.fat_table, 1, 1);
                    

                    return 0;

                }
                else {
                    return 2; // folder not empty
                }
            }
        }
        return 1; // FILE / FOLDER NOT FOUND
    }   
    else {
        return -1; // INVALID PARENT CLUSTER_NUMBER
    }
}

int8_t move(struct FAT32DriverRequest request, struct FAT32DriverRequest request_dest){
    // Validasi panjang nama & ext
    if (strlen(request_dest.name) > 7 || strlen(request_dest.ext) > 2){
        return 4; // length of name or ext is out of bounds.
    }
    int destination = -1;
    
    // Validasi parent cluster
    if (request.parent_cluster_number > 1 && request_dest.parent_cluster_number > 1){
        // Reading destination directory
        read_clusters(&driver_state.dir_table_buf, request_dest.parent_cluster_number, 1);

        // Moving folder
        if (request.buffer_size == 0){
            // Making sure folder new name doesnt exist in destination directory table
            for (int i = 2; i < (int) (CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {
                if (!memcmp(driver_state.dir_table_buf.table[i].name, request_dest.name, 8) 
                && !memcmp(driver_state.dir_table_buf.table[i].ext, request_dest.ext, 3)) {
                    return 3; // File/Folder already exists in destination.
                }
            }
            // Finding empty entry in destination folder
            for (int i = 2; i < (int) (CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {
                if (driver_state.dir_table_buf.table[i].user_attribute != UATTR_NOT_EMPTY) {
                    destination = i;
                }
            }
            if (destination == -1){
                return 2; // Destination entry full
            }
            // Reading parent directory and finding the folder to move
            read_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);

            for (int i = 2; i < (int) (CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {
                if (!memcmp(driver_state.dir_table_buf.table[i].name, request.name, 8) && 
                driver_state.dir_table_buf.table[i].attribute == ATTR_SUBDIRECTORY) {
                    // Folder found from request
                    // Getting entry information to be moved.
                    uint16_t cluster_high_folder = driver_state.dir_table_buf.table[i].cluster_high;
                    uint16_t cluster_low_folder = driver_state.dir_table_buf.table[i].cluster_low;
                    
                    // Emptying dir_table_buf entry for deleted file
                    driver_state.dir_table_buf.table[i].user_attribute = 0;
                    driver_state.dir_table_buf.table[i].attribute = 0;
                    memcpy(driver_state.dir_table_buf.table[i].name, "\0\0\0\0\0\0\0\0", 8);
                    write_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);
                    
                    // Filling destination entry with needed file

                    read_clusters(&driver_state.dir_table_buf, request_dest.parent_cluster_number, 1);
                    memcpy(driver_state.dir_table_buf.table[destination].name, request_dest.name, 8);
                    driver_state.dir_table_buf.table[destination].user_attribute = UATTR_NOT_EMPTY;
                    driver_state.dir_table_buf.table[destination].attribute = ATTR_SUBDIRECTORY;
                    driver_state.dir_table_buf.table[destination].cluster_high = cluster_high_folder;
                    driver_state.dir_table_buf.table[destination].cluster_low = cluster_low_folder;
                    write_clusters(&driver_state.dir_table_buf, request_dest.parent_cluster_number, 1);
                    
                    return 0; // Move folder successful
                } 
            }
            return 1; // FOLDER NOT FOUND
        }
        
        // Moving file
        else{
            // Making sure file new name doesnt exist in destination directory table
            for (int i = 2; i < (int) (CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {
                if (!memcmp(driver_state.dir_table_buf.table[i].name, request_dest.name, 8)
                && !memcmp(driver_state.dir_table_buf.table[i].ext, request_dest.ext, 3)) {
                    return 3; // File/folder already exists in destination.
                }
            }
            // Finding empty entry in destination folder
            for (int i = 2; i < (int) (CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {
                if (driver_state.dir_table_buf.table[i].user_attribute != UATTR_NOT_EMPTY) {
                    destination = i;
                }
            }
            if (destination == -1){
                return 2; // Destination entry full
            }
            // Reading parent directory and finding the file to move
            read_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);
            for (int i = 2; i < (int) (CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {
                if (!memcmp(driver_state.dir_table_buf.table[i].name, request.name, 8) &&
                !memcmp(driver_state.dir_table_buf.table[i].ext, request.ext, 3) && 
                driver_state.dir_table_buf.table[i].attribute != ATTR_SUBDIRECTORY) {
                    // File found from request
                    // Getting entry information to be moved.
                    uint16_t cluster_high_file = driver_state.dir_table_buf.table[i].cluster_high;
                    uint16_t cluster_low_file = driver_state.dir_table_buf.table[i].cluster_low;
                    uint32_t filesize = driver_state.dir_table_buf.table[i].filesize;
                     // Emptying dir_table_buf entry for deleted file
                    driver_state.dir_table_buf.table[i].user_attribute = 0;
                    memcpy(driver_state.dir_table_buf.table[i].name, "\0\0\0\0\0\0\0\0", 8);
                    memcpy(driver_state.dir_table_buf.table[i].ext, "\0\0\0", 3);
                    write_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);
                    

                    // Filling destination entry with needed file
                    read_clusters(&driver_state.dir_table_buf, request_dest.parent_cluster_number, 1);
                    memcpy(driver_state.dir_table_buf.table[destination].name, request_dest.name, 8);
                    memcpy(driver_state.dir_table_buf.table[destination].ext, request_dest.ext, 3);
                    driver_state.dir_table_buf.table[destination].user_attribute = UATTR_NOT_EMPTY;
                    driver_state.dir_table_buf.table[destination].cluster_high = cluster_high_file;
                    driver_state.dir_table_buf.table[destination].cluster_low = cluster_low_file;
                    driver_state.dir_table_buf.table[destination].filesize = filesize;
                    write_clusters(&driver_state.dir_table_buf, request_dest.parent_cluster_number, 1);
                   return 0; // Move file successful
                } 
            }
            return 1; // FILE NOT FOUND
        }
    }
    return -1; // INVALID PARENT OR DESTINATION CLUSTER
}

/**
 * Rounding up function
 * 
 * @param dividend 
 * @param divisor 
 * @return int 
 */
int ceil(int dividend, int divisor){
    if (dividend % divisor != 0) {
        return (dividend/divisor) + 1;
    }
    else {
        return (dividend/divisor);
    }
}

/**
 * Function to check whether the directory table only contains 1 entry
 * 
 * @return True if directory table only contains 1 entry, false for other
 */
bool is_folder_empty(void){
    for (int i = 2; i < (int) (CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {
        if (driver_state.dir_table_buf.table[i].user_attribute == UATTR_NOT_EMPTY) {
            return FALSE;
        }
    }
    return TRUE;
}




