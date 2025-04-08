#pragma once
#include <string>

class Clipboard 
{
public:
  void TextToClipboard(const std::wstring& text);
  std::wstring TextFromClipboard();
  void CopyFileToClipboard(std::string filePath);

  void CtrlV();
  void Enter();
};

inline Clipboard *g_Clipboard = new Clipboard();