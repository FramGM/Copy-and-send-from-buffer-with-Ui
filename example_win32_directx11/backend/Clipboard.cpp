#include "Clipboard.h"

//---------------------------------------------------------------------------
#ifndef COPYFFILES_TO_CLIPBOARD_CPP
#define COPYFFILES_TO_CLIPBOARD_CPP
//---------------------------------------------------------------------------
#define NO_WIN32_LEAN_AND_MEAN
#include <vector>
#include <string>
#include <Windows.h>
using namespace std;
//---------------------------------------------------------------------------
typedef struct _DROPFILES
{
    DWORD pFiles; // offset of file list
    POINT pt;     // drop point (coordinates depend on fNC)
    BOOL fNC;     // see below
    BOOL fWide;   // TRUE if file contains wide characters,
                  // FALSE otherwise
} DROPFILES, FAR * LPDROPFILES;
//---------------------------------------------------------------------------
bool CopyFilesToClipboard(vector <string> &ListFile)
{
    vector<string>::iterator It;
   //открываем буфер
   if(!OpenClipboard(0)) return false;
   //очищаем буфер
   EmptyClipboard();
   //Определяем размер
   int SizeLine(1);
   for (It=ListFile.begin();It!=ListFile.end();++It)
      SizeLine+=(*It).length()+1;
   //Выделяем память
   HGLOBAL  hGlobal = GlobalAlloc(GMEM_SHARE|GMEM_MOVEABLE|GMEM_ZEROINIT,
                                            sizeof(DROPFILES) + SizeLine);
   if (!hGlobal) return false;
   //Создаем структуру
   DROPFILES *MyDropFiles = (DROPFILES*)GlobalLock(hGlobal);
   //записываем данные в структуру
   SizeLine = MyDropFiles->pFiles = sizeof(DROPFILES);
   for (It=ListFile.begin();It!=ListFile.end();++It)
   {
      strcpy((char*)MyDropFiles + SizeLine, (*It).c_str());
      SizeLine+=(*It).length()+1;
   }
   GlobalUnlock(hGlobal);
   //записываем данные в буфер
   if (!SetClipboardData(CF_HDROP, hGlobal)) return false;
   //закрываем буфер
   CloseClipboard();
   return true;
}
//---------------------------------------------------------------------------
#endif
#include <iostream>

void Clipboard::TextToClipboard(const std::wstring& text) {
    if (!OpenClipboard(nullptr)) {
        std::cerr << "Не удалось открыть буфер обмена!" << std::endl;
        return;
    }

    if (!EmptyClipboard()) {
        std::cerr << "Не удалось очистить буфер обмена!" << std::endl;
        CloseClipboard();
        return;
    }

    size_t sizeInBytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, sizeInBytes);
    if (!hGlob) {
        CloseClipboard();
        return;
    }

    void* pGlobMem = GlobalLock(hGlob);
    memcpy(pGlobMem, text.c_str(), sizeInBytes);
    GlobalUnlock(hGlob);

    SetClipboardData(CF_UNICODETEXT, hGlob);
    CloseClipboard();
}

std::wstring Clipboard::TextFromClipboard() {
    std::wstring result;

    if (OpenClipboard(nullptr)) {
        HANDLE hData = GetClipboardData(CF_UNICODETEXT);
        if (hData) {
          wchar_t *pMem = static_cast<wchar_t *>(GlobalLock(hData));
          if (pMem) {
            result = pMem;
            GlobalUnlock(hData);
          }
        }
        CloseClipboard();
    } else {


    }

    return result;
}

void Clipboard::CopyFileToClipboard(string filePath) {
    std::vector<std::string> fl;
    fl.push_back(filePath);
    CopyFilesToClipboard(fl);
}

void Clipboard::CtrlV() 
{
    keybd_event(VK_CONTROL, 0x1D, KEYEVENTF_EXTENDEDKEY | 0, 0);
    keybd_event('V', 0x2F, KEYEVENTF_EXTENDEDKEY | 0, 0);
    keybd_event('V', 0x2F, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
    keybd_event(VK_CONTROL, 0x1D, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
}

void Clipboard::Enter() 
{
    keybd_event(VK_RETURN, 0x2F, KEYEVENTF_EXTENDEDKEY | 0, 0);
    keybd_event(VK_RETURN, 0x2F, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
}
