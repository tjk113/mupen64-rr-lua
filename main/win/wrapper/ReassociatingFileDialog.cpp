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
	
	// fighting with "almost-managed-but-not-quite-this-sucks" winapi
	// technically IFileOpenDialog/IFileSaveDialog should be used in case but they inherit from same base class and thus allow us to do this unsafe monstrosity
	if (openDialog) {
		FAILSAFE(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileDialog)));
	}
	else {
		FAILSAFE(CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileDialog))); // nope
	}
	FAILSAFE(pFileDialog->GetOptions(&dwFlags));
	FAILSAFE(pFileDialog->SetOptions(dwFlags | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST | (allowMultiSelect ? FOS_ALLOWMULTISELECT : 0)));
	COMDLG_FILTERSPEC fileTypes[] =
	{
		{ L"Supported files", fTypes },
	};
	FAILSAFE(pFileDialog->SetFileTypes(ARRAYSIZE(fileTypes), fileTypes));

	if (lastPath) {
		printf("found path \"%s\"\n", lastPath);
		IShellItem* shlPtr;
		SHCreateItemFromParsingName(lastPath, NULL, IID_PPV_ARGS(&shlPtr));
		pFileDialog->SetFolder(shlPtr);
	}

	FAILSAFE(pFileDialog->Show(hWnd));
	FAILSAFE(pFileDialog->GetResult(&pShellItem));
	FAILSAFE(pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &pFilePath));

	wcstombs(path, pFilePath, MAX_PATH);
	wcsncpy(lastPath, pFilePath, MAX_PATH);

cleanUp:
	if (pFilePath) CoTaskMemFree(pFilePath);
	if (pShellItem) pShellItem->Release();
	if (pFileDialog) pFileDialog->Release();

	return strlen(path) > 0;
}
