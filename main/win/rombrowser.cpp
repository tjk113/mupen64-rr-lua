#include "RomBrowser.hpp"
#include "main_win.h"
#include "rom.h"
#include "../../winproject/resource.h"
#include "helpers/io_helpers.h"
#include "helpers/string_helpers.h"
#include <algorithm>
#include <fstream>
#include <iterator>
#include <vector>

HWND rombrowser_hwnd = nullptr;
std::vector<t_rombrowser_entry*> rombrowser_entries;

void rombrowser_create()
{
	if (rombrowser_hwnd)
	{
		DestroyWindow(rombrowser_hwnd);
	}

	RECT rcl, rtool, rstatus;
	GetClientRect(mainHWND, &rcl);
	GetWindowRect(hTool, &rtool);
	GetWindowRect(hStatus, &rstatus);

	rombrowser_hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, NULL,
	                                 WS_TABSTOP | WS_VISIBLE | WS_CHILD |
	                                 LVS_SINGLESEL | LVS_REPORT |
	                                 LVS_SHOWSELALWAYS,
	                                 0, rtool.bottom - rtool.top,
	                                 rcl.right - rcl.left,
	                                 rcl.bottom - rcl.top - rtool.bottom + rtool
	                                 .top - rstatus.bottom + rstatus.top,
	                                 mainHWND, (HMENU)IDC_ROMLIST,
	                                 app_hInstance,
	                                 NULL);

	ListView_SetExtendedListViewStyle(rombrowser_hwnd,
	                                  LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT |
	                                  LVS_EX_DOUBLEBUFFER);

	HIMAGELIST hSmall =
		ImageList_Create(16, 16, ILC_COLORDDB | ILC_MASK, 11, 0);
	HICON hIcon;

	hIcon = LoadIcon(app_hInstance, MAKEINTRESOURCE(IDI_GERMANY));
	ImageList_AddIcon(hSmall, hIcon);
	hIcon = LoadIcon(app_hInstance, MAKEINTRESOURCE(IDI_USA));
	ImageList_AddIcon(hSmall, hIcon);
	hIcon = LoadIcon(app_hInstance, MAKEINTRESOURCE(IDI_JAPAN));
	ImageList_AddIcon(hSmall, hIcon);
	hIcon = LoadIcon(app_hInstance, MAKEINTRESOURCE(IDI_EUROPE));
	ImageList_AddIcon(hSmall, hIcon);
	hIcon = LoadIcon(app_hInstance, MAKEINTRESOURCE(IDI_AUSTRALIA));
	ImageList_AddIcon(hSmall, hIcon);
	hIcon = LoadIcon(app_hInstance, MAKEINTRESOURCE(IDI_ITALIA));
	ImageList_AddIcon(hSmall, hIcon);
	hIcon = LoadIcon(app_hInstance, MAKEINTRESOURCE(IDI_FRANCE));
	ImageList_AddIcon(hSmall, hIcon);
	hIcon = LoadIcon(app_hInstance, MAKEINTRESOURCE(IDI_SPAIN));
	ImageList_AddIcon(hSmall, hIcon);
	hIcon = LoadIcon(app_hInstance, MAKEINTRESOURCE(IDI_UNKNOWN));
	ImageList_AddIcon(hSmall, hIcon);
	hIcon = LoadIcon(app_hInstance, MAKEINTRESOURCE(IDI_DEMO));
	ImageList_AddIcon(hSmall, hIcon);
	hIcon = LoadIcon(app_hInstance, MAKEINTRESOURCE(IDI_BETA));
	ImageList_AddIcon(hSmall, hIcon);
	ListView_SetImageList(rombrowser_hwnd, hSmall, LVSIL_SMALL);

	LVCOLUMN lv_column = {0};

	lv_column.mask = LVCF_WIDTH;
	lv_column.iImage = 1;
	lv_column.cx = 24;
	ListView_InsertColumn(rombrowser_hwnd, 0, &lv_column);

	lv_column.mask = LVCF_TEXT | LVCF_WIDTH;
	lv_column.pszText = (LPTSTR)"Name";
	lv_column.cx = 240;
	ListView_InsertColumn(rombrowser_hwnd, 1, &lv_column);

	lv_column.mask = LVCF_TEXT | LVCF_WIDTH;
	lv_column.pszText = (LPTSTR)"Filename";
	lv_column.cx = 240;
	ListView_InsertColumn(rombrowser_hwnd, 2, &lv_column);

	lv_column.mask = LVCF_TEXT | LVCF_WIDTH;
	lv_column.pszText = (LPTSTR)"Size";
	lv_column.cx = 120;
	ListView_InsertColumn(rombrowser_hwnd, 3, &lv_column);

	BringWindowToTop(rombrowser_hwnd);
}

int32_t rombrowser_country_code_to_image_index(uint16_t country_code)
{
	switch (country_code & 0xFF)
	{
	case 0:
		return 9;
	case '7':
		return 10;
	case 0x44:
		return 0; // IDI_GERMANY
	case 0x45:
		return 1; // IDI_USA
	case 0x4A:
		return 2; // IDI_JAPAN
	case 0x20:
	case 0x21:
	case 0x38:
	case 0x70:
	case 0x50:
	case 0x58:
		return 3; // IDI_EUROPE
	case 0x55:
		return 4; // IDI_AUSTRALIA
	case 'I':
		return 5; // Italy
	case 0x46: // IDI_FRANCE
		return 6;
	case 'S': //SPAIN
		return 7;
	default:
		return 8; // IDI_N64CART
	}
}

