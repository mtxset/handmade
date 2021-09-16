/* date = August 6th 2021 1:55 am */

#ifndef FILE_IO_H
#define FILE_IO_H

#include "types.h"

struct Debug_file_read_result {
    u32 bytes_read;
    void* content;
};

//static void debug_free_file(void* handle);
//static debug_file_read_result debug_read_entire_file(char* file_name);
//static bool debug_write_entire_file(char* file_name, u32 memory_size, void* memory);

#endif //FILE_IO_H
