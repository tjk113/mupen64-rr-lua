#include "MovieDialog.h"
#include <Windows.h>

#include "rom.h"
#include "savestates.h"
#include "Statusbar.hpp"
#include "vcr.h"
#include "win/main_win.h"
#include "win/Config.hpp"
#include "../winproject/resource.h"
#include "helpers/string_helpers.h"
#include "win/wrapper/PersistentPathDialog.h"

namespace MovieDialog
{
	t_record_params record_params;

	LRESULT CALLBACK RecordMovieProc(HWND hwnd, UINT Message, WPARAM wParam,
	                                 LPARAM lParam)
	{
		char tempbuf[MAX_PATH];
		char tempbuf2[MAX_PATH];
		HWND descriptionDialog;
		HWND authorDialog;

		switch (Message)
		{
		case WM_INITDIALOG:
			descriptionDialog = GetDlgItem(hwnd, IDC_INI_DESCRIPTION);
			authorDialog = GetDlgItem(hwnd, IDC_INI_AUTHOR);

			SendMessage(descriptionDialog, EM_SETLIMITTEXT,
			            MOVIE_DESCRIPTION_DATA_SIZE, 0);
			SendMessage(authorDialog, EM_SETLIMITTEXT, MOVIE_AUTHOR_DATA_SIZE,
			            0);

			SetDlgItemText(hwnd, IDC_INI_AUTHOR,
			               Config.last_movie_author.c_str());
			SetDlgItemText(hwnd, IDC_INI_DESCRIPTION, "");

			CheckRadioButton(hwnd, IDC_FROMSNAPSHOT_RADIO, IDC_FROMSTART_RADIO, record_params.start_flag);

			sprintf(tempbuf, "%s (%s)", (char*)ROM_HEADER.nom,
			        country_code_to_country_name(ROM_HEADER.Country_code).
			        c_str());
			strcat(tempbuf, ".m64");
			SetDlgItemText(hwnd, IDC_INI_MOVIEFILE, tempbuf);

			SetDlgItemText(hwnd, IDC_ROM_INTERNAL_NAME2, (CHAR*)ROM_HEADER.nom);

			SetDlgItemText(hwnd, IDC_ROM_COUNTRY2,
			               country_code_to_country_name(ROM_HEADER.Country_code)
			               .c_str());

			sprintf(tempbuf, "%X", (unsigned int)ROM_HEADER.CRC1);
			SetDlgItemText(hwnd, IDC_ROM_CRC3, tempbuf);

			SetDlgItemText(hwnd, IDC_MOVIE_VIDEO_TEXT2,
			               video_plugin->name.c_str());
			SetDlgItemText(hwnd, IDC_MOVIE_INPUT_TEXT2,
			               input_plugin->name.c_str());
			SetDlgItemText(hwnd, IDC_MOVIE_SOUND_TEXT2,
			               audio_plugin->name.c_str());
			SetDlgItemText(hwnd, IDC_MOVIE_RSP_TEXT2,
			               rsp_plugin->name.c_str());

			strcpy(tempbuf, Controls[0].Present ? "Present" : "Disconnected");
			if (Controls[0].Present && Controls[0].Plugin ==
				controller_extension::mempak)
				strcat(tempbuf, " with mempak");
			if (Controls[0].Present && Controls[0].Plugin ==
				controller_extension::rumblepak)
				strcat(tempbuf, " with rumble");
			SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER1_TEXT2, tempbuf);

			strcpy(tempbuf, Controls[1].Present ? "Present" : "Disconnected");
			if (Controls[1].Present && Controls[1].Plugin ==
				controller_extension::mempak)
				strcat(tempbuf, " with mempak");
			if (Controls[1].Present && Controls[1].Plugin ==
				controller_extension::rumblepak)
				strcat(tempbuf, " with rumble pak");
			SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER2_TEXT2, tempbuf);

