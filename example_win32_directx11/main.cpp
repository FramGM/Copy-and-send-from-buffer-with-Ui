// Dear ImGui: standalone example application for DirectX 11

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <vector>
#include <string>
#include <filesystem>
#include "imgui/imgui_internal.h"
#include "backend/ui/UI.h"
#include "frontend/TextBox.h"
#include "frontend/Includes.h"

// Data
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);



namespace ImGui
{
	bool HotKey(const char* szLabel, unsigned int* pValue);

}

bool ImGui::HotKey(const char* szLabel, unsigned int* pValue)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* pWindow = g.CurrentWindow;

	if (pWindow->SkipItems)
		return false;

	ImGuiIO& io = g.IO;
	const ImGuiStyle& style = g.Style;
	const ImGuiID nIndex = pWindow->GetID(szLabel);

	const float flWidth = CalcItemWidth();
	const ImVec2 vecLabelSize = CalcTextSize(szLabel, nullptr, true);

	const ImRect rectFrame(pWindow->DC.CursorPos + ImVec2(vecLabelSize.x > 0.0f ? style.ItemInnerSpacing.x + GetFrameHeight() : 0.0f, 0.0f), pWindow->DC.CursorPos + ImVec2(flWidth, vecLabelSize.x > 0.0f ? vecLabelSize.y + style.FramePadding.y : 0.f));
	const ImRect rectTotal(rectFrame.Min, rectFrame.Max);

	ItemSize(rectTotal, style.FramePadding.y);
	if (!ItemAdd(rectTotal, nIndex, &rectFrame))
		return false;

	const bool bHovered = ItemHoverable(rectFrame, nIndex, ImGuiItemFlags_None);
	if (bHovered)
	{
		SetHoveredID(nIndex);
		g.MouseCursor = ImGuiMouseCursor_TextInput;
	}

	const bool bClicked = bHovered && io.MouseClicked[0];
	const bool bDoubleClicked = bHovered && io.MouseDoubleClicked[0];
	if (bClicked || bDoubleClicked)
	{
		if (g.ActiveId != nIndex)
		{
			memset(io.MouseDown, 0, sizeof(io.MouseDown));
			io.AddKeyEvent(ImGuiKey_NamedKey_BEGIN, 0);
			*pValue = 0U;
		}

		SetActiveID(nIndex, pWindow);
		FocusWindow(pWindow);
	}

	bool bValueChanged = false;
	if (unsigned int nKey = *pValue; g.ActiveId == nIndex)
	{
		for (int n = 0; n < IM_ARRAYSIZE(io.MouseDown); n++)
		{
			if (IsMouseDown(n))
			{
				switch (n)
				{
				case 0:
					nKey = VK_LBUTTON;
					break;
				case 1:
					nKey = VK_RBUTTON;
					break;
				case 2:
					nKey = VK_MBUTTON;
					break;
				case 3:
					nKey = VK_XBUTTON1;
					break;
				case 4:
					nKey = VK_XBUTTON2;
					break;
				}

				bValueChanged = true;
				ClearActiveID();
			}
		}

		if (!bValueChanged)
		{
			for (int n = ImGuiKey_NamedKey_BEGIN; n < ImGuiKey_NamedKey_END; n++)
			{
				if (IsKeyDown((ImGuiKey)n) && n != ImGuiKey_MouseLeft)
				{
					nKey = n;
					bValueChanged = true;
					ClearActiveID();
				}
			}
		}

		if (IsKeyPressed(ImGuiKey_Escape))
		{
			*pValue = 0U;
			ClearActiveID();
		}
		else
			*pValue = nKey;
	}

	char szBuffer[64] = {};
	char* szBufferEnd = strcpy(szBuffer, "  ");
	if (*pValue != 0 && g.ActiveId != nIndex)
		szBufferEnd = strcat(szBufferEnd, GetKeyName((ImGuiKey)*pValue));
	else if (g.ActiveId == nIndex)
		szBufferEnd = strcat(szBufferEnd, ("press"));
	else
		szBufferEnd = strcat(szBufferEnd, ("none"));
	strcat(szBufferEnd, "  ");

	// modified by neir0n
	PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.x, -1));

	const ImVec2 vecBufferSize = CalcTextSize(szBuffer);
	RenderFrame(ImVec2(rectFrame.Max.x - vecBufferSize.x, rectTotal.Min.y), ImVec2(rectFrame.Max.x, rectTotal.Min.y + style.FramePadding.y + vecBufferSize.y), GetColorU32((bHovered || bClicked || bDoubleClicked) ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), true, style.FrameRounding);
	pWindow->DrawList->AddText(ImVec2(rectFrame.Max.x - vecBufferSize.x, rectTotal.Min.y + style.FramePadding.y), GetColorU32(g.ActiveId == nIndex ? ImGuiCol_Text : ImGuiCol_TextDisabled), szBuffer);

	if (vecLabelSize.x > 0.f)
		RenderText(ImVec2(rectTotal.Min.x, rectTotal.Min.y + style.FramePadding.y), szLabel);

	PopStyleVar();
	return bValueChanged;
}



