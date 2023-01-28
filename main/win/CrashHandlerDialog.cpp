#include "CrashHandlerDialog.h"
#include "main_win.h"
#include "../../winproject/resource.h"
#include <wingdi.h>
#pragma comment(lib, "Msimg32.lib") // TransparentBlt
HBITMAP bitmapHandle;

inline CrashHandlerDialog::Types operator&(CrashHandlerDialog::Types a, CrashHandlerDialog::Types b)
{
    return static_cast<CrashHandlerDialog::Types>(static_cast<int>(a) & static_cast<int>(b));
}

inline CrashHandlerDialog::Types operator|(CrashHandlerDialog::Types a, CrashHandlerDialog::Types b)
{
    return static_cast<CrashHandlerDialog::Types>(static_cast<int>(a) | static_cast<int>(b));
}

BOOL CALLBACK CrashHandlerDialogProcedure(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
    switch (Message) {
    case WM_INITDIALOG: {

        SetWindowText(hwnd, MUPEN_VERSION);
        
        SetWindowText(GetDlgItem(hwnd, IDC_ERRORTEXT), CrashHandlerDialog::Caption);

        bitmapHandle = reinterpret_cast<HBITMAP>(::LoadImage(
            GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_LOGO_BW), IMAGE_BITMAP,
            0, 0, LR_CREATEDIBSECTION));
        
        EnableWindow(GetDlgItem(hwnd, IDC_ERROR_PANIC_IGNORE), (BOOL)(CrashHandlerDialog::Type & CrashHandlerDialog::Types::Ignorable));

        return TRUE;
    }
    case WM_DRAWITEM:
        if (wParam == IDC_ERROR_PICTUREBOX) // Draw transparent bitmap in IDC_IMG3
        {
            LPDRAWITEMSTRUCT lpDrawItem = (LPDRAWITEMSTRUCT)lParam;
            HDC hMemoryDc = CreateCompatibleDC(lpDrawItem->hDC);
            HGDIOBJ hOldBitmap = SelectObject(hMemoryDc, bitmapHandle);
            BITMAP bitmap;
            GetObject(bitmapHandle, sizeof(BITMAP), &bitmap);
            TransparentBlt(lpDrawItem->hDC, 0, 0, bitmap.bmWidth, bitmap.bmHeight, hMemoryDc, 0, 0, bitmap.bmWidth, bitmap.bmHeight, RGB(255, 255, 255));
            SelectObject(hMemoryDc, hOldBitmap);
            SetWindowPos(lpDrawItem->hwndItem, NULL, NULL, NULL, bitmap.bmWidth, bitmap.bmHeight, SWP_NOMOVE);
            DeleteDC(hMemoryDc);
            DeleteObject(hOldBitmap);
            return TRUE;
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_ERROR_PANIC_IGNORE:
            CrashHandlerDialog::CrashHandlerDialogChoice = CrashHandlerDialog::Choices::Ignore;
            EndDialog(hwnd, 0);
            break;
        case IDC_ERROR_PANIC_CLOSE:
            CrashHandlerDialog::CrashHandlerDialogChoice = CrashHandlerDialog::Choices::Exit;
            EndDialog(hwnd, 0);
            break;
        }
        break;
    case WM_DESTROY:
        DeleteObject(bitmapHandle);
        break;
    default:
        return FALSE;

    }

}

CrashHandlerDialog::Choices CrashHandlerDialog::Show() {
	DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_MUPENERROR), 0, CrashHandlerDialogProcedure);

    return CrashHandlerDialogChoice;
}
