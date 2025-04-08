#pragma once
#include <string>
#include <vector>



class C_Info {
public:
  void Instance();

  void FillMessages();
  void FillImages();
  void Update();

  size_t GetMessagesSize() { return m_sMessagesVec.size(); };
  std::vector<std::wstring>& GetMessagesVec() { return m_sMessagesVec; };
  const int GetImagesSize() { return m_iSizeFilesArray; };
  std::string* GetImagesArr() { return m_sFileNamesArr; };


  const std::wstring getMessage(int idx) { return m_sMessagesVec.at(idx); };
private:
  std::vector<std::wstring> m_sMessagesVec;
  std::string* m_sFileNamesArr;
  int m_iSizeFilesArray;

  uintmax_t m_MessagesFileSize;
  int m_iImagesCount;

};

inline C_Info *g_Info = new C_Info();