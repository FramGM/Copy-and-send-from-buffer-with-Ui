#include "info.h"
#include <fstream>
#include <ShlObj.h>
#include <filesystem>
#include <string>
#include <iostream>
#include "filesystem/filesystem.h"

void C_Info::Instance() 
{
  
}

std::wstring StringToWString(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size_needed);
    return wstr;
}
void C_Info::FillMessages() 
{
    std::ifstream text_file(g_Filesystem->getMessagesPath());
    std::string message;

    while (std::getline(text_file, message))  
        m_sMessagesVec.push_back(StringToWString(message));

    text_file.close();

    m_MessagesFileSize = std::filesystem::file_size(std::filesystem::path(g_Filesystem->getMessagesPath()));
}

void C_Info::FillImages()
{
    std::vector<std::string> temp_dirs;
    for (const auto& entry : std::filesystem::directory_iterator(g_Filesystem->getImagesPath()))
    {
        temp_dirs.push_back(entry.path().string());
    }

    int arraySize = m_sMessagesVec.size();
    m_sFileNamesArr = new std::string[arraySize];
    m_iSizeFilesArray = arraySize;

    for (int i = 0; i < arraySize; i++)
    {
        for (int j = 0; j < temp_dirs.size(); j++)
        {
          if (m_sFileNamesArr[i] != "")
            continue;

            std::string fFile = temp_dirs.at(j);
            fFile = fFile.substr(g_Filesystem->getImagesPath().length());
            std::string fileNumber = "";

            for (auto& ch : fFile) {
              if (isdigit(ch))
                fileNumber += ch;
            }
            
            int NumberImage = stoi(fileNumber);

            if (NumberImage > arraySize) {
              printf("Wrong name of image!\n");
              return;
            }
            else if (NumberImage <= 0) {
              printf("NumberImage > 100!\n");
              return;
            }
            else if(NumberImage >= 100) {
              printf("NumberImage > 100!\n");
              return;
            }
              

            if (NumberImage == i + 1)
                m_sFileNamesArr[i] = temp_dirs.at(j);
        }

    }

    m_iImagesCount = temp_dirs.size();
}

void C_Info::Update() 
{
    int ImagesCount = 0;
    for (const auto& entry : std::filesystem::directory_iterator(g_Filesystem->getImagesPath()))
    {
        ImagesCount++;
    }

    if (std::filesystem::file_size(std::filesystem::path(g_Filesystem->getMessagesPath())) == m_MessagesFileSize && ImagesCount == m_iImagesCount)
        return;

    m_sMessagesVec.clear();
    delete[] m_sFileNamesArr;
    FillMessages();
    FillImages();
}