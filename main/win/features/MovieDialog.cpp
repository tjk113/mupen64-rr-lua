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
		switch (Message)
		{
		case WM_INITDIALOG:
			{
				SendMessage(GetDlgItem(hwnd, IDC_INI_DESCRIPTION),
				            EM_SETLIMITTEXT,
				            MOVIE_DESCRIPTION_DATA_SIZE, 0);
				SendMessage(GetDlgItem(hwnd, IDC_INI_AUTHOR), EM_SETLIMITTEXT,
				            MOVIE_AUTHOR_DATA_SIZE,
				            0);

				SetDlgItemText(hwnd, IDC_INI_AUTHOR,
				               Config.last_movie_author.c_str());
				SetDlgItemText(hwnd, IDC_INI_DESCRIPTION, "");

				CheckDlgButton(hwnd, IDC_FROMSNAPSHOT_RADIO,
				               record_params.start_flag ==
				               MOVIE_START_FROM_SNAPSHOT);
				CheckDlgButton(hwnd, IDC_FROMEXISTINGSNAPSHOT_RADIO,
				               record_params.start_flag ==
				               MOVIE_START_FROM_EXISTING_SNAPSHOT);
				CheckDlgButton(hwnd, IDC_FROMSTART_RADIO,
				               record_params.start_flag ==
				               MOVIE_START_FROM_NOTHING);
				CheckDlgButton(hwnd, IDC_FROMEEPROM_RADIO,
				               record_params.start_flag ==
				               MOVIE_START_FROM_EEPROM);

				SetDlgItemText(hwnd, IDC_INI_MOVIEFILE,
				               record_params.path.string().c_str());

				SetDlgItemText(hwnd, IDC_ROM_INTERNAL_NAME2,
				               (CHAR*)ROM_HEADER.nom);

				SetDlgItemText(hwnd, IDC_ROM_COUNTRY2,
				               country_code_to_country_name(
					               ROM_HEADER.Country_code)
				               .c_str());

				char crc_buf[260] = {0};
				sprintf(crc_buf, "%X", (unsigned int)ROM_HEADER.CRC1);
				SetDlgItemText(hwnd, IDC_ROM_CRC3, crc_buf);

				SetDlgItemText(hwnd, IDC_MOVIE_VIDEO_TEXT2,
				               video_plugin->name.c_str());
				SetDlgItemText(hwnd, IDC_MOVIE_INPUT_TEXT2,
				               input_plugin->name.c_str());
				SetDlgItemText(hwnd, IDC_MOVIE_SOUND_TEXT2,
				               audio_plugin->name.c_str());
				SetDlgItemText(hwnd, IDC_MOVIE_RSP_TEXT2,
				               rsp_plugin->name.c_str());

				for (int i = 0; i < 4; ++i)
				{
					std::string str = Controls[i].Present
						                  ? "Present"
						                  : "Disconnected";
					if (Controls[i].Present && Controls[i].Plugin == mempak)
						str += " with mempak";
					if (Controls[i].Present && Controls[i].Plugin == rumblepak)
						str += " with rumble";
					SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER1_TEXT2 + i,
					               str.c_str());
				}

				// workaround because initial selected button is "Start"
				SetFocus(GetDlgItem(hwnd, IDC_INI_AUTHOR));

				return FALSE;
			}
		case WM_CLOSE:
			EndDialog(hwnd, IDOK);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDC_OK:
			case IDOK:
				{
					char author[MOVIE_AUTHOR_DATA_SIZE] = {0};
					GetDlgItemTextA(hwnd, IDC_INI_AUTHOR, author,
					                std::size(author));
					record_params.author = author;

					char description[MOVIE_DESCRIPTION_DATA_SIZE] = {0};
					GetDlgItemTextA(hwnd, IDC_INI_DESCRIPTION, description,
					                std::size(description));
					record_params.description = description;

					char path[MAX_PATH] = {0};
					GetDlgItemText(hwnd, IDC_INI_MOVIEFILE, path,
					               std::size(path));
					record_params.path = path;

					Config.last_movie_type = record_params.start_flag;

					if (record_params.start_flag ==
						MOVIE_START_FROM_EXISTING_SNAPSHOT)
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

						std::filesystem::path movie_path = strip_extension(path)
							+ ".m64";

						if (exists(movie_path))
						{
							if (MessageBox(
									hwnd, std::format(
										"{} already exists. Are you sure want to overwrite this movie?",
										movie_path.string()).c_str(), "VCR", MB_YESNO) ==
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
				record_params.start_flag = MOVIE_START_FROM_EEPROM;
				break;
			case IDC_FROMSNAPSHOT_RADIO:
				EnableWindow(GetDlgItem(hwnd, IDC_MOVIE_BROWSE), 1);
				EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE), 1);
				EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE_TEXT), 1);
				record_params.start_flag = MOVIE_START_FROM_SNAPSHOT;
				break;
			case IDC_FROMEXISTINGSNAPSHOT_RADIO:
				EnableWindow(GetDlgItem(hwnd, IDC_MOVIE_BROWSE), 0);
				EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE), 0);
				EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE_TEXT), 0);
				record_params.start_flag = MOVIE_START_FROM_EXISTING_SNAPSHOT;
				break;
			case IDC_FROMSTART_RADIO:
				EnableWindow(GetDlgItem(hwnd, IDC_MOVIE_BROWSE), 1);
				EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE), 1);
				EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE_TEXT), 1);
				record_params.start_flag = MOVIE_START_FROM_NOTHING;
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
		record_params.path = std::format("{} ({}).m64", (char*)ROM_HEADER.nom,
		                                 country_code_to_country_name(
			                                 ROM_HEADER.Country_code));
		record_params.start_flag = Config.last_movie_type;
		record_params.author = Config.last_movie_author;
		record_params.description = "";

		DialogBox(GetModuleHandle(NULL),
		          MAKEINTRESOURCE(IDD_MOVIE_RECORD_DIALOG), mainHWND,
		          (DLGPROC)RecordMovieProc);

		return record_params;
	}
}
