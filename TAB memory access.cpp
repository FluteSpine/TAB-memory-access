#include <iostream>
#include <windows.h>
#include <vector>
#include <thread>

// was previously used when i didnt know when to exit the search for rain 
// & checking if it was more than N seconds since last ingame update of some stat and then triggering readdress
//still used for global coverage quarry in case you never select CC
//dont need to include it, where do i get it from?
//#include <chrono>

//this thing is like 100kb alone
#include "json/value.h"
#include "json/json.h"
#include <fstream>

#include <random>

#include "memory_manip.h"
#include "keybind_thread.h"


#pragma warning(disable : 4996) //ThIs FuNcTiOn Or VaRiAbLe MiGhT bE uNsAFe

void print_description()
{
	std::cout << "Made by FluteSpine in free time, dont expect anything good from it\n\n";
	std::cout << "Thanks to plaskbar for help with mayors list\n";
	std::cout << "Thanks to DaneelTrevize for help with understanding saves\n";
	std::cout << "Thanks to Teknicians for making saves for me and suggesting ideas\n\n";
	std::cout << "Ctrl+P opens map.\n";
	std::cout << "Ctrl+I disables rain, removes darkening on borders in flat mode, disables darkening when far from units.\n";
	std::cout << "Ctrl+N changes visuals, kinda cheaty if you ask me.\n";
	std::cout << "Just launch this after launching the game. It will work (Copium) till game is closed.\n";
}

std::atomic_uintptr_t LevelState = 0;

