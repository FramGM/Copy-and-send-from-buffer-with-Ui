#include "filesystem.h"
#include <fstream>
#include <ShlObj.h>
#include <filesystem>
#include "../info.h"
#include "../images/1.h";
#include "../images/2.h";
#include "../images/3.h";
#include "../images/4.h";

//std::string abs_exe_path()
//{
//    wchar_t path[FILENAME_MAX] = { 0 };
//    GetModuleFileNameW(nullptr, path, FILENAME_MAX);
//    return std::filesystem::path(path).string();
//}
//
//std::string abs_exe_directory()
//{
//    wchar_t path[FILENAME_MAX] = { 0 };
//    GetModuleFileNameW(nullptr, path, FILENAME_MAX);
//    return std::filesystem::path(path).parent_path().string();
//}

int C_Filesystem::Instance()
{
    std::wstring programFolder = getProgramFolder();
    if (std::filesystem::exists(programFolder + L"\\ProgFiles") && 
        std::filesystem::exists(programFolder + L"\\ProgFiles\\Messages.txt") &&
        std::filesystem::exists(programFolder + L"\\ProgFiles\\Images"))
    {
        m_sMessagesPath = programFolder + L"\\ProgFiles\\Messages.txt";
      m_sImagesPath = programFolder + L"\\ProgFiles\\Images";
        return OK;
    }

    if (!std::filesystem::create_directory(programFolder + L"\\ProgFiles"))
        return PROGRAM_PATH;

    std::ofstream outFile(programFolder + L"\\ProgFiles\\Messages.txt");
    if (!outFile) {
        outFile.close();
        return MESSAGES_PATH;
    }
    outFile.close();

    m_sMessagesPath = programFolder + L"\\ProgFiles\\Messages.txt";

    if (!std::filesystem::create_directory(programFolder + L"\\ProgFiles\\Images"))
        return IMAGES_PATH;

    m_sImagesPath = programFolder + L"\\ProgFiles\\Images";

    return OK;
}

std::wstring C_Filesystem::getProgramFolder() {
  PWSTR p = nullptr;
  HRESULT result = SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &p);

  size_t length = wcslen(p); // Определите длину строки
  std::wstring wstr(p, length); // Преобразование PWSTR в std::wstring с учетом длины

  return wstr;
}

bool C_Filesystem::deleteImage()
{
    if (!g_Info->GetImagesSize())
        return false;

    // Укажите путь к файлу, который хотите удалить
    //std::string file_path = m_sDownloadsFolder + "\\" + g_Info->GetImagesVec().at(g_Info->GetImagesSize() - 1);

    //g_Info->GetImagesVec().erase(g_Info->GetImagesVec().end() - 1);
    // Попробуйте удалить файл
//    try
//    {
//
//        if (std::filesystem::remove(file_path))
//            return true;
//        else
//            return false;
//
//    }
//    catch (const std::filesystem::filesystem_error& e)
//    {
//        return false;
//    }
    return false;
}

std::wstring C_Filesystem::downloadImage(int imgIdx) {
    size_t image_size;

    switch (imgIdx)
    {
    case 0:
        image_size = sizeof(img1) / sizeof(img1[0]);
        break;
    case 1:
        image_size = sizeof(img2) / sizeof(img2[0]);

        break;
    case 2:
        image_size = sizeof(img3) / sizeof(img3[0]);

        break;
    case 3:
        image_size = sizeof(img4) / sizeof(img4[0]);
        break;
    default:
        break;
    }
    // Размер массива
    srand(time(0));
    std::wstring name_of_file = L"";

    for (int i = 0; i < 5; i++) {
        name_of_file += char(rand() % ('Z' - 'A' + 1) + 'A');
        name_of_file += char(rand() % ('z' - 'a' + 1) + 'a');
    }
    name_of_file += L".png";

    std::wstring output_path = m_sProgramFolder + L"\\" + name_of_file;

    std::ofstream output_file(output_path, std::ios::binary);
    if (!output_file)
        return L"";

    switch (imgIdx)
    {
    case 0:
        output_file.write(reinterpret_cast<char*>(img1), image_size);
        break;
    case 1:
        output_file.write(reinterpret_cast<char*>(img2), image_size);

        break;
    case 2:
        output_file.write(reinterpret_cast<char*>(img3), image_size);

        break;
    case 3:
        output_file.write(reinterpret_cast<char*>(img4), image_size);
        break;
    default:
        break;
    }

    output_file.close();

    //g_Info->GetImages().push_back(name_of_file);

    return name_of_file;
}
