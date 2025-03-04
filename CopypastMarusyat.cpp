#include <windows.h>
#include <string>
#include <algorithm>
#include <cctype>
#include <locale>
#include <codecvt>
#include <iostream>
#include <map>

const std::map<wchar_t, wchar_t> symbols
{ 
    {L'Ё', L'е'}, {L'ё', L'е'}, {L'\n', L' '},
    {L'@', L'@'}, {L'$', L'$'},
    {L'%', L'%'}, {L'&', L'&'}
};

const std::locale locale("ru-RU.utf-8");

std::wstring ConvertText(const std::wstring& input) 
{
    std::wstring output;
    for (wchar_t c : input) 
    {
        if (symbols.count(c))
        {
            if (c == L'\n')
                output.pop_back();
            output.push_back(symbols.at(c));
        }
        else if (!iswpunct(c))
        {
            if (iswspace(output.back()) && iswspace(c))
            {
                continue;
            }
            output += tolower(c, locale);
        }
        else if ((iswalpha(output.back()) || iswdigit(output.back())) && c == L'-')
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
            << "Внимание! Следующие символы так же будут удалены:\n"
            << "Решетка(#), Номер(№), Математические знаки, Все валютные знаки кроме $"
            << std::endl;
    }
    // # № 
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