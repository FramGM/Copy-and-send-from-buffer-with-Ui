#pragma once
#include <string>
#include <vector>
#include "../imgui/imgui.h"

class C_TextBox
{
public:
	ImVector<char>& GetText() { return m_vecText; }
	void SetText(ImVector<char> vecText) { m_vecText = vecText; }

	void AddFile(std::string strPath) { m_vecFilesPaths.push_back(strPath); }
	std::string GetFileByIndex(int iIndex) { return m_vecFilesPaths.at(iIndex); }
	int GetFilesSize() { return m_vecFilesPaths.size(); }
private:
	ImVector<char> m_vecText;
	std::vector<std::string> m_vecFilesPaths;
};
