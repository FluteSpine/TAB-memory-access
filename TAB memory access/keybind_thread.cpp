#include <iostream>
#include <windows.h>

#include <vector>
#include <map>

#include "memory_manip.h"

extern std::atomic_uintptr_t LevelState;

uintptr_t find_destroy_command(HANDLE handle_process, uintptr_t LevelState)
{
	if (LevelState == 0)
		return 0;
	wchar_t name_of_destroy[8];
	uintptr_t destroy_command_name_addr;
	uintptr_t destroy_command = 0;
	uintptr_t destroy_command_addr = 0;
	while (destroy_command == 0)
	{
		destroy_command_name_addr = FindDMAAddy(handle_process, LevelState, { 0xD0, 0x38, 0x20, 0x8, 0x1A8, 0x18, 0x90, 0x20, 0x8, 0x10, 0xC });
		ReadProcessMemory(handle_process, (BYTE*)destroy_command_name_addr, &name_of_destroy, sizeof(name_of_destroy), nullptr);
		if (!wcscmp(name_of_destroy, L"Destroy"))
		{
			destroy_command_addr = FindDMAAddy(handle_process, LevelState, { 0xD0, 0x38, 0x20, 0x8, 0x1A8, 0x18, 0x90, 0x20 });
			ReadProcessMemory(handle_process, (BYTE*)destroy_command_addr, &destroy_command, sizeof(destroy_command), nullptr);
			std::cout << "Grabbed destroy command\n";
			return destroy_command;
		}
	}
}