void RenderTabs()
{
	ImGui::BeginChild("Child tab", ImVec2(165, g_Include->vecWindowSize.y - 55), ImGuiChildFlags_Borders);
	{
		for (int iTab = 0; iTab < g_Include->m_vecTabs.size(); iTab++)
		{
			if (ImGui::Button(g_Include->m_vecTabs.at(iTab)->GetName().c_str(), ImVec2(147, 40)))
				g_Include->m_iCurrentTab = iTab;
		}

		if (ImGui::Button("+", ImVec2(147, 40)))
			g_Include->m_iCurrentTab = INT_MAX;

	}
	ImGui::EndChild();
}

struct CSettings
{
	bool bChangeStyle = false;
	char m_strConfigName[100];
	ImVec4 m_vecChildColor;


} *g_Settings = new CSettings();

void AddConfig()
{
	ImGui::InputText("Input config name", g_Settings->m_strConfigName, IM_ARRAYSIZE(g_Settings->m_strConfigName));

	if (ImGui::Button("Add config"))
	{
		C_Tab* pNewTab = new C_Tab(g_Settings->m_strConfigName);
		pNewTab->SetIndex(g_Include->m_vecTabs.size() - 1);

		g_Include->m_vecTabs.push_back(pNewTab);
		g_Include->m_iCurrentTab = g_Include->m_vecTabs.size() - 1;

		for (auto& ch : g_Settings->m_strConfigName)
		{
			ch = '\0';
		}
	}
}

static int MyResizeCallback(ImGuiInputTextCallbackData* data)
{
	if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
	{
		ImVector<char>* my_str = (ImVector<char>*)data->UserData;
		IM_ASSERT(my_str->begin() == data->Buf);
		my_str->resize(data->BufSize); // NB: On resizing calls, generally data->BufSize == data->BufTextLen + 1
		data->Buf = my_str->begin();
	}
	return 0;
}
static bool MyInputTextMultiline(const char* label, ImVector<char>* my_str, const ImVec2& size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0)
{
	IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
	return ImGui::InputTextMultiline(label, my_str->begin(), (size_t)my_str->size(), size, flags | ImGuiInputTextFlags_CallbackResize, MyResizeCallback, (void*)my_str);
}

#include <shobjidl.h> // For IFileDialog
#include <iostream>
#include "backend/Clipboard.h"
void GetFilePath(C_TextBox& pBox)
{// Initialize COM library
	CoInitialize(NULL);

	// Create the file dialog
	IFileDialog* pFileDialog = NULL;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_PPV_ARGS(&pFileDialog));

	if (SUCCEEDED(hr)) {
		// Show the dialog
		hr = pFileDialog->Show(NULL);
		if (SUCCEEDED(hr)) {
			// Get the result
			IShellItem* pItem;
			hr = pFileDialog->GetResult(&pItem);
			if (SUCCEEDED(hr)) {
				// Get the file path
				LPWSTR filePath;
				hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &filePath);
				if (SUCCEEDED(hr)) {
					// Output the file path
					std::wstring ws(filePath);
					std::string myVarS = std::string(ws.begin(), ws.end());
					pBox.AddFile(myVarS);
					CoTaskMemFree(filePath); // Free the memory allocated for the file path
				}
				pItem->Release();
			}
		}
		pFileDialog->Release();
	}
	else {
		std::cerr << "Failed to create file dialog." << std::endl;
	}

	// Uninitialize COM library
	CoUninitialize();
}

