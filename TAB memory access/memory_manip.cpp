#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <winternl.h>

#include <vector>
#include <map>

//dont ask me about this
typedef enum _mTHREADINFOCLASS {
	ThreadBasicInformation = 0
} mTHREADINFOCLASS;
typedef struct _mTEB {
	NT_TIB                  Tib;
	PVOID                   EnvironmentPointer;
	CLIENT_ID               Cid;
	PVOID                   ActiveRpcInfo;
	PVOID                   ThreadLocalStoragePointer;
	PPEB                    Peb;
	ULONG                   LastErrorValue;
	ULONG                   CountOfOwnedCriticalSections;
	PVOID                   CsrClientThread;
	PVOID                   Win32ThreadInfo;
	ULONG                   Win32ClientInfo[0x1F];
	PVOID                   WOW32Reserved;
	ULONG                   CurrentLocale;
	ULONG                   FpSoftwareStatusRegister;
	PVOID                   SystemReserved1[0x36];
	PVOID                   Spare1;
	ULONG                   ExceptionCode;
	ULONG                   SpareBytes1[0x28];
	PVOID                   SystemReserved2[0xA];
	ULONG                   GdiRgn;
	ULONG                   GdiPen;
	ULONG                   GdiBrush;
	CLIENT_ID               RealClientId;
	PVOID                   GdiCachedProcessHandle;
	ULONG                   GdiClientPID;
	ULONG                   GdiClientTID;
	PVOID                   GdiThreadLocaleInfo;
	PVOID                   UserReserved[5];
	PVOID                   GlDispatchTable[0x118];
	ULONG                   GlReserved1[0x1A];
	PVOID                   GlReserved2;
	PVOID                   GlSectionInfo;
	PVOID                   GlSection;
	PVOID                   GlTable;
	PVOID                   GlCurrentRC;
	PVOID                   GlContext;
	NTSTATUS                LastStatusValue;
	UNICODE_STRING          StaticUnicodeString;
	WCHAR                   StaticUnicodeBuffer[0x105];
	PVOID                   DeallocationStack;
	PVOID                   TlsSlots[0x40];
	LIST_ENTRY              TlsLinks;
	PVOID                   Vdm;
	PVOID                   ReservedForNtRpc;
	PVOID                   DbgSsReserved[0x2];
	ULONG                   HardErrorDisabled;
	PVOID                   Instrumentation[0x10];
	PVOID                   WinSockData;
	ULONG                   GdiBatchCount;
	ULONG                   Spare2;
	ULONG                   Spare3;
	ULONG                   Spare4;
	PVOID                   ReservedForOle;
	ULONG                   WaitingOnLoaderLock;
	PVOID                   StackCommit;
	PVOID                   StackCommitMax;
	PVOID                   StackReserved;
} mTEB;
typedef NTSTATUS(WINAPI* fpNtQueryInformationThread)(
	IN HANDLE          ThreadHandle,
	IN mTHREADINFOCLASS ThreadInformationClass,
	OUT PVOID          ThreadInformation,
	IN ULONG           ThreadInformationLength,
	OUT PULONG         ReturnLength
	);
typedef struct _THREAD_BASIC_INFORMATION {

	NTSTATUS                ExitStatus;
	mTEB* TebBaseAddress;
	CLIENT_ID               ClientId;
	KAFFINITY               AffinityMask;
	KPRIORITY               Priority;
	KPRIORITY               BasePriority;
} THREAD_BASIC_INFORMATION, * PTHREAD_BASIC_INFORMATION;


DWORD get_process_ID(const wchar_t* process_name) {
	DWORD process_ID = 0;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap != INVALID_HANDLE_VALUE)
	{
		PROCESSENTRY32 procEntry;
		procEntry.dwSize = sizeof(procEntry);
		if (Process32First(hSnap, &procEntry))
		{
			do
			{
				//why going through list of all active processes and comparing name is the best way?
				if (!_wcsicmp(procEntry.szExeFile, process_name))
				{
					process_ID = procEntry.th32ProcessID;
					break;
				}
			} while (Process32Next(hSnap, &procEntry));
		}
	}
	CloseHandle(hSnap);
	return process_ID;
}