void adjust_keybinds(HANDLE handle_process, DWORD process_ID)
{
	//using this for both visible and illuminated
	bool visible = true;

	//in game numbers for keybinds
	//they follow order in settings
	int delete_keybind = 45;
	int bonus_keybind = 25;
	int defense_keybind = 26;
	int colonists_keybind = 27;
	int wonders_keybind = 28;
	int energy_keybind = 29;
	int industry_keybind = 30;
	int military_keybind = 31;
	int resources_keybind = 32;

	int amount_of_commands = 0; //checking from all POSSIBLE commands from building/unit
	uintptr_t amount_of_commands_addr;


	//grabbing this command ensures command manager exists in the first place and following functions will not miss
	uintptr_t destroy_command = 0;
	while (destroy_command == 0)
		destroy_command = find_destroy_command(handle_process, LevelState);


	//they belong to same function tbh, maybe i could just remove it whole, but too much thinking
	// i dont know a better way to find that private data with thread environmental block
	//it seems to be always around this module tho
	uintptr_t start_ps_search = get_module_base_address(process_ID, L"System.Windows.Forms.ni.dll") - 0x000100000000;
	uintptr_t end_ps_search = 0x7FFFFFFFFFFF;
	void* function_that_writes_destroy_addr = pattern_scan_external(handle_process, start_ps_search, end_ps_search, //void cuz i made patternscan void*
		(char*)"\x48\x89\x41\x20\x48\x89\x41\x10\x33\xC0", (char*)"xxxxxxxxxx");

	//they belong to same function, no need for extra scans
	uintptr_t function_that_writes_keybind_addr = (uintptr_t)function_that_writes_destroy_addr + 0x3D;
	uintptr_t function_that_writes_visible_addr = (uintptr_t)function_that_writes_destroy_addr + 0x31;
	uintptr_t function_that_writes_illuminated_addr = (uintptr_t)function_that_writes_destroy_addr - 0x11;
	std::cout << std::hex << function_that_writes_destroy_addr << " found address of command manager\n";

	//in some very rare cases previous search doesnt find it
	//workaround till end of time, should just end thread if count higher than 16
	int attempts = 1;
	while (function_that_writes_destroy_addr == nullptr)
	{
		attempts++;
		std::cout << "Attempt: " << attempts << "\n";
		end_ps_search = start_ps_search;
		start_ps_search = start_ps_search - 0x000800000000;
		function_that_writes_destroy_addr = pattern_scan_external(handle_process, start_ps_search, end_ps_search,
			(char*)"\x48\x89\x41\x20\x48\x89\x41\x10\x33\xC0", (char*)"xxxxxxxxxx");		
	}

	//maybe it's fixed distance from previous, i dont know and cba to check since this is very fast anyways
	void* function_that_writes_CC_submenu_keybind_addr = pattern_scan_external(handle_process, (uintptr_t)function_that_writes_destroy_addr - 0xF000000, end_ps_search,
		(char*)"\x89\x45\x58\x48\x8B\x96", (char*)"xxxxxx");

	std::cout << function_that_writes_CC_submenu_keybind_addr << " found address of CC submenu commands" << std::dec << std::endl;

	while (true)
	{
		while (LevelState != 0)
		{
			std::cout << "In game with both command managers grabbed\n";
			amount_of_commands = 0;

			//might rename this to row3_column5
			uintptr_t destroy_command_addr = FindDMAAddy(handle_process, LevelState, { 0xD0, 0x38, 0x20, 0x8, 0x1A8, 0x18, 0x90, 0 });

			//eeh, but i dont see a better way, cuz you need to finddma for each
			//maybe command as a whole is exact size and i can plus from one to another, but not sure
			uintptr_t row1_column1 = FindDMAAddy(handle_process, LevelState, { 0xD0, 0x38, 0x20, 0x8, 0x1A8, 0x18, 0x20, 0 });
			uintptr_t row2_column1 = FindDMAAddy(handle_process, LevelState, { 0xD0, 0x38, 0x20, 0x8, 0x1A8, 0x18, 0x28, 0 });
			uintptr_t row3_column1 = FindDMAAddy(handle_process, LevelState, { 0xD0, 0x38, 0x20, 0x8, 0x1A8, 0x18, 0x30, 0 });

			uintptr_t row1_column2 = FindDMAAddy(handle_process, LevelState, { 0xD0, 0x38, 0x20, 0x8, 0x1A8, 0x18, 0x38, 0 });
			uintptr_t row2_column2 = FindDMAAddy(handle_process, LevelState, { 0xD0, 0x38, 0x20, 0x8, 0x1A8, 0x18, 0x40, 0 });
			uintptr_t row3_column2 = FindDMAAddy(handle_process, LevelState, { 0xD0, 0x38, 0x20, 0x8, 0x1A8, 0x18, 0x48, 0 });

			uintptr_t row1_column3 = FindDMAAddy(handle_process, LevelState, { 0xD0, 0x38, 0x20, 0x8, 0x1A8, 0x18, 0x50, 0 });
			uintptr_t row2_column3 = FindDMAAddy(handle_process, LevelState, { 0xD0, 0x38, 0x20, 0x8, 0x1A8, 0x18, 0x58, 0 });

			//to distinguish what/when should be noped
			bool patched = true;
			bool noped_units = false;
			bool noped_CC = false;

			//too dumb to not grab commands second time
			//too lazy to make a variable that checks if they were grabbed
			//so this is a worthless while true loop
			while (true)
			{
				switch (amount_of_commands)
				{
				case 64: //CC
					if (noped_CC == false)
					{
						//technically dont need to patch this and leave destroy command on cc
						//not like pressing destroy on cc does anything without further changes in EntityDefaultParams
						//but this is standard visual

						//bytes are original bytecodes for commands, check cheat engine for them if needed
						patch_external((BYTE*)function_that_writes_destroy_addr, (BYTE*)"\x48\x89\x41\x20", 4, handle_process);
						patch_external((BYTE*)function_that_writes_keybind_addr, (BYTE*)"\x89\x41\x58", 3, handle_process);
						patch_external((BYTE*)function_that_writes_visible_addr, (BYTE*)"\xC6\x41\x5C\x00", 4, handle_process);
						patch_external((BYTE*)function_that_writes_illuminated_addr, (BYTE*)"\xC6\x41\x5D\x00", 4, handle_process);
						
						//the writing instruction for submenus is in some god forsaken place

						//not sure i actually need to EVER patch it back
						nop_external((BYTE*)function_that_writes_CC_submenu_keybind_addr, 3, handle_process);
						nop_external((BYTE*)function_that_writes_keybind_addr, 3, handle_process);

						WriteProcessMemory(handle_process, (BYTE*)(row1_column1 + 0x58), &colonists_keybind, sizeof(colonists_keybind), nullptr);
						WriteProcessMemory(handle_process, (BYTE*)(row2_column1 + 0x58), &industry_keybind, sizeof(industry_keybind), nullptr);
						WriteProcessMemory(handle_process, (BYTE*)(row3_column1 + 0x58), &bonus_keybind, sizeof(bonus_keybind), nullptr);

						WriteProcessMemory(handle_process, (BYTE*)(row1_column2 + 0x58), &resources_keybind, sizeof(resources_keybind), nullptr);
						WriteProcessMemory(handle_process, (BYTE*)(row2_column2 + 0x58), &military_keybind, sizeof(military_keybind), nullptr);
						WriteProcessMemory(handle_process, (BYTE*)(row3_column2 + 0x58), &wonders_keybind, sizeof(wonders_keybind), nullptr);

						WriteProcessMemory(handle_process, (BYTE*)(row1_column3 + 0x58), &energy_keybind, sizeof(energy_keybind), nullptr);
						WriteProcessMemory(handle_process, (BYTE*)(row2_column3 + 0x58), &defense_keybind, sizeof(defense_keybind), nullptr);

						noped_CC = true;
						patched = false;
						noped_units = false;
						std::cout << "noped CC\n";
					}
					break;
				case 9: case 10: // fails on enemy zombies, mutants and stone towers, cuz mutants dont have hold and have 8 commands
					if (noped_units == false)
					{
						patch_external((BYTE*)function_that_writes_CC_submenu_keybind_addr, (BYTE*)"\x89\x45\x58", 3, handle_process);

						nop_external((BYTE*)function_that_writes_destroy_addr, 4, handle_process);
						WriteProcessMemory(handle_process, (BYTE*)(destroy_command_addr + 0x20), &destroy_command, sizeof(destroy_command), nullptr);
						nop_external((BYTE*)function_that_writes_keybind_addr, 3, handle_process);
						WriteProcessMemory(handle_process, (BYTE*)(destroy_command_addr + 0x58), &delete_keybind, sizeof(delete_keybind), nullptr);
						nop_external((BYTE*)function_that_writes_visible_addr, 4, handle_process);
						WriteProcessMemory(handle_process, (BYTE*)(destroy_command_addr + 0x5C), &visible, sizeof(visible), nullptr);
						nop_external((BYTE*)function_that_writes_illuminated_addr, 4, handle_process);
						WriteProcessMemory(handle_process, (BYTE*)(destroy_command_addr + 0x5D), &visible, sizeof(visible), nullptr);

						noped_CC = false;
						noped_units = true;
						patched = false;
						std::cout << "noped units\n";
					}
					break;
				default:
					if (patched == false)
					{
						patch_external((BYTE*)function_that_writes_CC_submenu_keybind_addr, (BYTE*)"\x89\x45\x58", 3, handle_process);
						patch_external((BYTE*)function_that_writes_destroy_addr, (BYTE*)"\x48\x89\x41\x20", 4, handle_process);
						patch_external((BYTE*)function_that_writes_keybind_addr, (BYTE*)"\x89\x41\x58", 3, handle_process);
						patch_external((BYTE*)function_that_writes_visible_addr, (BYTE*)"\xC6\x41\x5C\x00", 4, handle_process);
						patch_external((BYTE*)function_that_writes_illuminated_addr, (BYTE*)"\xC6\x41\x5D\x00", 4, handle_process);

						noped_CC = false;
						noped_units = false;
						patched = true;
						std::cout << "patched back\n";
					}
					break;
				}


				//uses quite a lot of cpu without sleep
				Sleep(10); //what would be reasonable human reaction / button press time?
				//checking every loop to see what to do
				amount_of_commands_addr = FindDMAAddy(handle_process, LevelState, { 0xD0, 0x38, 0x20, 0x8, 0x30, 0x18 });
				ReadProcessMemory(handle_process, (BYTE*)amount_of_commands_addr, &amount_of_commands, sizeof(amount_of_commands), nullptr);

				if (LevelState == 0)
				{
					patch_external((BYTE*)function_that_writes_CC_submenu_keybind_addr, (BYTE*)"\x89\x45\x58", 3, handle_process);
					patch_external((BYTE*)function_that_writes_destroy_addr, (BYTE*)"\x48\x89\x41\x20", 4, handle_process);
					patch_external((BYTE*)function_that_writes_keybind_addr, (BYTE*)"\x89\x41\x58", 3, handle_process);
					patch_external((BYTE*)function_that_writes_visible_addr, (BYTE*)"\xC6\x41\x5C\x00", 4, handle_process);
					patch_external((BYTE*)function_that_writes_illuminated_addr, (BYTE*)"\xC6\x41\x5D\x00", 4, handle_process);
					std::cout << "patched back\n";
					break;
				}
			}
		}
	}
}