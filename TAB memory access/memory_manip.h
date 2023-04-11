#pragma once

DWORD get_process_ID(const wchar_t* process_name);
uintptr_t get_module_base_address(DWORD owner_PID, const wchar_t* mod_name);
std::vector<DWORD> list_process_threads(DWORD owner_PID);
std::vector<uintptr_t> thread_sizes(DWORD owner_PID, DWORD thread_id, HANDLE handle_process);

uintptr_t FindDMAAddy(HANDLE handle_process, uintptr_t ptr, std::vector<unsigned int> offsets);
uintptr_t find_LevelState(HANDLE handle_process, DWORD process_ID);

void patch_external(BYTE* destination, BYTE* source, unsigned int size, HANDLE handle_process);
void nop_external(BYTE* destination, unsigned int size, HANDLE handle_process);

void* pattern_scan(char* base, size_t size, char* pattern, char* mask);
void* Booyers_Moore_pattern_scan(char* base, size_t size, char* pattern, char* mask);
void* pattern_scan_external(HANDLE handle_process, uintptr_t begin, uintptr_t end, char* pattern, char* mask);