uintptr_t FindDMAAddy(HANDLE handle_process, uintptr_t ptr, std::vector<unsigned int> offsets)
{
	//find dynamic memory allocation address
	uintptr_t addr = ptr;
	for (unsigned int i = 0; i < offsets.size(); ++i)
	{
		ReadProcessMemory(handle_process, (BYTE*)addr, &addr, sizeof(addr), 0);
		addr += offsets[i];
	}
	return addr;
}

uintptr_t get_module_base_address(DWORD owner_PID, const wchar_t* mod_name)
{
	//same idea as ListProcessThreads, written a bit cleaner
	uintptr_t mod_base_addr = 0;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, owner_PID);
	if (hSnap != INVALID_HANDLE_VALUE)
	{
		MODULEENTRY32 modEntry;
		modEntry.dwSize = sizeof(modEntry);
		if (Module32First(hSnap, &modEntry))
		{
			do
			{
				//wcout << modEntry.szModule << std::endl;	was just curious about all names		
				if (!_wcsicmp(modEntry.szModule, mod_name))
				{
					mod_base_addr = (uintptr_t)modEntry.modBaseAddr;
					break;
				}

			} while (Module32Next(hSnap, &modEntry));
		}
	}
	CloseHandle(hSnap);
	return mod_base_addr;
}

std::vector<DWORD> list_process_threads(DWORD owner_PID)
{
	std::vector<DWORD> all_thread_ids;
	HANDLE hThreadSnap = INVALID_HANDLE_VALUE;
	THREADENTRY32 te32;

	//snapshot of all running threads  
	hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (hThreadSnap == INVALID_HANDLE_VALUE)
		return all_thread_ids;

	//fill in the size of the structure before using it 
	te32.dwSize = sizeof(THREADENTRY32);
	//retrieve information about the first thread
	//exit if unsuccessful
	if (!Thread32First(hThreadSnap, &te32))
	{
		CloseHandle(hThreadSnap);
		return all_thread_ids;
	}

	//walking the thread list of the system and comparing process id
	do
	{
		if (te32.th32OwnerProcessID == owner_PID)
		{
			all_thread_ids.push_back(te32.th32ThreadID);
		}
	} while (Thread32Next(hThreadSnap, &te32));


	//clean up the snapshot object
	CloseHandle(hThreadSnap);
	return(all_thread_ids);
}

std::vector<uintptr_t> thread_sizes(DWORD owner_PID, DWORD thread_id, HANDLE handle_process)
{
	//i dont remember why i wanted process id here
	std::vector<uintptr_t> size;
	HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, false, thread_id);
	if (hThread == NULL)
	{
		int error = GetLastError();
		std::cout << "OpenThread error: " << error << std::endl;
	}

	HMODULE h = LoadLibrary(L"ntdll.dll");
	fpNtQueryInformationThread NtQueryInformationThread = (fpNtQueryInformationThread)GetProcAddress(h, "NtQueryInformationThread");

	THREAD_BASIC_INFORMATION info = { 0 };
	NTSTATUS ret = NtQueryInformationThread(hThread, ThreadBasicInformation, &info, sizeof(THREAD_BASIC_INFORMATION), NULL);

	NT_TIB tib = { 0 };
	if (!ReadProcessMemory(handle_process, info.TebBaseAddress, &tib, sizeof(NT_TIB), nullptr))
	{
		int error = GetLastError();
		std::cout << "ReadProcessMemory error: " << error << std::endl;
	}

	//cout << tib.StackLimit << " to " << tib.StackBase << endl;
	size.push_back((uintptr_t)tib.StackLimit);
	size.push_back((uintptr_t)tib.StackBase);
	return size;
}

