#include <stdint.h>
#include "types.h"

internal
void
win32_debug_msg(char* format, ...) {
  va_list args;
  va_start(args, format);
  
  char buffer[1024];
  vsprintf_s((char*)buffer, array_count(buffer), format, args); 
  OutputDebugStringA((char*)buffer);
}

i32 
string_to_binary(char* str) {
	i32 size = string_len(str);
	i32 result = 0;
	char* ptr = str;
	i32 i = 0;
  
	while (*ptr) {
		i++;
		if (*ptr == '1') {
			result |= (1 << (size - i));
		}
		ptr++;
	}
  
	return result;
}

char* 
i32_to_string(i32 n) {
	local_persist char binary[8] = {};
	for (auto x = 0; x < 8; x++) {
		binary[x] = n & 0x80 ? '1' : '0';
		n <<= 1;
	}
	return binary;
}
