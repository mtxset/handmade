#include <windows.h>
#include <math.h>
#include "utils.h"
#include "file_io.h"

void debug_free_file(void* handle) {
  if (handle) { 
    VirtualFree(handle, 0, MEM_RELEASE);
  }
}

Debug_file_read_result 
debug_read_entire_file(char* file_name) {
  Debug_file_read_result result = {};
  
  auto file_handle = CreateFileA(file_name, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0);
  
  if (file_handle == INVALID_HANDLE_VALUE) {
    goto exit;
  }
  
  LARGE_INTEGER file_size;
  if (!GetFileSizeEx(file_handle, &file_size) || file_size.QuadPart == 0)
    goto exit;
  
  auto file_size_32 = truncate_u64_u32(file_size.QuadPart);
  result.content = VirtualAlloc(0, file_size_32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
  
  if (!result.content) 
    goto exit;
  
  DWORD bytes_read;
  if (!ReadFile(file_handle, result.content, file_size_32, &bytes_read, 0)) {
    debug_free_file(result.content);
    goto exit;
  }
  
  if (file_size_32 != bytes_read)
    goto exit;
  
  result.bytes_read = bytes_read;
  
  exit:
  CloseHandle(file_handle);
  return result;
}

bool debug_write_entire_file(char* file_name, u32 memory_size, void* memory) {
  auto file_handle = CreateFileA(file_name, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
  
  if (file_handle == INVALID_HANDLE_VALUE)
    goto error;
  
  DWORD bytes_written;
  if (!WriteFile(file_handle, memory, memory_size, &bytes_written, 0)) {
    goto error;
  }
  
  if (memory_size != bytes_written)
    goto error;
  
  CloseHandle(file_handle);
  return true;
  
  error:
  CloseHandle(file_handle);
  return false;
}