uintptr_t find_LevelState(HANDLE handle_process, DWORD process_ID)
{
	//it's either this or patternscanning, but refining bounds for scan is hard

	//dont need to make 50 variables, can just reassign this
	uintptr_t address_of_what_im_looking_for;
	uintptr_t address_with_offset;
	uintptr_t LevelState;

	//this dont cover all the possible situations, but at least very wide majority of them
	//i've encountered only a handful of cases when this doesnt work
	//removing 0x18 offset would give you GameState, but i never needed it
	std::vector<std::vector<unsigned int>> DMA_offsets =
	{
		{ 0x110, 0x470, 0x10, 0x530, 0x48, 0x8, 0x6A8, 0x0, 0x18 },
		{ 0x110, 0x438, 0x10, 0xC20, 0x30, 0x28, 0x140, 0x0, 0x18 },
		{ 0x110, 0x5B0, 0x2B0, 0x28, 0xA8, 0x8, 0x630, 0x0, 0x18 },
		{ 0x110, 0x5B0, 0x2B0, 0x28, 0xA8, 0x8, 0x600, 0x0, 0x18 }
	};

	//using both CC health and playable area just to be sure
	//they are independant structures and one could theoretically encounter them randomly individually
	//so checking for both ensures correct LevelState
	int CC_health;

	//why are this floats, i never seen them being different
	float playable_area_1;
	float playable_area_2;
	float playable_area_3;
	float playable_area_4;

	//just need the first one, it seems to always be threadstack0
	DWORD thread_id = list_process_threads(process_ID)[0];
	std::vector<uintptr_t> thread_size = thread_sizes(process_ID, thread_id, handle_process);

	//not convinced it's +=4, it might be 8 since 64 bit pointers are 8 bytes, but damn if i know
	for (uintptr_t address = thread_size[0]; address < thread_size[1]; address += 4)
	{
		for (unsigned int offset : {0xE40, 0xD28, 0xE10})
		{
			address_with_offset = address - offset;
			for (std::vector<unsigned int> DMA_part : DMA_offsets)
			{				
				LevelState = FindDMAAddy(handle_process, address_with_offset, DMA_part);

				//changed from mayor to playable area

				//nulling them in case read fails
				//maybe not required
				playable_area_1 = 0;
				playable_area_2 = 0;
				playable_area_3 = 0;
				playable_area_4 = 0;

				//this are 4 consecutive fields in DXVisionDXLevel
				address_of_what_im_looking_for = FindDMAAddy(handle_process, LevelState, { 0xD0, 0x78 });
				ReadProcessMemory(handle_process, (BYTE*)address_of_what_im_looking_for, &playable_area_1, sizeof(playable_area_1), nullptr);
				address_of_what_im_looking_for += 4; // FindDMAAddy(handle_process, LevelState, { 0xD0, 0x7C });
				ReadProcessMemory(handle_process, (BYTE*)address_of_what_im_looking_for, &playable_area_2, sizeof(playable_area_2), nullptr);
				address_of_what_im_looking_for += 4; // FindDMAAddy(handle_process, LevelState, { 0xD0, 0x80 });
				ReadProcessMemory(handle_process, (BYTE*)address_of_what_im_looking_for, &playable_area_3, sizeof(playable_area_3), nullptr);
				address_of_what_im_looking_for += 4; // FindDMAAddy(handle_process, LevelState, { 0xD0, 0x84 });
				ReadProcessMemory(handle_process, (BYTE*)address_of_what_im_looking_for, &playable_area_4, sizeof(playable_area_4), nullptr);

				//54, 54, 148, 148 is standard size survival map
				//should check other values to support custom maps if needed
				if (playable_area_1 == 54 && playable_area_2 == 54 && playable_area_3 == 148 && playable_area_4 == 148) 
				{
					//Following will always fail unless CC has been selected at least once in game
					//which actually happens if you load back into the map and for some reason never click CC
					address_of_what_im_looking_for = FindDMAAddy(handle_process, LevelState, { 0xC0, 0x118, 0x30 });
					ReadProcessMemory(handle_process, (BYTE*)address_of_what_im_looking_for, &CC_health, sizeof(CC_health), nullptr);
					//fuck people who dont repair CC, dont get it damaged in the first place
					//i guess also might fail on custom maps
					if (CC_health == 5000)
					{
						std::cout << "Found LevelState at: " << std::hex << LevelState << std::dec << std::endl;
						return LevelState;
					}

				}
			}
		}
	}
	std::cout << "Did not find LevelState\n";
	return 0;
}