void AddFileButton(int iIndex, C_Tab*& pCurrentTab)
{
	std::string strAddFile = "Add file##" + std::to_string(iIndex);
	auto& pBox = pCurrentTab->GetBox(iIndex);

	if (ImGui::Button(strAddFile.c_str()))
	{
		GetFilePath(pBox);
	}

}

void RenderMainTab()
{
	ImGui::Checkbox("Change style", &g_Settings->bChangeStyle);
	ImGui::ColorEdit3("Child color", (float*)&g_Settings->m_vecChildColor);
}

void RenderTexts()
{
	ImGui::BeginChild("Text Child", ImVec2(g_Include->vecWindowSize.x - 205, g_Include->vecWindowSize.y - 55), ImGuiChildFlags_Borders);
	{
		if (!g_Include->m_iCurrentTab)
			RenderMainTab();
		else if (g_Include->m_iCurrentTab == INT_MAX)
			AddConfig();

		if (g_Include->m_iCurrentTab && g_Include->m_iCurrentTab != INT_MAX)
		{
			C_Tab* pCurrentTab = g_Include->m_vecTabs.at(g_Include->m_iCurrentTab);
			ImVector<char> strEmptyText;
			if (strEmptyText.empty())
				strEmptyText.push_back(0);

			C_TextBox pBox;
			pBox.SetText(strEmptyText);
			if (ImGui::Button("Add text"))
				pCurrentTab->AddBox(pBox);

			ImGui::SameLine();

			if (ImGui::Button("Delete tab"))
			{
				g_Include->m_vecTabs.erase(g_Include->m_vecTabs.begin() + g_Include->m_iCurrentTab);
				g_Include->m_iCurrentTab = 0;
			}

			ImGui::HotKey("key", &pCurrentTab->pKeyBind.uKey);
			if (pCurrentTab->pKeyBind.uKey != 0)
				pCurrentTab->pKeyBind.szName = ImGui::GetKeyName((ImGuiKey)pCurrentTab->pKeyBind.uKey);

			ImGui::NewLine();

			ImGui::PushFont(g_UI->pTextFont);
			for (int i = 0; i < pCurrentTab->GetBoxSize(); i++)
			{
				
				std::string strBoxName = "Text##" + std::to_string(i);
				ImVector<char> strText = pCurrentTab->GetText(i);
				MyInputTextMultiline(strBoxName.c_str(), &strText);
				pCurrentTab->SetText(i, strText);

				AddFileButton(i, pCurrentTab);
				ImGui::NewLine();
			}
			ImGui::PopFont();

		}

	}
	ImGui::EndChild();
}

// 
// or the standard way taken from the answer above
#include <codecvt>
#include <string>

// convert UTF-8 string to wstring
std::wstring utf8_to_wstring(const std::string& str) {
	std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
	return myconv.from_bytes(str);
}