int main()
{
	print_description();

	//im not gonna add 50 checks, let it silently crash if user is dumb
	std::ifstream mayors_file("tab_mayors.json", std::ifstream::binary); 
	Json::Value mayors;
	mayors_file >> mayors;

	const wchar_t* exe_name = L"TheyAreBillions.exe";
	DWORD process_ID = get_process_ID(exe_name);
	HANDLE handle_process = 0;
	handle_process = OpenProcess(PROCESS_ALL_ACCESS, NULL, process_ID);

	//i dont want to implement a waiting loop
	//it's just look for it, if 0 sleep N seconds, do again
	if (process_ID == 0 || handle_process == INVALID_HANDLE_VALUE)
	{
		std::cout << "\nGame is not open, I'm too lazy to allow waiting for game\n";
		return 0;
	}

	std::thread keybind_thread(adjust_keybinds, handle_process, process_ID);

	//variables for CTRL-P
	bool beholder_value = false;


	//for comparison and not cleaning file unecessarily
	uintptr_t previous_LevelState = 0;
	bool need_readdress = true;

	bool is_bob_renamed = false;
	//also not used, cuz name is confusing
	bool is_radar_renamed = false;
	//currently not used
	bool if_calculating_score = false;


	//virtual time is all the time scene was rendered
	//pauses only when game is being saved or scene is changed from game to main menu or maybe campaign map, never tested
	//using this to check if player is in the game
	double VirtualTime = 0;
	double VirtualTime_previous = 0;
	uintptr_t VirtualTime_addr = 0;


	//initializing visuals variables
	bool visuals_changed = false;

	//initializing file for mayors and vector of effects
	//cleaning up the existing file at start
	std::vector<std::string> list_of_effects_to_write;
	list_of_effects_to_write = { "", "", "", "" };
	std::ofstream my_file;
	my_file.open("filename.txt", std::ofstream::out | std::ofstream::trunc);
	my_file.close();

	//registering hotkeys
	// 3 is missing, was previously used for forcing readress
	if (RegisterHotKey(NULL, 1, MOD_CONTROL, 0x50))
		std::cout << "Hotkey 'CTRL+P' registered\n";
	if (RegisterHotKey(NULL, 2, MOD_CONTROL, 0x49))
		std::cout << "Hotkey 'CTRL+I' registered\n";
	if (RegisterHotKey(NULL, 4, MOD_CONTROL, 0x4E))
		std::cout << "Hotkey 'CTRL+N' registered\n";
	//message struct
	MSG msg = { 0 };

	while (true)
	{
		if (LevelState != 0)
		{
			for (unsigned int mayor_tier = 0x10, tier = 0; mayor_tier <= 0x40; mayor_tier += 0x10, tier++)
			{
				//reading mayors
				wchar_t mayor_name_wchar[256];
				uintptr_t mayor_name_wchar_addr = FindDMAAddy(handle_process, LevelState, { 0x98, 0x8, mayor_tier, 0xC });
				if (ReadProcessMemory(handle_process, (BYTE*)mayor_name_wchar_addr, &mayor_name_wchar, sizeof(mayor_name_wchar), nullptr))
				{
					//i got confused here when refactoring
					std::wstring mayor_wstring(mayor_name_wchar);
					std::string mayor_name_string(mayor_wstring.begin(), mayor_wstring.end());
					if (mayors[mayor_name_string].toStyledString() != "null\n")
					{
						if ("\"" + list_of_effects_to_write[tier] + "\"" + "\n" != mayors[mayor_name_string].toStyledString())
						{
							list_of_effects_to_write[tier] = mayors[mayor_name_string].toStyledString();

							//if (list_of_effects_to_write[tier][list_of_effects_to_write[tier].length() - 1] == '\n') //i dont need to check if it's \n
							list_of_effects_to_write[tier].erase(list_of_effects_to_write[tier].length() - 1); //erases \n

							list_of_effects_to_write[tier].erase(0, 1); // erase the first character "
							list_of_effects_to_write[tier].erase(list_of_effects_to_write[tier].length() - 1); // erase the last character "

							//just appending to the end of file
							my_file.open("filename.txt", std::ios_base::app);
							my_file << list_of_effects_to_write[tier] << "\n";
							my_file.close();
							my_file.clear();

							std::cout << list_of_effects_to_write[tier] << "\n";
						}
					}
				}
			}

			if (is_bob_renamed == false)
			{
				//for whatever reason this arent in the same order
				//so i iterate through all possible units

				//Bob variables and different descriptions
				//have to use struct, cuz you cant have vector of wchar_t* obviously
				//and i want to rng descriptions, but all of this could be removed

				srand(std::time(0));
				struct wide_char_array
				{
					wchar_t text[118];
				};

				wide_char_array description_1;
				wcscpy(description_1.text, L"There\'s nothing compared to awesomeness of Bob, the only contender is shiny Bob in golden armor & pleasure he brings.");
				wide_char_array description_2;
				wcscpy(description_2.text, L"Friend, savior, betrayer. Bob is all this and much more. He locates the VoD with ease, but sometimes commits suicide.");
				std::vector<wide_char_array> bob_descriptions = { description_1, description_2 };

				wchar_t new_bob_description[118];
				wcscpy(new_bob_description, bob_descriptions[rand() % bob_descriptions.size()].text);

				wchar_t new_bob_name[8] = L"Bob";
				uintptr_t unit_show_name_address;
				uintptr_t unit_name_addr;
				uintptr_t unit_show_description_address;

				wchar_t entity_name_wide[256];
				for (unsigned int unit_id = 0x10; unit_id <= 0x100; unit_id += 0x10) //this includes all possible units including heroes
				{
					unit_name_addr = FindDMAAddy(handle_process, LevelState, { 0xD0, 0x38, 0x20, 0x8, 0x250, 0x8, unit_id, 0x148, 0x10, 0xC });
					ReadProcessMemory(handle_process, (BYTE*)unit_name_addr, &entity_name_wide, sizeof(entity_name_wide), nullptr);
					if (!wcscmp(entity_name_wide, L"SoldierRegular"))
					{
						unit_show_name_address = FindDMAAddy(handle_process, LevelState, { 0xD0, 0x38, 0x20, 0x8, 0x250, 0x8, unit_id, 0x148, 0x18, 0xC });
						WriteProcessMemory(handle_process, (BYTE*)unit_show_name_address, &new_bob_name, sizeof(new_bob_name), nullptr);

						//description is defined earlier, random between a couple of them
						unit_show_description_address = FindDMAAddy(handle_process, LevelState, { 0xD0, 0x38, 0x20, 0x8, 0x250, 0x8, unit_id, 0x148, 0x20, 0xC });
						WriteProcessMemory(handle_process, (BYTE*)unit_show_description_address, &new_bob_description, sizeof(new_bob_description), nullptr);

						is_bob_renamed = true;
						std::cout << "POGGERS Renamed Bob!\n";
						break;
					}
				}
			}
			
			//initally assumed with it changing name on the map when u click it
			//i forgor it also changed mayor, which makes this undesirable
			//if (is_radar_renamed == false)
			//{
				//dont know why i went with actual name instead of programmed name of RadarTower
				//but i dont feel like changing it
				//at least it doesnt trigger on non english clients resulting in potential crash
			//	wchar_t radar_default_name[256];
			//	wchar_t new_radar_name[12] = L"DontYouDare";
			//
			//	uintptr_t radar_default_name_addr = FindDMAAddy(handle_process, LevelState, {0xD0, 0x38, 0x20, 0x8, 0x30, 0x8, 0x178, 0x58, 0x148, 0x18, 0xC});
			//	ReadProcessMemory(handle_process, (BYTE*)radar_default_name_addr, &radar_default_name, sizeof(radar_default_name), nullptr);
			//	if (!wcscmp(radar_default_name, L"Radar Tower"))
			//	{
			//		WriteProcessMemory(handle_process, (BYTE*)radar_default_name_addr, &new_radar_name, sizeof(new_radar_name), nullptr);
			//		is_radar_renamed = true;
			//	}
			//}

			if (if_calculating_score)
			{
				//i dont know when to start this thing
				//i can constantly be checking for amount of killed zombies or maybe grab day from ui
				//non of this seems right for lower amount of zombies or more days'
				//but at the same time *9 would be wrong too
				//can grab both from GameState and calculate accordingly, but ugh
				//not sure if nulling them is required either
				//can change finddmaaddy to simple +=m but essentially it's the same thing

				//addresses could be found only once at finding new LevelState
				//but this whole thing is a mess that nobody is interested in

				//this could be easily modified at runtime to cheat scores
				//amount of killed zombies is nigh impossible to check in the video

				int killed_zombies = 0;
				uintptr_t killed_zombies_address = FindDMAAddy(handle_process, LevelState, { 0x1B8 });
				ReadProcessMemory(handle_process, (BYTE*)killed_zombies_address, &killed_zombies, sizeof(killed_zombies), nullptr);

				int max_population = 0;
				uintptr_t max_population_address = FindDMAAddy(handle_process, LevelState, { 0x1CC });
				ReadProcessMemory(handle_process, (BYTE*)max_population_address, &max_population, sizeof(max_population), nullptr);

				int infected = 0;
				uintptr_t infected_address = FindDMAAddy(handle_process, LevelState, { 0x1C8 });
				ReadProcessMemory(handle_process, (BYTE*)infected_address, &infected, sizeof(infected), nullptr);

				int lost_units = 0;
				uintptr_t lost_units_address = FindDMAAddy(handle_process, LevelState, { 0x1C0 });
				ReadProcessMemory(handle_process, (BYTE*)lost_units_address, &lost_units, sizeof(lost_units), nullptr);

				int score = 0;

				if (infected > 500)
					infected = 500;
				if (lost_units > 500)
					lost_units = 500;
				// i'll never understand the internal formula and thats fine
				score = (29000 + killed_zombies / 10 + max_population / 2 - infected * 10 - lost_units * 10) * 9; //20k for winning, 9k for all wonders, 9 is 900% multiplier
				std::cout << "Score prediction (assuming all wonders and 900%): " << score << std::endl;
			}
		}
		else
		{
			need_readdress = true;
		}


		VirtualTime = 0;
		if (ReadProcessMemory(handle_process, (BYTE*)VirtualTime_addr, &VirtualTime, sizeof(VirtualTime), nullptr))
		{
			if (VirtualTime > VirtualTime_previous)
			{
				VirtualTime_previous = VirtualTime;
			}
			else
			{
				need_readdress = true;
			}
		}
		else
		{
			need_readdress = true;
		}

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0) //peek message, not get message cuz that one blocks execution
		{
			if (msg.message == WM_HOTKEY)
			{
				if (LevelState != 0)
				{
					switch (msg.wParam)
					{
					case 1:
					{
						wchar_t quarry_name[256];
						uintptr_t resource_collection_radius_addr;
						int resource_collection_radius;

						uintptr_t beholder_addr = FindDMAAddy(handle_process, LevelState, { 0x1D7 });
						beholder_value = !beholder_value;
						WriteProcessMemory(handle_process, (BYTE*)beholder_addr, &beholder_value, sizeof(beholder_value), nullptr);

						//surely you gonna select CC if you want to build a quarry, right
						//i never seen this result into infinite loop, but there might be a case
						//when you press beholder and never select CC again

						std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
						do
						{
							std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
							if (std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() > 5)
							{
								std::cout << "If you want op quarry, you need to select CC in 5 second period after pressing CTRL-P\n";
								break;
							}
							uintptr_t quarry_name_addr = FindDMAAddy(handle_process, LevelState, { 0xD0, 0x38, 0x20, 0x8, 0x30, 0x8, 0xA8, 0x58, 0x148, 0x10, 0xC });
							ReadProcessMemory(handle_process, (BYTE*)quarry_name_addr, &quarry_name, sizeof(quarry_name), nullptr);
							resource_collection_radius_addr = FindDMAAddy(handle_process, LevelState, { 0xD0, 0x38, 0x20, 0x8, 0x30, 0x8, 0xA8, 0x58, 0x148, 0x1D8 });
						} while (wcscmp(quarry_name, L"Quarry")); //while not

						if (beholder_value)
						{
							if (!wcscmp(quarry_name, L"Quarry"))
							{
								resource_collection_radius = 5000;
								WriteProcessMemory(handle_process, (BYTE*)resource_collection_radius_addr, &resource_collection_radius, sizeof(resource_collection_radius), nullptr);
							}
							std::cout << "Beholder and op quarry enabled!\n"; //quote unquote enabled
						}

						else
						{
							if (!wcscmp(quarry_name, L"Quarry"))
							{
								resource_collection_radius = 2;
								WriteProcessMemory(handle_process, (BYTE*)resource_collection_radius_addr, &resource_collection_radius, sizeof(resource_collection_radius), nullptr);
							}
							std::cout << "Beholder and op quarry disabled\n";
						}

						//looping through entities
						//some entities get moved around after save like vod buildings, giants and mutants
						//this doesnt check if minerals are obtainable (positioned so quarry can get them)
						//so to ensure correct stats run this right after finding LevelState for the first time
						//moving it there instead of button press allows to see all the information and is giga cheat
						//surely nobody will read my code and comments, right?
						//should move declarations before main loop

						uintptr_t entity_amount_addr = FindDMAAddy(handle_process, LevelState, { 0xD0, 0x8, 0x8, 0x18 });
						int entity_amount;
						ReadProcessMemory(handle_process, (BYTE*)entity_amount_addr, &entity_amount, sizeof(entity_amount), nullptr);

						uintptr_t template_name_addr;
						wchar_t template_name[256];

						int MineralGoldA_amount = 0;
						int MineralIronA_amount = 0;
						int MineralStoneA_amount = 0;
												
						int zombie_amount = 0;

						int DoomBuildingSmall_amount = 0;
						int DoomBuildingMedium_amount = 0;
						int DoomBuildingLarge_amount = 0;

						for (unsigned int i = 0; i <= 0x10 + entity_amount * 0x8; i += 0x8)
						{
							template_name_addr = FindDMAAddy(handle_process, LevelState, { 0xD0, 0x8, 0x8, 0x8, i, 0x58, 0x8, 0xC});
							ReadProcessMemory(handle_process, (BYTE*)template_name_addr, &template_name, sizeof(template_name), nullptr);
							
							if (!wcscmp(template_name, L"MineralGoldA"))
								MineralGoldA_amount++;
							if (!wcscmp(template_name, L"MineralIronA"))
								MineralIronA_amount++;
							if (!wcscmp(template_name, L"MineralStoneA"))
								MineralStoneA_amount++;

							if (!wcscmp(template_name, L"DoomBuildingSmall"))
								DoomBuildingSmall_amount++;
							if (!wcscmp(template_name, L"DoomBuildingMedium"))
								DoomBuildingMedium_amount++;
							if (!wcscmp(template_name, L"DoomBuildingLarge"))
								DoomBuildingLarge_amount++;

							if (!wcscmp(template_name, L"ZombieWeakA"))
								zombie_amount++;
							if (!wcscmp(template_name, L"ZombieWeakB"))
								zombie_amount++;
							if (!wcscmp(template_name, L"ZombieWeakC"))
								zombie_amount++;

							if (!wcscmp(template_name, L"ZombieWorkerA"))
								zombie_amount++;
							if (!wcscmp(template_name, L"ZombieWorkerB"))
								zombie_amount++;

							if (!wcscmp(template_name, L"ZombieMediumA"))
								zombie_amount++;
							if (!wcscmp(template_name, L"ZombieMediumB"))
								zombie_amount++;

							if (!wcscmp(template_name, L"ZombieStrongA"))
								zombie_amount++;
							if (!wcscmp(template_name, L"ZombieHarpy"))
								zombie_amount++;
							if (!wcscmp(template_name, L"ZombieVenom"))
								zombie_amount++;

							if (!wcscmp(template_name, L"ZombieGiant"))
								zombie_amount++;
							if (!wcscmp(template_name, L"ZombieMutant"))
								zombie_amount++;
							if (!wcscmp(template_name, L"ZombieWeakA"))
								zombie_amount++;
						}
						//well apparently it is amount of borders of mineral, but why is it like that?
						std::cout << "amount of gold tiles: " << MineralGoldA_amount / 4 << std::endl;
						std::cout << "amount of iron tiles: " << MineralIronA_amount / 4 << std::endl;
						std::cout << "amount of stone tiles: " << MineralStoneA_amount / 4 << std::endl;

						std::cout << "small buildings: " << DoomBuildingSmall_amount << std::endl;
						std::cout << "medium buildings: " << DoomBuildingMedium_amount << std::endl;
						std::cout << "large buildings: " << DoomBuildingLarge_amount << std::endl;

						std::cout << "total on map: " << zombie_amount << std::endl;
						//default amount is 40/150/500 per building
						//1.5 is caustic multiplier
						std::cout << "total in vod (assuming Caustic lands): " << (DoomBuildingSmall_amount*40 + DoomBuildingMedium_amount*150 + DoomBuildingLarge_amount * 500) * 1.5 << std::endl;

						
						break;
					}
					case 2:
					{
						bool rain_skip_render;
						bool DefaultImageFilter_value;
						bool LIMIT_LEFT_skip_render;
						if (visuals_changed == false) 
						{
							rain_skip_render = true;
							DefaultImageFilter_value = false;
							LIMIT_LEFT_skip_render = true;
							visuals_changed = true;
						}
						else
						{
							rain_skip_render = false;
							DefaultImageFilter_value = true;
							LIMIT_LEFT_skip_render = false;
							visuals_changed = false;
						}


						//used for finding rain name
						wchar_t object_name[256];

						//looping through objects
						uintptr_t entity_amount_addr = FindDMAAddy(handle_process, LevelState, { 0xD0, 0x38, 0xC0, 0x20, 0x28 });
						int entity_amount;
						ReadProcessMemory(handle_process, (BYTE*)entity_amount_addr, &entity_amount, sizeof(entity_amount), nullptr);
						
						for (unsigned int i = 0; i <= 0x10 + entity_amount * 0x10; i += 0x10)
						{							
							uintptr_t object_name_addr = FindDMAAddy(handle_process, LevelState, { 0xD0, 0x38, 0xC0, 0x20, 0x10, i, 0x138, 0xC});
							ReadProcessMemory(handle_process, (BYTE*)object_name_addr, &object_name, sizeof(object_name), nullptr);
							if (!wcscmp(object_name, L"RainA1"))
							{
								uintptr_t rain_skip_render_addr = FindDMAAddy(handle_process, LevelState, { 0xD0, 0x38, 0xC0, 0x20, 0x10, i, 0x169 });
								WriteProcessMemory(handle_process, (BYTE*)rain_skip_render_addr, &rain_skip_render, sizeof(rain_skip_render), nullptr);
								break;
							}
							
						}

						for (unsigned int i = 0xD8; i <= 0xF0; i+=0x8)
						{
							uintptr_t LIMIT_LEFT_skip_render_addr = FindDMAAddy(handle_process, LevelState, { 0xD0, 0x38, 0x20, 0x8, i, 0x169 });
							WriteProcessMemory(handle_process, (BYTE*)LIMIT_LEFT_skip_render_addr, &LIMIT_LEFT_skip_render, sizeof(LIMIT_LEFT_skip_render), nullptr);
						}

						uintptr_t DefaultImageFilter_addr = FindDMAAddy(handle_process, LevelState, { 0xD0, 0x38, 0xC0, 0x193 });
						WriteProcessMemory(handle_process, (BYTE*)DefaultImageFilter_addr, &DefaultImageFilter_value, sizeof(DefaultImageFilter_value), nullptr);

						std::cout << "Changed visuals!" << std::endl; //PeekMessage and Peek Map
						break;
					}
					case 4:
					{
						uintptr_t angle_addr = FindDMAAddy(handle_process, LevelState, { 0xD0, 0x38, 0xF8, 0x20 });
						for (float i = 0; i <= 360; i++)
						{
							WriteProcessMemory(handle_process, (BYTE*)angle_addr, &i, sizeof(i), nullptr);
							Sleep(10);
						}
						break;
					}
					}
				}
			}
		}
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0) //Clear message queue
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}


		Sleep(500);

		if (need_readdress)
		{
			LevelState = find_LevelState(handle_process, process_ID);

			VirtualTime_addr = FindDMAAddy(handle_process, LevelState, { 0xD0, 0x38, 0xC0, 0x110 });

			if (LevelState != previous_LevelState)
			{
				visuals_changed = false;
				beholder_value = false;

				VirtualTime_previous = 0;
				previous_LevelState = LevelState;

				list_of_effects_to_write = { "", "", "", "" };
				my_file.open("filename.txt", std::ofstream::out | std::ofstream::trunc);
				my_file.close();
				my_file.clear();
			}

			need_readdress = false;
		}
		//should also join a thread
		//but that would require checking for process ID in a thread
		//and that would require making it atomic and global
		if (get_process_ID(exe_name) == 0)
			break;
	}
}