#include "ReassociatingFileDialog.h"
#include <shlobj.h>
#include <stdio.h>
#pragma comment(lib,"comctl32.lib") 
#pragma comment(lib,"propsys.lib")
#pragma comment(lib,"shlwapi.lib") 

bool ReassociatingFileDialog::ShowFileDialog(char* path, const wchar_t* fTypes, bool openDialog, bool allowMultiSelect, HWND hWnd) {

#define FAILSAFE(operation) if(FAILED(operation)) goto cleanUp

	IFileDialog* pFileDialog = nullptr;
	IShellItem* pShellItem = nullptr;
	PWSTR pFilePath = nullptr;
	DWORD dwFlags;
	bool succeeded = false;

	// fighting with "almost-managed-but-not-quite-this-sucks" winapi
	// technically IFileOpenDialog/IFileSaveDialog should be used in case but they inherit from same base class and thus allow us to do this unsafe monstrosity
	if (openDialog) {
		FAILSAFE(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileDialog)));
	}
	else {
		FAILSAFE(CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileDialog))); // nope
	}
	FAILSAFE(pFileDialog->GetOptions(&dwFlags));
	FAILSAFE(pFileDialog->SetOptions((allowMultiSelect ? FOS_ALLOWMULTISELECT : 0))); //dwFlags | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST | 
	// scoped bc initialization should be skipped if FAILSAFE
	{
		COMDLG_FILTERSPEC fileTypes[] =
		{
			{ L"Supported files", fTypes },
		};
		FAILSAFE(pFileDialog->SetFileTypes(ARRAYSIZE(fileTypes), fileTypes));
	}

	IShellItem* shlPtr;
	if (path[0] != NULL) {
		printf("[ReassociatingFileDialog]: found path \"%s\"\n", path);
		if (SHCreateItemFromParsingName(convertCharArrayToLPCWSTR(path), NULL, IID_PPV_ARGS(&shlPtr)) != S_OK)
			printf("[ReassociatingFileDialog]: Unable to create IShellItem from parsing path name");
	}
	else {
		printf("[ReassociatingFileDialog]: found lastPath \"%s\"\n", (char*)lastPath);
		if (SHCreateItemFromParsingName(lastPath, NULL, IID_PPV_ARGS(&shlPtr)) != S_OK)
			printf("[ReassociatingFileDialog]: Unable to create IShellItem from parsing lastPath name");
	}
	FAILSAFE(pFileDialog->SetFolder(shlPtr));

	FAILSAFE(pFileDialog->Show(hWnd));
	FAILSAFE(pFileDialog->GetResult(&pShellItem));
	FAILSAFE(pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &pFilePath));

	wcstombs(path, pFilePath, MAX_PATH);
	wcsncpy(lastPath, pFilePath, MAX_PATH);
	
	succeeded = true;

cleanUp:
	if (pFilePath) CoTaskMemFree(pFilePath);
	if (pShellItem) pShellItem->Release();
	if (pFileDialog) pFileDialog->Release();

	return succeeded && strlen(path) > 0;
}

// https://gist.github.com/jsxinvivo/11f383ac61a56c1c0c25
wchar_t* ReassociatingFileDialog::convertCharArrayToLPCWSTR(const char* charArray) {
	wchar_t* wString = new wchar_t[4096];
	MultiByteToWideChar(CP_ACP, 0, charArray, -1, wString, 4096);
	return wString;
}