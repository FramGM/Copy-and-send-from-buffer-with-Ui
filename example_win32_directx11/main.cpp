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
#include <imgui_internal.h>

// Data
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;
ImVec2 vecWindowSize = ImVec2(1000, 700);

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class C_TextBox
{
public:
    ImVector<char>& GetText() { return m_vecText; }
    void SetText(ImVector<char> vecText) { m_vecText = vecText; }

    void AddFile(std::string strPath) { m_vecFilesPaths.push_back(strPath); }
    std::string GetFileByIndex(int iIndex) { return m_vecFilesPaths.at(iIndex); }

private:
    ImVector<char> m_vecText;
    std::vector<std::string> m_vecFilesPaths;
};

struct KeyBind_t
{
    constexpr KeyBind_t(const char* szName, const unsigned int uKey = 0U) :
        szName(szName), uKey(uKey) {
    }

    bool bEnable = false;
    const char* szName = nullptr;
    unsigned int uKey = 0U;
};

bool GetBindState(KeyBind_t& keyBind)
{
    if (keyBind.uKey == 0U)
        return false;

    keyBind.bEnable = GetAsyncKeyState(keyBind.uKey);

    return keyBind.bEnable;
}

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

std::vector<C_Tab*> m_vecTabs;
int m_iCurrentTab = 0;

void RenderTabs()
{
    ImGui::BeginChild("Test tab", ImVec2(165, vecWindowSize.y - 55), ImGuiChildFlags_Borders);
    {
        for (int iTab = 0; iTab < m_vecTabs.size(); iTab++)
        {
            if (ImGui::Button(m_vecTabs.at(iTab)->GetName().c_str(), ImVec2(147, 40)))
                m_iCurrentTab = iTab;
        }

        if (ImGui::Button("+", ImVec2(147, 40)))
            m_iCurrentTab = INT_MAX;

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
        pNewTab->SetIndex(m_vecTabs.size() - 1);

        m_vecTabs.push_back(pNewTab);
        m_iCurrentTab = m_vecTabs.size() - 1;

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
    ImGui::BeginChild("Text Child", ImVec2(vecWindowSize.x - 205, vecWindowSize.y - 55), ImGuiChildFlags_Borders);
    {
        if (!m_iCurrentTab)
            RenderMainTab();
        else if (m_iCurrentTab == INT_MAX)
            AddConfig();

        if (m_iCurrentTab && m_iCurrentTab != INT_MAX)
        {
            C_Tab* pCurrentTab = m_vecTabs.at(m_iCurrentTab);
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
                m_vecTabs.erase(m_vecTabs.begin() + m_iCurrentTab);
                m_iCurrentTab = 0;
            }

            ImGui::HotKey("key", &pCurrentTab->pKeyBind.uKey);
            if (pCurrentTab->pKeyBind.uKey != 0)
                pCurrentTab->pKeyBind.szName = ImGui::GetKeyName((ImGuiKey)pCurrentTab->pKeyBind.uKey);

            ImGui::NewLine();

            for (int i = 0; i < pCurrentTab->GetBoxSize(); i++)
            {

                std::string strBoxName = "Text##"+ std::to_string(i);
                ImVector<char> strText = pCurrentTab->GetText(i);
                MyInputTextMultiline(strBoxName.c_str(), &strText);
                pCurrentTab->SetText(i, strText);

                AddFileButton(i,pCurrentTab);
                ImGui::NewLine();
            }

            
        }

    }
    ImGui::EndChild();
}

void TabsEvents()
{
    for (auto& pTab : m_vecTabs)
    {
        if (pTab->pKeyBind.uKey == 0)
            continue;

        if (!ImGui::IsKeyPressed((ImGuiKey)pTab->pKeyBind.uKey))
            continue;

        //do smth

    }
}

// Main code
int main(int, char**)
{
    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Autoruss Helper", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Autoruss Helper", WS_OVERLAPPEDWINDOW, 100, 100, vecWindowSize.x, vecWindowSize.y, nullptr, nullptr, wc.hInstance, nullptr);

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
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    C_Tab* pMainTab = new C_Tab("Main");
    pMainTab->SetIndex(0);
    m_vecTabs.push_back(pMainTab);
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

            vecWindowSize.x = width;
            vecWindowSize.y = height;
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
        ImGui::SetNextWindowSize(vecWindowSize, ImGuiCond_Always);

        auto* pStyle = &ImGui::GetStyle();
        pStyle->ChildRounding = 5.f;
       // pStyle->Colors[ImGuiCol_ChildBg] = g_Settings->m_vecChildColor;

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            bool bOpen = true;
            ImGui::Begin("Hello, world!",&bOpen, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
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
