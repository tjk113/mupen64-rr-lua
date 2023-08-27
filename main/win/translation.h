/***************************************************************************
						  translation.h  -  description
							 -------------------
	copyright            : (C) 2003 by ShadowPrince
	email                : shadow@emulation64.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <Windows.h>



void SelectLang(HWND hWnd, int LangMenuID);
void SetupLanguages(HWND hWnd);
void Translate(const char* GuiWord, char* Ret);
void TranslateDefault(const char* GuiWord, const char* Default, char* Ret);
void SetItemTranslatedString(HWND hwnd, int ElementID, const char* Str);
void SetStatusTranslatedString(HWND hStatus, int section, const char* Str);
void SetMenuTranslatedString(HMENU hMenu, int elementID, const char* Str, const char* Acc);
void SetMenuAccelerator(HMENU hMenu, int elementID, const char* Acc);
void TranslateMenu(HMENU hMenu, HWND mainHWND);
void TranslateRomInfoDialog(HWND hwnd);
void TranslateRomBrowserMenu(HMENU hMenu);
void TranslateAuditDialog(HWND hwnd);
void TranslateGeneralDialog(HWND hwnd);
void TranslateLangInfoDialog(HWND hwnd);
void TranslateAdvancedDialog(HWND hwnd);
void TranslateHotkeyDialog(HWND hwnd);
void freeLanguages();
void SetMenuAcceleratorsFromUser(HWND hwnd);