int ConvertImGuiKeyToChar(ImGuiKey iKey)
{
	switch (iKey)
	{
	case ImGuiKey_None:
		break;
	case ImGuiKey_NamedKey_BEGIN:
		return VK_TAB;
		break;
	case ImGuiKey_LeftArrow:
		return VK_LEFT;
		break;
	case ImGuiKey_RightArrow:
		return VK_RIGHT;
		break;
	case ImGuiKey_UpArrow:
		return VK_UP;
		break;
	case ImGuiKey_DownArrow:
		return VK_DOWN;
		break;
	case ImGuiKey_PageUp:
		return VK_PRIOR;
		break;
	case ImGuiKey_PageDown:
		return VK_NEXT;
		break;
	case ImGuiKey_Home:
		return VK_HOME;
		break;
	case ImGuiKey_End:
		return VK_END;
		break;
	case ImGuiKey_Insert:
		return VK_INSERT;
		break;
	case ImGuiKey_Delete:
		return VK_DELETE;
		break;
	case ImGuiKey_Space:
		return VK_SPACE;
		break;
	case ImGuiKey_LeftCtrl:
		return VK_LCONTROL;
		break;
	case ImGuiKey_LeftShift:
		return VK_LSHIFT;
		break;
	case ImGuiKey_LeftAlt:
		return VK_LMENU;
		break;
	case ImGuiKey_LeftSuper:
		break;
	case ImGuiKey_RightCtrl:
		return VK_RCONTROL;
		break;
	case ImGuiKey_RightShift:
		return VK_RSHIFT;

		break;
	case ImGuiKey_RightAlt:
		return VK_RMENU;
		break;
	case ImGuiKey_RightSuper:
		break;
	case ImGuiKey_Menu:
		break;
	case ImGuiKey_0:
		return '0';
		break;
	case ImGuiKey_1:
		return '1';
		break;
	case ImGuiKey_2:
		return '2';
		break;
	case ImGuiKey_3:
		return '3';
		break;
	case ImGuiKey_4:
		return '4';
		break;
	case ImGuiKey_5:
		return '5';
		break;
	case ImGuiKey_6:
		return '6';
		break;
	case ImGuiKey_7:
		return '7';
		break;
	case ImGuiKey_8:
		return '8';
		break;
	case ImGuiKey_9:
		return '9';
		break;
	case ImGuiKey_A:
		return 'A';
		break;
	case ImGuiKey_B:
		return 'B';
		break;
	case ImGuiKey_C:
		return 'C';
		break;
	case ImGuiKey_D:
		return 'D';
		break;
	case ImGuiKey_E:
		return 'E';
		break;
	case ImGuiKey_F:
		return 'F';
		break;
	case ImGuiKey_G:
		return 'G';
		break;
	case ImGuiKey_H:
		return 'H';
		break;
	case ImGuiKey_I:
		return 'I';
		break;
	case ImGuiKey_J:
		return 'J';
		break;
	case ImGuiKey_K:
		return 'K';
		break;
	case ImGuiKey_L:
		return 'L';
		break;
	case ImGuiKey_M:
		return 'M';
		break;
	case ImGuiKey_N:
		return 'N';
		break;
	case ImGuiKey_O:
		return 'O';
		break;
	case ImGuiKey_P:
		return 'P';
		break;
	case ImGuiKey_Q:
		return 'Q';
		break;
	case ImGuiKey_R:
		return 'R';
		break;
	case ImGuiKey_S:
		return 'S';
		break;
	case ImGuiKey_T:
		return 'T';
		break;
	case ImGuiKey_U:
		return 'U';
		break;
	case ImGuiKey_V:
		return 'V';
		break;
	case ImGuiKey_W:
		return 'W';
		break;
	case ImGuiKey_X:
		return 'X';
		break;
	case ImGuiKey_Y:
		return 'Y';
		break;
	case ImGuiKey_Z:
		return 'Z';
		break;
	case ImGuiKey_F1:
		return VK_F1;
		break;
	case ImGuiKey_F2:
		return VK_F2;
		break;
	case ImGuiKey_F3:
		return VK_F3;
		break;
	case ImGuiKey_F4:
		return VK_F4;
		break;
	case ImGuiKey_F5:
		return VK_F5;
		break;
	case ImGuiKey_F6:
		return VK_F6;
		break;
	case ImGuiKey_F7:
		return VK_F7;
		break;
	case ImGuiKey_F8:
		return VK_F8;
		break;
	case ImGuiKey_F9:
		return VK_F9;
		break;
	case ImGuiKey_F10:
		return VK_F10;
		break;
	case ImGuiKey_F11:
		return VK_F11;
		break;
	case ImGuiKey_F12:
		return VK_F12;
		break;
	case ImGuiKey_F13:
		return VK_F13;
		break;
	case ImGuiKey_F14:
		return VK_F14;
		break;
	case ImGuiKey_F15:
		return VK_F15;
		break;
	case ImGuiKey_F16:
		return VK_F16;
		break;
	case ImGuiKey_F17:
		return VK_F17;
		break;
	case ImGuiKey_F18:
		return VK_F18;
		break;
	case ImGuiKey_F19:
		return VK_F19;
		break;
	case ImGuiKey_F20:
		return VK_F20;
		break;
	case ImGuiKey_F21:
		return VK_F21;
		break;
	case ImGuiKey_F22:
		return VK_F22;
		break;
	case ImGuiKey_F23:
		return VK_F23;
		break;
	case ImGuiKey_F24:
		return VK_F24;
		break;
	case ImGuiKey_Apostrophe:
		return '\'';
		break;
	case ImGuiKey_Comma:
		return ',';
		break;
	case ImGuiKey_Minus:
		return '-';
		break;
	case ImGuiKey_Period:
		return '.';
		break;
	case ImGuiKey_Slash:
		return '/';
		break;
	case ImGuiKey_Semicolon:
		return ';';
		break;
	case ImGuiKey_Equal:
		return '=';
		break;
	case ImGuiKey_LeftBracket:
		return '[';
		break;
	case ImGuiKey_Backslash:
		return '\\';
		break;
	case ImGuiKey_RightBracket:
		return ']';
		break;
	case ImGuiKey_GraveAccent:
		return '`';
		break;
	case ImGuiKey_CapsLock:
		return VK_CAPITAL;
		break;
	case ImGuiKey_ScrollLock:
		return VK_SCROLL;
		break;
	case ImGuiKey_NumLock:
		return VK_NUMLOCK;
		break;
	case ImGuiKey_PrintScreen:
		return VK_SNAPSHOT;
		break;
	case ImGuiKey_Pause:
		return VK_PAUSE;
		break;
	case ImGuiKey_Keypad0:
		return VK_NUMPAD0;
		break;
	case ImGuiKey_Keypad1:
		return VK_NUMPAD1;
		break;
	case ImGuiKey_Keypad2:
		return VK_NUMPAD2;
		break;
	case ImGuiKey_Keypad3:
		return VK_NUMPAD3;
		break;
	case ImGuiKey_Keypad4:
		return VK_NUMPAD4;
		break;
	case ImGuiKey_Keypad5:
		return VK_NUMPAD5;
		break;
	case ImGuiKey_Keypad6:
		return VK_NUMPAD6;
		break;
	case ImGuiKey_Keypad7:
		return VK_NUMPAD7;
		break;
	case ImGuiKey_Keypad8:
		return VK_NUMPAD8;
		break;
	case ImGuiKey_Keypad9:
		return VK_NUMPAD9;
		break;
	case ImGuiKey_KeypadDecimal:
		return VK_SUBTRACT;
		break;
	case ImGuiKey_KeypadDivide:
		return VK_DECIMAL;
		break;
	case ImGuiKey_KeypadMultiply:
		return VK_MULTIPLY;
		break;
	case ImGuiKey_KeypadSubtract:
		return VK_SUBTRACT;
		break;
	case ImGuiKey_KeypadAdd:
		return VK_ADD;
		break;
	default:
		return 0;
		break;
	}
}
#include <thread>

