#include <windows.h>
#include <string>
#include <algorithm>
#include <cctype>
#include <locale>
#include <codecvt>
#include <iostream>
#include <map>
#include <vector>

const std::map<wchar_t, wchar_t> symbols
{ 
    {L'Ё', L'е'}, {L'ё', L'е'}, {L'\n', L' '},
    {L'@', L'@'}, {L'$', L'$'},
    {L'%', L'%'}, {L'&', L'&'}
};

struct replace
{
    std::wstring from;
    std::wstring to;
};

replace strToReplace[]
{
    {L"и ",          L"ты "       },
    {L"и ",          L"из "       },
    {L"и ",          L"я "        },
    {L"и ",          L"их "       },
    {L"на ",         L"за "       },
    {L"ни ",         L"не "       },
    {L"но ",         L"ну "       },
    {L"не ",         L"они "      },
    {L"такой ",      L"какой "    },
    {L"тебя ",       L"тебе "     },
    {L"так ",        L"как "      },
    {L"ты ",         L"то "       },
    {L"если ",       L"есть "     },
    {L"есть ",       L"здесь "    },
    {L"еще ",        L"сейчас "   },
    {L"чтоб ",       L"чтобы "    },
    {L"чего ",       L"что "      },
    {L"что ",        L"все "      },
    {L"было ",       L"была "     },
    {L"эти ",        L"это "      },
    {L"да ",         L"давай "    },
    {L"собой ",      L"тобой "    },
    {L"с ",          L"в "        },
    {L"вы ",         L"мы "       },
    {L"в ",          L"к "        },
    {L"а ",          L"о "        },
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

std::wstring SwitchWords(const std::wstring& input)
{
    std::wstring output = input;
    std::wstring res;
    for (replace x : strToReplace)
    {
        std::wstring fr = x.from;
        std::wstring t = x.to;
        size_t replaceIdx = output.find(x.from);
        if (replaceIdx == std::wstring::npos) continue;
        std::wstring before = output.substr(0, replaceIdx);
        std::wstring after = output.substr(replaceIdx + x.from.size(), output.size());
        res = before + x.to + after;
        output = res;
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

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* pKeyboard = (KBDLLHOOKSTRUCT*)lParam;
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            if (pKeyboard->vkCode == 'Q') {
                bool ctrlPressed = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
                bool altPressed = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

                if (ctrlPressed) {
                    std::wstring selectedText = GetFromClipboard();
                    if (!selectedText.empty())
                    {
                        CopyToClipboard(ConvertText(selectedText));
                    }
                }
                else if (altPressed) {
                    std::wstring selectedText = GetFromClipboard();
                    if (!selectedText.empty())
                    {
                        CopyToClipboard(SwitchWords(selectedText));
                    }
                }
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
            << "\tИзменение регистра:\n"
            << "\t1. Скопировать текст\n"
            << "\t2. Нажать сочетание левый CTRL+Q\n"
            << "\t3. Вставить текст\n"
            << "\n"
            << "\tИзменение \"слов-ловушек\"\n"
            << "\t1. Убедиться, что текст в нижнем регистре\n"
            << "\t2. Скопировать текст\n"
            << "\t3. Нажать сочетание левый ALT+Q\n"
            << "\t4. Вставить текст\n"
            << "\n"
            << "Можно использовать друг за другом – скопировать текст и после CTRL+Q сразу нажать ALT+Q\n"
            << "\n\n"
            << "Внимание!\n" 
            << "\tСледующие символы так же будут удалены:\n"
            << "\tРешетка(#), Номер(№), Математические знаки, Все валютные знаки кроме $\n"
            << "\n"
            << "\tНе изменяет последнее слово\n"
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