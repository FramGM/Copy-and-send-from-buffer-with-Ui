#pragma once

struct ImFont;
class C_UI 
{
public:
  bool Instance();
  ImFont* pTextFont = nullptr;

private:
};

inline C_UI *g_UI = new C_UI();