void TabsEvents()
{
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		for (auto& pTab : g_Include->m_vecTabs)
		{
			if (pTab->pKeyBind.uKey == 0)
				continue;

			if (!GetAsyncKeyState(ConvertImGuiKeyToChar((ImGuiKey)pTab->pKeyBind.uKey)) && !ImGui::IsKeyPressed((ImGuiKey)pTab->pKeyBind.uKey))
				continue;


			for (int i = 0; i < pTab->GetBoxSize(); i++)
			{
				Sleep(100);

				for (int iFiles = 0; iFiles < pTab->GetBox(i).GetFilesSize(); iFiles++)
				{
					std::string strFilePath = pTab->GetBox(i).GetFileByIndex(iFiles);
					g_Clipboard->CopyFileToClipboard(strFilePath);
					Sleep(100);
					g_Clipboard->CtrlV();
					Sleep(100);
				}
				Sleep(100);
				std::string strText = std::string(pTab->GetText(i).begin(), pTab->GetText(i).end());
				g_Clipboard->TextToClipboard(utf8_to_wstring(strText));
				Sleep(100);
				g_Clipboard->CtrlV();
				Sleep(100);

				g_Clipboard->Enter();
			}

		}
	}

}

// Main code
int main(int, char**)
{
	// Create application window
	//ImGui_ImplWin32_EnableDpiAwareness();
	WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Autoruss Helper", nullptr };
	::RegisterClassExW(&wc);
	HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Autoruss Helper", WS_OVERLAPPEDWINDOW, 100, 100, g_Include->vecWindowSize.x, g_Include->vecWindowSize.y, nullptr, nullptr, wc.hInstance, nullptr);
	::ShowWindow(::GetConsoleWindow(), SW_HIDE);
	// Initialize Direct3D
	if (!CreateDeviceD3D(hwnd))
	{
		CleanupDeviceD3D();
		::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	// Show the window
	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	HANDLE hEventThread = CreateThread(NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(TabsEvents), 0, 0, NULL);

	if (hEventThread != nullptr)
		CloseHandle(hEventThread);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	g_UI->pTextFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Arial.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesCyrillic());
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != nullptr);

	C_Tab* pMainTab = new C_Tab("Main");
	pMainTab->SetIndex(0);
	g_Include->m_vecTabs.push_back(pMainTab);

	// Main loop
	bool done = false;
	while (!done)
	{
		// Poll and handle messages (inputs, window resize, etc.)
		// See the WndProc() function below for our to dispatch events to the Win32 backend.
		MSG msg;
		while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
				done = true;
		}
		if (done)
			break;

		// Handle window being minimized or screen locked
		if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
		{
			::Sleep(10);
			continue;
		}
		g_SwapChainOccluded = false;

		// Handle window resize (we don't resize directly in the WM_SIZE handler)
		if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
		{
			CleanupRenderTarget();
			g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
			g_ResizeWidth = g_ResizeHeight = 0;
			CreateRenderTarget();
		}

		RECT rect;
		if (GetWindowRect(hwnd, &rect))
		{
			int width = rect.right - rect.left;
			int height = rect.bottom - rect.top;

			g_Include->vecWindowSize.x = width;
			g_Include->vecWindowSize.y = height;
		}

		if (g_Settings->bChangeStyle)
			ImGui::StyleColorsLight();
		else
			ImGui::StyleColorsDark();

		// Start the Dear ImGui frame
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		ImGui::SetNextWindowPos(ImVec2(-1, -1), ImGuiCond_Always);
		ImGui::SetNextWindowSize(g_Include->vecWindowSize, ImGuiCond_Always);

		auto* pStyle = &ImGui::GetStyle();
		pStyle->ChildRounding = 5.f;
		// pStyle->Colors[ImGuiCol_ChildBg] = g_Settings->m_vecChildColor;

		 // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
		{
			bool bOpen = true;
			ImGui::Begin("Hello, world!", &bOpen, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
			RenderTabs();

			ImGui::SetNextWindowPos(ImVec2(175, 7), ImGuiCond_Always);
			RenderTexts();

			ImGui::End();
		}

		// Rendering
		ImGui::Render();
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		// Present
		HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
		//HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
		g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
	}

	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	::DestroyWindow(hwnd);
	::UnregisterClassW(wc.lpszClassName, wc.hInstance);

	return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT createDeviceFlags = 0;
	//createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
		res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if (res != S_OK)
		return false;

	CreateRenderTarget();
	return true;
}

void CleanupDeviceD3D()
{
	CleanupRenderTarget();
	if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
	if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
	ID3D11Texture2D* pBackBuffer;
	g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
	pBackBuffer->Release();
}

void CleanupRenderTarget()
{
	if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED)
			return 0;
		g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
		g_ResizeHeight = (UINT)HIWORD(lParam);
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
