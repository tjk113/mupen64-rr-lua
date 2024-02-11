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

namespace MovieDialog {
	t_record_params record_params;
	bool is_readonly;

	LRESULT CALLBACK MovieInspectorProc(HWND hwnd, UINT Message, WPARAM wParam,
										LPARAM lParam) {
		switch (Message) {
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

				SetDlgItemText(hwnd, IDC_INI_MOVIEFILE,
							   record_params.path.string().c_str());


				// workaround because initial selected button is "Start"
				SetFocus(GetDlgItem(hwnd, IDC_INI_AUTHOR));

				return FALSE;
			}
			case WM_CLOSE:
				EndDialog(hwnd, IDCANCEL);
				break;
			case WM_COMMAND:
				switch (LOWORD(wParam)) {
					case IDC_OK:
					case IDOK:
					{
						char text[MAX_PATH] = {0};
						GetDlgItemText(hwnd, IDC_PAUSEAT_FIELD, text, std::size(text));
						if (strlen(text) == 0) {
							record_params.pause_at = -1;
						} else {
							record_params.pause_at = std::atoi(text);
						}
						record_params.pause_at_last = IsDlgButtonChecked(hwnd, IDC_PAUSE_AT_END);

						Config.last_movie_type = record_params.start_flag;

						if (record_params.start_flag ==
							MOVIE_START_FROM_EXISTING_SNAPSHOT) {
							// The default directory we open the file dialog window in is the
							// parent directory of the last savestate that the user saved or loaded
							std::string path = wstring_to_string(
								show_persistent_open_dialog(
									"o_movie_existing_snapshot", hwnd,
									L"*.st;*.savestate"));

							if (path.empty()) {
								break;
							}

							st_path = path;
							st_medium = e_st_medium::path;

							std::filesystem::path movie_path = strip_extension(path)
								+ ".m64";

							if (exists(movie_path)) {
								if (MessageBox(
									hwnd, std::format(
										"{} already exists. Are you sure want to overwrite this movie?",
										movie_path.string()).c_str(), "VCR",
									MB_YESNO) ==
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
					case IDC_INI_MOVIEFILE:
					{
						char path[MAX_PATH] = {0};
						GetDlgItemText(hwnd, IDC_INI_MOVIEFILE, path,
									   std::size(path));
						record_params.path = path;

						goto refresh;
						break;
					}
					case IDC_INI_AUTHOR:
					{
						char author[MOVIE_AUTHOR_DATA_SIZE] = {0};
						GetDlgItemTextA(hwnd, IDC_INI_AUTHOR, author,
										std::size(author));
						record_params.author = author;
						break;
					}
					case IDC_INI_DESCRIPTION:
					{
						char description[MOVIE_DESCRIPTION_DATA_SIZE] = {0};
						GetDlgItemTextA(hwnd, IDC_INI_DESCRIPTION, description,
										std::size(description));
						record_params.description = description;
						break;
					}
					case IDC_MOVIE_BROWSE:
					{
						std::wstring path;
						if (is_readonly) {
							path = show_persistent_open_dialog(
								"o_movie", hwnd, L"*.m64;*.rec");
						} else {
							path = show_persistent_save_dialog(
								"s_movie", hwnd, L"*.m64;*.rec");
						}

						if (path.size() == 0) {
							break;
						}

						SetDlgItemText(hwnd, IDC_INI_MOVIEFILE,
									   wstring_to_string(path).c_str());
					}
					break;
					case IDC_RADIO_FROM_EEPROM:
						EnableWindow(GetDlgItem(hwnd, IDC_MOVIE_BROWSE), 1);
						EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE), 1);
						EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE_TEXT), 1);
						record_params.start_flag = MOVIE_START_FROM_EEPROM;
						break;
					case IDC_RADIO_FROM_ST:
						EnableWindow(GetDlgItem(hwnd, IDC_MOVIE_BROWSE), 1);
						EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE), 1);
						EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE_TEXT), 1);
						record_params.start_flag = MOVIE_START_FROM_SNAPSHOT;
						break;
					case IDC_RADIO_FROM_EXISTING_ST:
						EnableWindow(GetDlgItem(hwnd, IDC_MOVIE_BROWSE), 0);
						EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE), 0);
						EnableWindow(GetDlgItem(hwnd, IDC_INI_MOVIEFILE_TEXT), 0);
						record_params.start_flag = MOVIE_START_FROM_EXISTING_SNAPSHOT;
						break;
					case IDC_RADIO_FROM_START:
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

	refresh:
		char tempbuf[260] = {0};
		t_movie_header m_header = vcr_get_header_info(record_params.path.string().c_str());

		SetDlgItemText(hwnd, IDC_ROM_INTERNAL_NAME, m_header.rom_name);

		SetDlgItemText(hwnd, IDC_ROM_COUNTRY,
					   country_code_to_country_name(m_header.rom_country).
					   c_str());

		sprintf(tempbuf, "%X", (unsigned int)m_header.rom_crc1);
		SetDlgItemText(hwnd, IDC_ROM_CRC, tempbuf);

		SetDlgItemText(hwnd, IDC_MOVIE_VIDEO_TEXT, m_header.video_plugin_name);
		SetDlgItemText(hwnd, IDC_MOVIE_INPUT_TEXT, m_header.input_plugin_name);
		SetDlgItemText(hwnd, IDC_MOVIE_SOUND_TEXT, m_header.audio_plugin_name);
		SetDlgItemText(hwnd, IDC_MOVIE_RSP_TEXT, m_header.rsp_plugin_name);

		CheckDlgButton(hwnd, IDC_RADIO_FROM_ST,
							   m_header.startFlags ==
							   MOVIE_START_FROM_SNAPSHOT);
		CheckDlgButton(hwnd, IDC_RADIO_FROM_EXISTING_ST,
					   m_header.startFlags ==
					   MOVIE_START_FROM_EXISTING_SNAPSHOT);
		CheckDlgButton(hwnd, IDC_RADIO_FROM_START,
					   m_header.startFlags ==
					   MOVIE_START_FROM_NOTHING);
		CheckDlgButton(hwnd, IDC_RADIO_FROM_EEPROM,
					   m_header.startFlags ==
					   MOVIE_START_FROM_EEPROM);

		strcpy(tempbuf, (m_header.controller_flags & CONTROLLER_1_PRESENT)
							? "Present"
							: "Disconnected");
		if (m_header.controller_flags & CONTROLLER_1_MEMPAK)
			strcat(tempbuf, " with mempak");
		if (m_header.controller_flags & CONTROLLER_1_RUMBLE)
			strcat(tempbuf, " with rumble");
		SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER1_TEXT, tempbuf);

		strcpy(tempbuf, (m_header.controller_flags & CONTROLLER_2_PRESENT)
							? "Present"
							: "Disconnected");
		if (m_header.controller_flags & CONTROLLER_2_MEMPAK)
			strcat(tempbuf, " with mempak");
		if (m_header.controller_flags & CONTROLLER_2_RUMBLE)
			strcat(tempbuf, " with rumble");
		SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER2_TEXT, tempbuf);

		strcpy(tempbuf, (m_header.controller_flags & CONTROLLER_3_PRESENT)
							? "Present"
							: "Disconnected");
		if (m_header.controller_flags & CONTROLLER_3_MEMPAK)
			strcat(tempbuf, " with mempak");
		if (m_header.controller_flags & CONTROLLER_3_RUMBLE)
			strcat(tempbuf, " with rumble");
		SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER3_TEXT, tempbuf);

		strcpy(tempbuf, (m_header.controller_flags & CONTROLLER_4_PRESENT)
							? "Present"
							: "Disconnected");
		if (m_header.controller_flags & CONTROLLER_4_MEMPAK)
			strcat(tempbuf, " with mempak");
		if (m_header.controller_flags & CONTROLLER_4_RUMBLE)
			strcat(tempbuf, " with rumble");
		SetDlgItemText(hwnd, IDC_MOVIE_CONTROLLER4_TEXT, tempbuf);

		SetDlgItemText(hwnd, IDC_FROMSNAPSHOT_TEXT,
					   (m_header.startFlags & MOVIE_START_FROM_SNAPSHOT)
						   ? "Savestate"
						   : "Start");
		if (m_header.startFlags & MOVIE_START_FROM_EEPROM) {
			SetDlgItemTextA(hwnd, IDC_FROMSNAPSHOT_TEXT, "EEPROM");
		}

		sprintf(tempbuf, "%u  (%u input)", (int)m_header.length_vis,
				(int)m_header.length_samples);
		SetDlgItemText(hwnd, IDC_MOVIE_FRAMES, tempbuf);

		if (m_header.vis_per_second == 0)
			m_header.vis_per_second = 60;

		double seconds = (double)m_header.length_vis / (double)m_header.
			vis_per_second;
		double minutes = seconds / 60.0;
		if ((bool)seconds)
			seconds = fmod(seconds, 60.0);
		double hours = minutes / 60.0;
		if ((bool)minutes)
			minutes = fmod(minutes, 60.0);

		if (hours >= 1.0)
			sprintf(tempbuf, "%d hours and %.1f minutes", (unsigned int)hours,
					(float)minutes);
		else if (minutes >= 1.0)
			sprintf(tempbuf, "%d minutes and %.0f seconds",
					(unsigned int)minutes,
					(float)seconds);
		else if (m_header.length_vis != 0)
			sprintf(tempbuf, "%.1f seconds", (float)seconds);
		else
			strcpy(tempbuf, "0 seconds");
		SetDlgItemText(hwnd, IDC_MOVIE_LENGTH, tempbuf);

		sprintf(tempbuf, "%lu", m_header.rerecord_count);
		SetDlgItemText(hwnd, IDC_MOVIE_RERECORDS, tempbuf);

		{
			// convert utf8 metadata to windows widechar
			WCHAR wszMeta[MOVIE_MAX_METADATA_SIZE];
			if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
				m_header.author, -1, wszMeta,
				MOVIE_AUTHOR_DATA_SIZE)) {
				SetLastError(0);
				SetWindowTextW(GetDlgItem(hwnd, IDC_INI_AUTHOR), wszMeta);
				if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) {
					// not implemented on this system - convert as best we can to 1-byte characters and set with that
					// TODO: load unicows.dll instead so SetWindowTextW won't fail even on Win98/ME
					char ansiStr[MOVIE_AUTHOR_DATA_SIZE];
					WideCharToMultiByte(CP_ACP, 0, wszMeta, -1, ansiStr,
										MOVIE_AUTHOR_DATA_SIZE, NULL, NULL);
					SetWindowTextA(GetDlgItem(hwnd, IDC_INI_AUTHOR), ansiStr);

					if (ansiStr[0] == '\0')
						SetWindowTextA(GetDlgItem(hwnd, IDC_INI_AUTHOR),
									   "(too lazy to type name)");

					SetLastError(0);
				} else {
					if (wszMeta[0] == '\0')
						SetWindowTextW(GetDlgItem(hwnd, IDC_INI_AUTHOR),
									   L"(too lazy to type name)");
				}
			}
			if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
				m_header.description, -1, wszMeta,
				MOVIE_DESCRIPTION_DATA_SIZE)) {
				SetWindowTextW(GetDlgItem(hwnd, IDC_INI_DESCRIPTION), wszMeta);
				if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) {
					char ansiStr[MOVIE_DESCRIPTION_DATA_SIZE];
					WideCharToMultiByte(CP_ACP, 0, wszMeta, -1, ansiStr,
										MOVIE_DESCRIPTION_DATA_SIZE, NULL,
										NULL);
					SetWindowTextA(GetDlgItem(hwnd, IDC_INI_DESCRIPTION),
								   ansiStr);

					if (ansiStr[0] == '\0')
						SetWindowTextA(GetDlgItem(hwnd, IDC_INI_DESCRIPTION),
									   "(no description entered)");

					SetLastError(0);
				} else {
					if (wszMeta[0] == '\0')
						SetWindowTextW(GetDlgItem(hwnd, IDC_INI_DESCRIPTION),
									   L"(no description entered)");
				}
			}
		}

		return FALSE;
	}

	t_record_params show(bool readonly) {
		is_readonly = readonly;
		record_params.path = std::format("{} ({}).m64", (char*)ROM_HEADER.nom,
										 country_code_to_country_name(
											 ROM_HEADER.Country_code));
		record_params.start_flag = Config.last_movie_type;
		record_params.author = Config.last_movie_author;
		record_params.description = "";

		DialogBox(GetModuleHandle(NULL),
				  MAKEINTRESOURCE(IDD_MOVIE_PLAYBACK_DIALOG), mainHWND,
				  (DLGPROC)MovieInspectorProc);

		return record_params;
	}
}
