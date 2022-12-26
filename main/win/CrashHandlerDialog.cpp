#include "CrashHandlerDialog.h"
#include "main_win.h"
#include "../../winproject/resource.h"

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

        DWORD exStyle = ::GetWindowLong(GetDlgItem(hwnd, IDC_ERROR_PICTUREBOX), GWL_EXSTYLE);

        SetWindowLong(GetDlgItem(hwnd, IDC_ERROR_PICTUREBOX), GWL_EXSTYLE, exStyle | WS_EX_LAYERED);

        HBITMAP hBitmap = reinterpret_cast<HBITMAP>(::LoadImage(
            GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_LOGO_BW), IMAGE_BITMAP,
            0, 0, LR_CREATEDIBSECTION));

        SendMessage(GetDlgItem(hwnd, IDC_ERROR_PICTUREBOX), STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);

        EnableWindow(GetDlgItem(hwnd, IDC_ERROR_PANIC_IGNORE), (BOOL)(CrashHandlerDialog::Type & CrashHandlerDialog::Types::Ignorable));

        // for some reason windows holds pointer to this so cant dispose it now
        // this, of course, causes a memory leak because we should free upon calling EndDialog()
        //DeleteObject((HBITMAP)hBitmap);

        return TRUE;
    }
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
    default:
        return FALSE;

    }

}

CrashHandlerDialog::Choices CrashHandlerDialog::Show() {
	DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_MUPENERROR), 0, CrashHandlerDialogProcedure);

    return CrashHandlerDialogChoice;
}
