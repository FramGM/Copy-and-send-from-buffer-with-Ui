#include "UI.h"
#include <iostream>
#include <fstream>
#include <string>
#include "../info.h"
#include "../filesystem/filesystem.h"
#include <Windows.h>
#include <filesystem>
using namespace std;

bool C_UI::Instance() 
{
    int ErrorCode = g_Filesystem->Instance();
    if (ErrorCode != OK) {
      printf("\nErrorCode: %i", ErrorCode);
        return false;
    }

    std::cout << "Заполните текстовый документ Messages.txt!" << endl;

    while (!std::filesystem::file_size(std::filesystem::path(g_Filesystem->getMessagesPath())))
    {
        Sleep(1000);
    }

    system("cls");

    g_Info->FillMessages();
    g_Info->FillImages();

    printf("\nВсё прошло успешно!");
    return true;
}