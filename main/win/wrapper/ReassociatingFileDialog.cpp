#include "ReassociatingFileDialog.h"
#include <shlobj.h>
#include <stdio.h>

#include "helpers/string_helpers.h"
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"propsys.lib")
#pragma comment(lib,"shlwapi.lib")

bool ReassociatingFileDialog::ShowFileDialog(char* path, const wchar_t* fTypes,
                                             bool openDialog,
                                             bool allowMultiSelect, HWND hWnd)
{
#define FAILSAFE(operation) if(FAILED(operation)) goto cleanUp

	IFileDialog* pFileDialog = nullptr;
	IShellItem* pShellItem = nullptr;
	PWSTR pFilePath = nullptr;
	DWORD dwFlags;
	bool succeeded = false;

	// fighting with "almost-managed-but-not-quite-this-sucks" winapi
	// technically IFileOpenDialog/IFileSaveDialog should be used in case but they inherit from same base class and thus allow us to do this unsafe monstrosity
	if (openDialog)
	{
		FAILSAFE(
			CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER
				, IID_PPV_ARGS(&pFileDialog)));
	} else
	{
		FAILSAFE(
			CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER
				, IID_PPV_ARGS(&pFileDialog))); // nope
	}
	FAILSAFE(pFileDialog->GetOptions(&dwFlags));
	if (openDialog)
	{
		FAILSAFE(
		pFileDialog->SetOptions((allowMultiSelect ? FOS_ALLOWMULTISELECT : 0)));
	}

	//dwFlags | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST |
	// scoped bc initialization should be skipped if FAILSAFE
	{
		COMDLG_FILTERSPEC fileTypes[] =
		{
			{L"Supported files", fTypes},
		};
		FAILSAFE(pFileDialog->SetFileTypes(ARRAYSIZE(fileTypes), fileTypes));
	}

	IShellItem* shlPtr;
	if (path[0] != NULL)
	{
		printf("[ReassociatingFileDialog]: Defaulting to path \"%s\"...\n",
		       path);
		if (SHCreateItemFromParsingName(convertCharArrayToLPCWSTR(path), NULL,
		                                IID_PPV_ARGS(&shlPtr)) != S_OK)
			printf(
				"[ReassociatingFileDialog]: Unable to create IShellItem from parsing path name");
	} else
	{
		printf("[ReassociatingFileDialog]: Restoring path \"%ls\"...\n",
		       m_lastPathWide);
		if (SHCreateItemFromParsingName(m_lastPathWide, NULL,
		                                IID_PPV_ARGS(&shlPtr)) != S_OK)
			printf(
				"[ReassociatingFileDialog]: Unable to create IShellItem from parsing lastPath name");
	}
	FAILSAFE(pFileDialog->SetFolder(shlPtr));

	if (!openDialog)
	{
		// if we are saving a file, use the filter list's first extension as the default suffix
		// because otherwise we get an extension-less file...
		auto first_filter = split_wstring(fTypes, L";")[0];
		first_filter.erase(0, 2);
		pFileDialog->SetDefaultExtension(first_filter.c_str());
	}

	FAILSAFE(pFileDialog->Show(hWnd));
	FAILSAFE(pFileDialog->GetResult(&pShellItem));
	FAILSAFE(pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &pFilePath));

	wcstombs(path, pFilePath, MAX_PATH);
	wcsncpy(m_lastPathWide, pFilePath, MAX_PATH);

	succeeded = true;

cleanUp:
	if (pFilePath) CoTaskMemFree(pFilePath);
	if (pShellItem) pShellItem->Release();
	if (pFileDialog) pFileDialog->Release();

	return succeeded && strlen(path) > 0;
}

bool ReassociatingFileDialog::ShowFolderDialog(char* path, int maxSize,
                                               HWND hWnd)
{
	bool ret = false;
	IFileDialog* pfd;
	if (SUCCEEDED(
		CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&pfd))))
	{
		if (m_lastPathShort && strlen(m_lastPathShort) > 1)
		{
			printf("[ReassociatingFileDialog]: Restoring path \"%s\"...\n",
			       m_lastPathShort);
			PIDLIST_ABSOLUTE pidl;
			WCHAR wstarting_dir[MAX_PATH];
			WCHAR* wc = wstarting_dir;
			for (const char* c = m_lastPathShort; *c && wc - wstarting_dir <
			     MAX_PATH - 1; ++c, ++wc)
			{
				*wc = *c == '/' ? '\\' : *c;
			}
			*wc = 0;

			HRESULT hresult = ::SHParseDisplayName(
				wstarting_dir, 0, &pidl, SFGAO_FOLDER, 0);
			if (SUCCEEDED(hresult))
			{
				IShellItem* psi;
				hresult = ::SHCreateShellItem(NULL, NULL, pidl, &psi);
				if (SUCCEEDED(hresult))
				{
					pfd->SetFolder(psi);
				}
				ILFree(pidl);
			}
		}

		DWORD dwOptions;
		if (SUCCEEDED(pfd->GetOptions(&dwOptions)))
		{
			pfd->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_PATHMUSTEXIST);
		}
		if (SUCCEEDED(pfd->Show(hWnd)))
		{
			IShellItem* psi;
			if (SUCCEEDED(pfd->GetResult(&psi)))
			{
				WCHAR* tmp;
				if (SUCCEEDED(
					psi->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &tmp)))
				{
					char* c = path;
					while (*tmp != NULL && (c - path) < (maxSize - 1))
					{
						*c = (char)*tmp;
						++c;
						++tmp;
					}
					*c = '\0';
					ret = true;
				}
				psi->Release();
			}
		}
		pfd->Release();
	}


	// HACK: fail if non-filesystem folders were picked
	if (ret && path)
	{
		bool containsShellIdentifiers = false;
		for (size_t i = 0; i < maxSize; i++)
		{
			if (path[i] == '{' || path[i] == '}')
			{
				containsShellIdentifiers = true;
				break;
			}
		}
		if (containsShellIdentifiers)
		{
			printf("[ReassociatingFileDialog]: Special folder was picked!\n");
			ret = false;
		}
	}

	if (ret && path)
	{
		strcpy(m_lastPathShort, path);
	}

	return ret;
}


char* GetDefaultFileDialogPath(char* buf, const char* path)
{
	// buf must be MAX_PATH or greater
	std::filesystem::path fullPath = path;
	strncpy(buf, fullPath.parent_path().string().c_str(), MAX_PATH);
	return buf;
}

// https://gist.github.com/jsxinvivo/11f383ac61a56c1c0c25
wchar_t* ReassociatingFileDialog::convertCharArrayToLPCWSTR(
	const char* charArray)
{
	wchar_t* wString = new wchar_t[4096];
	MultiByteToWideChar(CP_ACP, 0, charArray, -1, wString, 4096);
	return wString;
}
