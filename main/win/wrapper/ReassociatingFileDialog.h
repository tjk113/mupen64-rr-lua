#pragma once

// le cringe header inclusion
#include <shlobj.h>
#include <stdio.h>
#include <filesystem>

class ReassociatingFileDialog {
public:
	/// <summary>
	/// Shows a file dialog with the specified flags
	/// </summary>
	/// <param name="hWnd">Handle to parent window</param>
	/// <param name="path">Pointer to MAX_PATH-long ASCII string</param>
	/// <param name="fTypes">Allowed file types in format: *.extension1;*.extension2 ...</param>
	/// <param name="allowMultiSelect">Whether the file dialog allows multiple files to be selected</param>
	bool ShowFileDialog(char* path, const wchar_t* fTypes, bool openDialog, bool allowMultiSelect = FALSE, HWND hWnd = NULL);

	/// <summary>
	/// Shows a folder dialog with the specified flags
	/// </summary>
	/// <param name="path">Pointer to <paramref name="maxSize"/>-long ASCII string</param>
	/// <param name="maxSize">The length of the output <paramref name="path"/></param>
	/// <param name="hWnd"></param>
	/// <returns></returns>
	bool ShowFolderDialog(char* path, int maxSize, HWND hWnd = NULL);

	ReassociatingFileDialog() {
		// set lastPath to mupen64.exe directory by default
		char tmpArr[MAX_PATH];
		GetModuleFileName(NULL, tmpArr, _MAX_PATH);
		std::filesystem::path tmpPath = tmpArr;
		strncpy(tmpArr, tmpPath.parent_path().string().c_str(), MAX_PATH);
		wcscpy(m_lastPathWide, convertCharArrayToLPCWSTR(tmpArr));
		strcpy(m_lastPathShort, tmpArr);
	}

private:
	wchar_t m_lastPathWide[MAX_PATH]; // type-flattening PWSTR -> wchar_t*
	char m_lastPathShort[MAX_PATH];

	wchar_t* convertCharArrayToLPCWSTR(const char* charArray);
};

char* GetDefaultFileDialogPath(char* buf, char* path);