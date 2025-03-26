#pragma once
#include "Tab.h"

class C_Include
{
public:
	ImVec2 vecWindowSize = ImVec2(1000, 700);
	std::vector<C_Tab*> m_vecTabs;
	int m_iCurrentTab = 0;

};

inline C_Include* g_Include = new C_Include();