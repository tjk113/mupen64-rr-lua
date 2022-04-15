#pragma once

// le cringe header inclusion
#include <shlobj.h>
#include <stdio.h>


class ReassociatingFileDialog
{
public:
	/// <summary>
	/// Shows file dialog with specified flags
	/// </summary>
	/// <param name="hWnd">Handle to parent window</param>
	/// <param name="path">Pointer to MAX_PATH-long ASCII string</param>
	/// <param name="fTypes">Allowed file types in format: *.extension1;*.extension2 ...</param>
	/// <param name="allowMultiSelect">Whether the file dialog allows multiple files to be selected</param>
	bool ShowFileDialog(char* path, const wchar_t* fTypes, bool openDialog, bool allowMultiSelect = FALSE, HWND hWnd = NULL);
private:
	wchar_t lastPath[MAX_PATH]; // type-flattening PWSTR -> wchar_t*
								// dont think about it
};


