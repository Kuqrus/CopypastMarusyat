#include <windows.h>
#include <string>
#include <algorithm>
#include <cctype>
#include <locale>
#include <codecvt>
#include <iostream>

std::locale locale("ru-RU.utf-8");

std::wstring ConvertText(const std::wstring& input) 
{
    std::wstring output;
    for (wchar_t c : input) 
    {
        if (!iswpunct(c)) {            
            if (tolower(c, locale) == L'ё')
            {
                output += L'е';
            }
            else if (iswspace(output[output.size() - 1]) && iswspace(c))
            {
                continue;
            }
            else
            {
                output += tolower(c, locale);
            }
        }
        else if (iswalpha(output[output.size() - 1]) && c == '-')
        {
            output += c;
        }
    }
    return output;
}

void CopyToClipboard(const std::wstring& text) 
{
    if (OpenClipboard(nullptr)) 
    {
        EmptyClipboard();
        size_t size = (text.size() + 1) * sizeof(wchar_t);
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
        if (hMem) 
        {
            memcpy(GlobalLock(hMem), text.c_str(), size);
            GlobalUnlock(hMem);
            SetClipboardData(CF_UNICODETEXT, hMem);
        }
        CloseClipboard();
    }
}

std::wstring GetFromClipboard() 
{
    std::wstring result;
    if (OpenClipboard(nullptr)) 
    {
        HANDLE hData = GetClipboardData(CF_UNICODETEXT);
        if (hData) 
        {
            wchar_t* pText = static_cast<wchar_t*>(GlobalLock(hData));
            if (pText) 
            {
                result = pText;
                GlobalUnlock(hData);
            }
        }
        CloseClipboard();
    }
    return result;
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) 
{
    if (nCode == HC_ACTION && wParam == WM_KEYDOWN) 
    {
        KBDLLHOOKSTRUCT* kbdStruct = (KBDLLHOOKSTRUCT*)lParam;
        if (kbdStruct->vkCode == 'Q' && GetAsyncKeyState(VK_CONTROL) & 0x8000) 
        {
            std::wstring selectedText = GetFromClipboard();
            if (!selectedText.empty()) 
            {
                std::wstring processedText{ ConvertText(selectedText) };
                CopyToClipboard(processedText);
            }
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

int main() 
{
    setlocale(LC_ALL, "");

    HWND hwndConsole = GetConsoleWindow();
    if (hwndConsole) 
    {
        HICON exeIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDC_ICON));

        MoveWindow(hwndConsole, 100, 100, 800, 400, TRUE);

        std::cout 
            << "Порядок использования:\n"
            << "1. Скопировать текст\n"
            << "2. Нажать сочетание левый CTRL+Q\n"
            << "3. Вставить текст\n"
            << "\n\n"
            << "Внимание! Текущая версия удаляет дефис между цифрами (2-3 -> 23) и не удаляет перенос на следующую строку"
            << std::endl;
    }

    HHOOK hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
    if (!hHook) 
    {
        MessageBox(NULL, L"Не удалось установить хук!", L"Ошибка", MB_OK);
        return 1;
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) 
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hHook);
    return 0;
}