void patch_external(BYTE* destination, BYTE* source, unsigned int size, HANDLE handle_process)
{
	DWORD old_protect;
	VirtualProtectEx(handle_process, destination, size, PAGE_EXECUTE_READWRITE, &old_protect);
	WriteProcessMemory(handle_process, destination, source, size, nullptr);
	VirtualProtectEx(handle_process, destination, size, old_protect, &old_protect);
}

void nop_external(BYTE* destination, unsigned int size, HANDLE handle_process)
{
	BYTE* nop_array = new BYTE[size];
	memset(nop_array, 0x90, size);

	patch_external(destination, nop_array, size, handle_process);
	delete[] nop_array;
}

void* Booyers_Moore_pattern_scan(char* base, size_t size, char* pattern, char* mask)
{
	unsigned int num_of_skips = 0;
	unsigned int pattern_length = strlen(pattern);

	std::map<char, unsigned int> mismatch;
	for (unsigned int i = 0; i < pattern_length; i++)
	{
		mismatch[pattern[i]] = max(1, pattern_length - i - 1);
	}

	for (unsigned int i = 0; i <= size - pattern_length; i += num_of_skips)
	{
		num_of_skips = 0;
		for (unsigned int j = pattern_length - 1; j > 0; j--)
		{
			if (pattern[j] != *(base + i + j)) 
			{
				//if this exists in mismatch table
				if (mismatch.count(pattern[j]) > 0) 
					//skip for predefined number till next occurance
					num_of_skips = mismatch[pattern[j]]; 
				else
					//skip the whole pattern
					num_of_skips = pattern_length; 
				break;
			}
		}
		if (num_of_skips == 0)
			return (void*)(base + i);
	}
	return nullptr;
}

void* pattern_scan(char* base, size_t size, char* pattern, char* mask)
{
	unsigned int pattern_length = strlen(pattern);
	for (unsigned int i = 0; i < size - pattern_length; i++)
	{
		bool found = true;
		for (unsigned int j = 0; j < pattern_length; j++)
		{
			if (mask[j] != '?' && pattern[j] != *(base+i+j))
			{
				found = false;
				break;
			}
		}
		if (found)
			return (void*)(base + i);
	}
	return nullptr;
}

void* pattern_scan_external(HANDLE handle_process, uintptr_t begin, uintptr_t end, char* pattern, char* mask)
{
	uintptr_t current_chunk = begin;
	SIZE_T bytes_read;

	while (current_chunk < end)
	{
		char buffer[4096];
		DWORD old_protect;
		VirtualProtectEx(handle_process, (void*)current_chunk, sizeof(buffer), PROCESS_ALL_ACCESS, &old_protect);
		ReadProcessMemory(handle_process, (void*)current_chunk, &buffer, sizeof(buffer), &bytes_read);

		//i have no idea when this can be non size of buffer or 0
		//but apparently request of reading memory can be partially completed/failed
		//i think skipping to next chunk is better when missing access to read
		//than returning from function
		if (bytes_read == 0)
			current_chunk = current_chunk + 4096;
			//return nullptr;
		else 
		{
			//rewrote standard scan to be more efficient
			//void* internalAddress = pattern_scan((char*)buffer, bytesRead, pattern, mask); 
			void* internal_address = Booyers_Moore_pattern_scan((char*)buffer, bytes_read, pattern, mask);
			if (internal_address != nullptr)
			{
				uintptr_t offset_from_buffer = (uintptr_t)internal_address - (uintptr_t)&buffer;
				return (void*)(current_chunk + offset_from_buffer);
			}
			else
				current_chunk = current_chunk + bytes_read;
		}

	}
	return nullptr;
}