void rombrowser_add_rom(int32_t index, t_rombrowser_entry* rombrowser_entry)
{
	LV_ITEM lv_item = {0};

	lv_item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
	lv_item.pszText = LPSTR_TEXTCALLBACK;
	lv_item.stateMask = 0;
	lv_item.iSubItem = 0;
	lv_item.state = 0;
	lv_item.iItem = index;
	lv_item.iImage = rombrowser_country_code_to_image_index(
		rombrowser_entry->rom_header.Country_code);
	ListView_InsertItem(rombrowser_hwnd, &lv_item);
	rombrowser_entries.push_back(rombrowser_entry);
}

void rombrowser_build()
{
	ListView_DeleteAllItems(rombrowser_hwnd);
	for (auto entry : rombrowser_entries)
	{
		delete entry;
	}
	rombrowser_entries.clear();

	std::vector<std::string> rom_paths;

	// we aggregate all file paths and only filter them after we're done
	if (Config.is_rombrowser_recursion_enabled)
	{
		for (auto& path : Config.rombrowser_rom_paths)
		{
			auto file_paths = get_files_in_subdirectories(path + "\\");
			rom_paths.insert(rom_paths.end(), file_paths.begin(),
			                 file_paths.end());
		}
	} else
	{
		for (auto& path : Config.rombrowser_rom_paths)
		{
			auto file_paths = get_files_with_extension_in_directory(
				path + "\\", "*");
			rom_paths.insert(rom_paths.end(), file_paths.begin(),
			                 file_paths.end());
		}
	}

	std::vector<std::string> filtered_rom_paths;

	std::copy_if(rom_paths.begin(), rom_paths.end(),
	             std::back_inserter(filtered_rom_paths), [](std::string val)
	             {
		             char c_extension[260] = {0};
		             _splitpath(val.c_str(), NULL, NULL, NULL, c_extension);

		             auto extension = std::string(c_extension);
		             return is_case_insensitive_equal(extension, ".z64") ||
			             is_case_insensitive_equal(extension, ".n64") ||
			             is_case_insensitive_equal(
				             extension, ".v64")
			             || is_case_insensitive_equal(extension, ".rom");
	             });

	int32_t i = 0;
	for (auto& path : filtered_rom_paths)
	{
		FILE* f = fopen(path.c_str(), "rb");

		fseek(f, 0, SEEK_END);
		uint64_t len = ftell(f);
		fseek(f, 0, SEEK_SET);

		auto buffer = (uint8_t*)malloc(len);

		fread(buffer, sizeof(uint8_t), len / sizeof(uint8_t), f);

		if (len > sizeof(t_rom_header))
		{
			t_rom_header rom_header = {0};
			rom_byteswap(buffer);
			memcpy(&rom_header, buffer, sizeof(rom_header));

			auto rombrowser_entry = new t_rombrowser_entry;
			rombrowser_entry->rom_header = rom_header;
			char filename[MAX_PATH] = {0};
			_splitpath(path.c_str(), NULL, NULL,
			           filename, NULL);
			rombrowser_entry->path = path;
			rombrowser_entry->filename = std::string(filename);
			rombrowser_entry->size = std::to_string(len / (1024 * 1024)) +
				" MB";

			rombrowser_add_rom(i, rombrowser_entry);
		}

		free(buffer);
		fclose(f);

		i++;
	}
}

void rombrowser_set_visibility(int32_t is_visible)
{
	ShowWindow(rombrowser_hwnd, is_visible ? SW_SHOW : SW_HIDE);
}

void rombrowser_update_size()
{
	if (emu_launched) return;

	if (!IsWindow(rombrowser_hwnd))
		return;

	RECT rc, rc_main;
	WORD n_width, n_height;
	int32_t y = 0;
	GetClientRect(mainHWND, &rc_main);
	n_width = rc_main.right - rc_main.left;
	n_height = rc_main.bottom - rc_main.top;
	if (IsWindow(hStatus))
	{
		GetWindowRect(hStatus, &rc);
		n_height -= (WORD)(rc.bottom - rc.top);
	}
	if (IsWindow(hTool))
	{
		GetWindowRect(hTool, &rc);
		y += (WORD)(rc.bottom - rc.top);
	}
	MoveWindow(rombrowser_hwnd, 0, y, n_width, n_height - y, TRUE);
}

void rombrowser_notify(LPARAM lparam)
{
	switch (((LPNMHDR)lparam)->code)
	{
	case LVN_GETDISPINFO:
		{
			NMLVDISPINFO* plvdi = (NMLVDISPINFO*)lparam;
			t_rombrowser_entry* rombrowser_entry = rombrowser_entries[plvdi->
				item.iItem];
			switch (plvdi->item.iSubItem)
			{
			case 1:
				plvdi->item.pszText = (LPSTR)rombrowser_entry->rom_header.nom;
				break;
			case 2:
				plvdi->item.pszText = (LPSTR)rombrowser_entry->filename.c_str();
				break;
			case 3:
				plvdi->item.pszText = (LPSTR)rombrowser_entry->size.c_str();
				break;
			default:
				break;
			}

			break;
		}
		break;
	case NM_DBLCLK:
		{
			int i = ListView_GetNextItem(rombrowser_hwnd, -1, LVNI_SELECTED);
			if (i != -1)
			{
				StartRom(rombrowser_entries[i]->path.c_str());
			}
		}
		break;
	}
}