			strcpy(tempbuf, Controls[2].Present ? "Present" : "Disconnected");
			if (Controls[2].Present && Controls[2].Plugin ==
				controller_extension::mempak)
				strcat(tempbuf, " with mempak");
			if (Controls[2].Present && Controls[2].Plugin ==
				controller_extension::rumblepak)
				strcat(tempbuf, " with rumble pak");
			SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER3_TEXT2, tempbuf);

			strcpy(tempbuf, Controls[3].Present ? "Present" : "Disconnected");
			if (Controls[3].Present && Controls[3].Plugin ==
				controller_extension::mempak)
				strcat(tempbuf, " with mempak");
			if (Controls[3].Present && Controls[3].Plugin ==
				controller_extension::rumblepak)
				strcat(tempbuf, " with rumble pak");
			SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER4_TEXT2, tempbuf);

		// workaround because initial selected button is "Start"

			SetFocus(GetDlgItem(hwnd, IDC_INI_AUTHOR));

			return FALSE;
		case WM_CLOSE:
			EndDialog(hwnd, IDOK);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDC_OK:
			case IDOK:
				{
					// turn WCHAR into UTF8
					WCHAR authorWC[MOVIE_AUTHOR_DATA_SIZE];
					char authorUTF8[MOVIE_AUTHOR_DATA_SIZE * 4];
					if (GetDlgItemTextW(hwnd, IDC_INI_AUTHOR, authorWC,
					                    MOVIE_AUTHOR_DATA_SIZE))
						WideCharToMultiByte(
							CP_UTF8, 0, authorWC, -1, authorUTF8,
							sizeof(authorUTF8), NULL, NULL);
					else
						GetDlgItemTextA(hwnd, IDC_INI_AUTHOR, authorUTF8,
						                MOVIE_AUTHOR_DATA_SIZE);

					Config.last_movie_author = std::string(authorUTF8);

					WCHAR descriptionWC[MOVIE_DESCRIPTION_DATA_SIZE];
					char descriptionUTF8[MOVIE_DESCRIPTION_DATA_SIZE * 4];
					if (GetDlgItemTextW(hwnd, IDC_INI_DESCRIPTION,
					                    descriptionWC,
					                    MOVIE_DESCRIPTION_DATA_SIZE))
						WideCharToMultiByte(CP_UTF8, 0, descriptionWC, -1,
						                    descriptionUTF8,
						                    sizeof(descriptionUTF8), NULL,
						                    NULL);
					else
						GetDlgItemTextA(hwnd, IDC_INI_DESCRIPTION,
						                descriptionUTF8,
						                MOVIE_DESCRIPTION_DATA_SIZE);


					GetDlgItemText(hwnd, IDC_INI_MOVIEFILE, tempbuf, MAX_PATH);

					// big
					record_params.start_flag =
						IsDlgButtonChecked(hwnd, IDC_FROMSNAPSHOT_RADIO)
							? IDC_FROMSNAPSHOT_RADIO
							: IsDlgButtonChecked(hwnd, IDC_FROMSTART_RADIO)
							? IDC_FROMSTART_RADIO
							: IsDlgButtonChecked(hwnd, IDC_FROMEEPROM_RADIO)
							? IDC_FROMEEPROM_RADIO
							: IDC_FROMEXISTINGSNAPSHOT_RADIO;

					unsigned short flag = record_params.start_flag ==
					                      IDC_FROMSNAPSHOT_RADIO
						                      ? MOVIE_START_FROM_SNAPSHOT
						                      : record_params.start_flag ==
						                      IDC_FROMSTART_RADIO
						                      ? MOVIE_START_FROM_NOTHING
						                      : record_params.start_flag ==
						                      IDC_FROMEEPROM_RADIO
						                      ? MOVIE_START_FROM_EEPROM
						                      : MOVIE_START_FROM_EXISTING_SNAPSHOT;

					Config.last_movie_type = record_params.start_flag;

					if (flag == MOVIE_START_FROM_EXISTING_SNAPSHOT)
					{
						// The default directory we open the file dialog window in is the
						// parent directory of the last savestate that the user saved or loaded
						std::string path = wstring_to_string(
							show_persistent_open_dialog(
								"o_movie_existing_snapshot", hwnd,
								L"*.st;*.savestate"));

						if (path.empty())
						{
							break;
						}

						st_path = path;
						st_medium = e_st_medium::path;

						std::string movie_path = strip_extension(path) + ".m64";

						strcpy(tempbuf, movie_path.c_str());

						if (std::filesystem::exists(movie_path))
						{
							sprintf(tempbuf2,
							        "\"%s\" already exists. Are you sure want to overwrite this movie?",
							        tempbuf);
							if (MessageBox(hwnd, tempbuf2, "VCR", MB_YESNO) ==
								IDNO)
								break;
						}
					}

					EndDialog(hwnd, IDOK);
				}
				break;
			case IDC_CANCEL:
			case IDCANCEL:
				record_params.path.clear();
				EndDialog(hwnd, IDCANCEL);
				break;
			case IDC_MOVIE_BROWSE:
				{
					auto path = show_persistent_save_dialog(
						"s_movie", hwnd, L"*.m64;*.rec");

					if (path.size() == 0)
					{
						break;
					}

					SetDlgItemText(hwnd, IDC_INI_MOVIEFILE,
					               wstring_to_string(path).c_str());
				}
				break;

			case IDC_FROMEEPROM_RADIO:
				EnableWindow(GetDlgItem(hwnd, IDC_MOVIE_BROWSE), 1);
				EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE), 1);
				EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE_TEXT), 1);
				break;
			case IDC_FROMSNAPSHOT_RADIO:
				EnableWindow(GetDlgItem(hwnd, IDC_MOVIE_BROWSE), 1);
				EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE), 1);
				EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE_TEXT), 1);
				break;
			case IDC_FROMEXISTINGSNAPSHOT_RADIO:
				EnableWindow(GetDlgItem(hwnd, IDC_MOVIE_BROWSE), 0);
				EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE), 0);
				EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE_TEXT), 0);
				break;
			case IDC_FROMSTART_RADIO:
				EnableWindow(GetDlgItem(hwnd, IDC_MOVIE_BROWSE), 1);
				EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE), 1);
				EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE_TEXT), 1);
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
		return FALSE;
	}

	t_record_params show()
	{
		record_params.path = std::format("{} ({}).m64", (char*)ROM_HEADER.nom, country_code_to_country_name(ROM_HEADER.Country_code));
		record_params.start_flag = Config.last_movie_type;
		record_params.author = Config.last_movie_author;
		record_params.description = "";

		DialogBox(GetModuleHandle(NULL),
		          MAKEINTRESOURCE(IDD_MOVIE_RECORD_DIALOG), mainHWND,
		          (DLGPROC)RecordMovieProc);

		return record_params;
	}
}
