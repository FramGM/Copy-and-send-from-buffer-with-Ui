#pragma once
#include <string>
#include "TextBox.h"
struct KeyBind_t
{
	constexpr KeyBind_t(const char* szName, const unsigned int uKey = 0U) :
		szName(szName), uKey(uKey) {
	}

	bool bEnable = false;
	const char* szName = nullptr;
	unsigned int uKey = 0U;
};

class C_Tab
{
public:
	C_Tab(std::string strTabName) : pKeyBind(KeyBind_t("")) { m_strTabName = strTabName; }
	~C_Tab() {};

	std::string GetName() { return m_strTabName; }
	void SetIndex(int iIndex) { m_iTabIndex = iIndex; }
	int GetIndex() { return m_iTabIndex; }
	int GetBoxSize() { return vecTextContainers.size(); }
	C_TextBox& GetBox(int iIndex) { return vecTextContainers.at(iIndex); }
	void AddBox(C_TextBox pBox) { vecTextContainers.push_back(pBox); }


	void SetText(int iInputBoxIndex, ImVector<char> vecText)
	{
		vecTextContainers.at(iInputBoxIndex).SetText(vecText);
	}
	ImVector<char>& GetText(int iInputBoxIndex)
	{
		return vecTextContainers.at(iInputBoxIndex).GetText();
	}

	KeyBind_t pKeyBind;
private:
	std::string m_strTabName;
	int m_iTabIndex = -1;

	std::vector<C_TextBox> vecTextContainers;
};

