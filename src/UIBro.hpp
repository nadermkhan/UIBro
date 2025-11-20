// UIBro.hpp - Production Win32 Framework with WTL & Windows 10 Native UI
// Version: 2.0 (WTL Edition)
// License: MIT
// Author: Nader Mahbub Khan

#ifndef UIBRO_HPP
#define UIBRO_HPP

#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

// WTL Requirements
#define _WTL_NO_CSTRING
#define _WTL_NO_WTYPES
#define _ATL_NO_HOSTING

#include <atlbase.h>
#include <atlapp.h>

extern CAppModule _Module;

#include <atlwin.h>
#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlctrlw.h>
#include <atlmisc.h>
#include <atlcrack.h>
#include <atltheme.h>

#include <iostream>
#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <map>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <queue>
#include <atomic>
#include <chrono>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <regex>
#include <variant>
#include <stdexcept>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "shell32.lib")

// Initialize WTL App Module
CAppModule _Module;

namespace UIBro {

// ============================================================================
// DPI AWARENESS MANAGER
// ============================================================================
class DPIManager {
private:
    static int dpi_;
    static float scale_;
    
public:
    static void Initialize() {
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        
        HDC screen = ::GetDC(NULL);
        dpi_ = GetDeviceCaps(screen, LOGPIXELSX);
        ::ReleaseDC(NULL, screen);
        
        scale_ = dpi_ / 96.0f;
    }
    
    static int Scale(int value) {
        return static_cast<int>(value * scale_);
    }
    
    static int GetDPI() { return dpi_; }
    static float GetScale() { return scale_; }
    
    static HFONT CreateScaledFont(int size, int weight = FW_NORMAL, const wchar_t* face = L"Segoe UI") {
        return CreateFontW(
            -MulDiv(size, dpi_, 72),
            0, 0, 0, weight, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            face
        );
    }
};

int DPIManager::dpi_ = 96;
float DPIManager::scale_ = 1.0f;

// ============================================================================
// THREAD-SAFE QUEUE
// ============================================================================
template<typename T>
class ThreadSafeQueue {
private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    
public:
    void push(T item) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(item));
    }
    
    bool pop(T& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) return false;
        item = std::move(queue_.front());
        queue_.pop();
        return true;
    }
    
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
};

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================
class Component;
class Button;
class Input;
class Label;
class CheckBox;
class GroupBox;
class ComboBox;
class ProgressBar;
class Window;
class Notification;

// ============================================================================
// COMPONENT BASE CLASS (WTL Wrapper)
// ============================================================================
class Component {
protected:
    HWND hwnd_ = nullptr;
    HWND parent_ = nullptr;
    int x_ = 0, y_ = 0, width_ = 100, height_ = 30;
    std::wstring text_;
    bool visible_ = true;
    bool enabled_ = true;
    WTL::CFont font_;
    std::function<void()> onClick_;
    std::function<void(const std::wstring&)> onChange_;
    std::wstring id_;
    
public:
    virtual ~Component() {
        if (hwnd_ && ::IsWindow(hwnd_)) {
            ::DestroyWindow(hwnd_);
        }
    }
    
    HWND getHandle() const { return hwnd_; }
    std::wstring getId() const { return id_; }
    
    Component* position(int x, int y) {
        x_ = DPIManager::Scale(x);
        y_ = DPIManager::Scale(y);
        if (hwnd_ && ::IsWindow(hwnd_)) {
            ::SetWindowPos(hwnd_, NULL, x_, y_, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        return this;
    }
    
    Component* size(int width, int height) {
        width_ = DPIManager::Scale(width);
        height_ = DPIManager::Scale(height);
        if (hwnd_ && ::IsWindow(hwnd_)) {
            ::SetWindowPos(hwnd_, NULL, 0, 0, width_, height_, SWP_NOMOVE | SWP_NOZORDER);
        }
        return this;
    }
    
    Component* text(const std::wstring& txt) {
        text_ = txt;
        if (hwnd_ && ::IsWindow(hwnd_)) {
            ::SetWindowTextW(hwnd_, text_.c_str());
        }
        return this;
    }
    
    std::wstring text() const {
        if (!hwnd_ || !::IsWindow(hwnd_)) return text_;
        int len = ::GetWindowTextLengthW(hwnd_);
        if (len == 0) return L"";
        std::vector<wchar_t> buffer(len + 1);
        ::GetWindowTextW(hwnd_, buffer.data(), len + 1);
        return std::wstring(buffer.data());
    }
    
    Component* id(const std::wstring& identifier) {
        id_ = identifier;
        return this;
    }
    
    Component* show(bool visible = true) {
        visible_ = visible;
        if (hwnd_ && ::IsWindow(hwnd_)) {
            ::ShowWindow(hwnd_, visible ? SW_SHOW : SW_HIDE);
        }
        return this;
    }
    
    Component* enable(bool enabled = true) {
        enabled_ = enabled;
        if (hwnd_ && ::IsWindow(hwnd_)) {
            ::EnableWindow(hwnd_, enabled);
        }
        return this;
    }
    
    bool isVisible() const { return visible_; }
    bool isEnabled() const { return enabled_; }
    
protected:
    void setDefaultFont() {
        if (!font_.IsNull()) font_.DeleteObject();
        font_.Attach(DPIManager::CreateScaledFont(9, FW_NORMAL, L"Segoe UI"));
        if (hwnd_ && ::IsWindow(hwnd_)) {
            ::SendMessage(hwnd_, WM_SETFONT, (WPARAM)(HFONT)font_, TRUE);
        }
    }
};

// ============================================================================
// BUTTON COMPONENT (WTL)
// ============================================================================
class Button : public Component {
private:
    WTL::CButton btn_;
    bool isDefault_ = false;
    
public:
    Button(HWND parent, const std::wstring& text) {
        parent_ = parent;
        text_ = text;
        width_ = DPIManager::Scale(100);
        height_ = DPIManager::Scale(30);
        
        btn_.Create(parent_, WTL::CRect(x_, y_, x_ + width_, y_ + height_),
                   text_.c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON);
        
        hwnd_ = btn_.m_hWnd;
        
        if (!hwnd_) {
            throw std::runtime_error("Failed to create button");
        }
        
        ::SetWindowTheme(hwnd_, L"Explorer", NULL);
        setDefaultFont();
    }
    
    Button* position(int x, int y) { 
        Component::position(x, y);
        if (btn_.IsWindow()) {
            btn_.SetWindowPos(NULL, x_, y_, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        return this; 
    }
    
    Button* size(int w, int h) { 
        Component::size(w, h);
        if (btn_.IsWindow()) {
            btn_.SetWindowPos(NULL, 0, 0, width_, height_, SWP_NOMOVE | SWP_NOZORDER);
        }
        return this; 
    }
    
    Button* text(const std::wstring& txt) {
        Component::text(txt);
        return this;
    }
    
    Button* onClick(std::function<void()> callback) { 
        onClick_ = callback; 
        return this; 
    }
    
    Button* setDefault(bool isDefault = true) {
        isDefault_ = isDefault;
        if (btn_.IsWindow()) {
            LONG style = btn_.GetWindowLong(GWL_STYLE);
            style &= ~(BS_DEFPUSHBUTTON | BS_PUSHBUTTON);
            style |= isDefault ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON;
            btn_.SetWindowLong(GWL_STYLE, style);
            btn_.Invalidate();
        }
        return this;
    }
    
    void triggerClick() {
        if (onClick_) onClick_();
    }
};

// ============================================================================
// INPUT (EDIT CONTROL) - WTL
// ============================================================================
class Input : public Component {
private:
    WTL::CEdit edit_;
    std::wstring placeholder_;
    bool multiline_ = false;
    bool password_ = false;
    bool readonly_ = false;
    
public:
    Input(HWND parent, const std::wstring& placeholder = L"") {
        parent_ = parent;
        placeholder_ = placeholder;
        width_ = DPIManager::Scale(200);
        height_ = DPIManager::Scale(24);
        
        DWORD style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL;
        
        edit_.Create(parent_, WTL::CRect(x_, y_, x_ + width_, y_ + height_),
                    L"", style, WS_EX_CLIENTEDGE);
        
        hwnd_ = edit_.m_hWnd;
        
        if (!hwnd_) {
            throw std::runtime_error("Failed to create input");
        }
        
        setDefaultFont();
        
        if (!placeholder_.empty()) {
            edit_.SetCueBannerText(placeholder_.c_str());
        }
    }
    
    Input* position(int x, int y) { 
        Component::position(x, y);
        if (edit_.IsWindow()) {
            edit_.SetWindowPos(NULL, x_, y_, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        return this; 
    }
    
    Input* size(int w, int h) { 
        Component::size(w, h);
        if (edit_.IsWindow()) {
            edit_.SetWindowPos(NULL, 0, 0, width_, height_, SWP_NOMOVE | SWP_NOZORDER);
        }
        return this; 
    }
    
    Input* onChange(std::function<void(const std::wstring&)> callback) { 
        onChange_ = callback; 
        return this; 
    }
    
    Input* multiline(bool enable = true) {
        multiline_ = enable;
        if (edit_.IsWindow()) {
            DWORD style = edit_.GetWindowLong(GWL_STYLE);
            if (enable) {
                style |= ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL;
            } else {
                style &= ~(ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL);
            }
            edit_.SetWindowLong(GWL_STYLE, style);
        }
        return this;
    }
    
    Input* password(bool enable = true) {
        password_ = enable;
        if (edit_.IsWindow()) {
            LONG style = edit_.GetWindowLong(GWL_STYLE);
            if (enable) {
                style |= ES_PASSWORD;
            } else {
                style &= ~ES_PASSWORD;
            }
            edit_.SetWindowLong(GWL_STYLE, style);
        }
        return this;
    }
    
    Input* readonly(bool enable = true) {
        readonly_ = enable;
        if (edit_.IsWindow()) {
            edit_.SetReadOnly(enable);
        }
        return this;
    }
    
    std::wstring getValue() const {
        if (!edit_.IsWindow()) return L"";
        WTL::CString str;
        edit_.GetWindowText(str);
        return str.GetString();
    }
    
    void setValue(const std::wstring& value) {
        text(value);
    }
    
    void triggerChange() {
        if (onChange_) onChange_(getValue());
    }
};

// ============================================================================
// LABEL (STATIC TEXT) - WTL
// ============================================================================
class Label : public Component {
private:
    WTL::CStatic static_;
    
public:
    Label(HWND parent, const std::wstring& text) {
        parent_ = parent;
        text_ = text;
        width_ = DPIManager::Scale(200);
        height_ = DPIManager::Scale(20);
        
        static_.Create(parent_, WTL::CRect(x_, y_, x_ + width_, y_ + height_),
                      text_.c_str(), WS_CHILD | WS_VISIBLE | SS_LEFT);
        
        hwnd_ = static_.m_hWnd;
        
        if (!hwnd_) {
            throw std::runtime_error("Failed to create label");
        }
        
        setDefaultFont();
    }
    
    Label* position(int x, int y) { 
        Component::position(x, y);
        if (static_.IsWindow()) {
            static_.SetWindowPos(NULL, x_, y_, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        return this; 
    }
    
    Label* size(int w, int h) { 
        Component::size(w, h);
        if (static_.IsWindow()) {
            static_.SetWindowPos(NULL, 0, 0, width_, height_, SWP_NOMOVE | SWP_NOZORDER);
        }
        return this; 
    }
    
    Label* text(const std::wstring& txt) {
        Component::text(txt);
        return this;
    }
    
    Label* font(int size, int weight = FW_NORMAL, const wchar_t* face = L"Segoe UI") {
        if (!font_.IsNull()) font_.DeleteObject();
        font_.Attach(DPIManager::CreateScaledFont(size, weight, face));
        if (hwnd_ && ::IsWindow(hwnd_)) {
            ::SendMessage(hwnd_, WM_SETFONT, (WPARAM)(HFONT)font_, TRUE);
        }
        return this;
    }
    
    Label* bold(bool enable = true) {
        return font(9, enable ? FW_BOLD : FW_NORMAL);
    }
};

// ============================================================================
// CHECKBOX - WTL
// ============================================================================
class CheckBox : public Component {
private:
    WTL::CButton check_;
    bool checked_ = false;
    
public:
    CheckBox(HWND parent, const std::wstring& text) {
        parent_ = parent;
        text_ = text;
        width_ = DPIManager::Scale(200);
        height_ = DPIManager::Scale(20);
        
        check_.Create(parent_, WTL::CRect(x_, y_, x_ + width_, y_ + height_),
                     text_.c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX);
        
        hwnd_ = check_.m_hWnd;
        
        if (!hwnd_) {
            throw std::runtime_error("Failed to create checkbox");
        }
        
        ::SetWindowTheme(hwnd_, L"Explorer", NULL);
        setDefaultFont();
    }
    
    CheckBox* position(int x, int y) { 
        Component::position(x, y);
        if (check_.IsWindow()) {
            check_.SetWindowPos(NULL, x_, y_, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        return this; 
    }
    
    CheckBox* size(int w, int h) { 
        Component::size(w, h);
        if (check_.IsWindow()) {
            check_.SetWindowPos(NULL, 0, 0, width_, height_, SWP_NOMOVE | SWP_NOZORDER);
        }
        return this; 
    }
    
    CheckBox* onChange(std::function<void(bool)> callback) {
        onChange_ = [callback](const std::wstring&) {};
        return this;
    }
    
    bool isChecked() const {
        if (!check_.IsWindow()) return checked_;
        return check_.GetCheck() == BST_CHECKED;
    }
    
    CheckBox* setChecked(bool checked) {
        checked_ = checked;
        if (check_.IsWindow()) {
            check_.SetCheck(checked ? BST_CHECKED : BST_UNCHECKED);
        }
        return this;
    }
    
    void triggerChange() {}
};

// ============================================================================
// COMBOBOX (DROPDOWN) - WTL
// ============================================================================
class ComboBox : public Component {
private:
    WTL::CComboBox combo_;
    std::vector<std::wstring> items_;
    
public:
    ComboBox(HWND parent) {
        parent_ = parent;
        width_ = DPIManager::Scale(200);
        height_ = DPIManager::Scale(200);
        
        combo_.Create(parent_, WTL::CRect(x_, y_, x_ + width_, y_ + height_),
                     NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL);
        
        hwnd_ = combo_.m_hWnd;
        
        if (!hwnd_) {
            throw std::runtime_error("Failed to create combobox");
        }
        
        setDefaultFont();
    }
    
    ComboBox* position(int x, int y) { 
        Component::position(x, y);
        if (combo_.IsWindow()) {
            combo_.SetWindowPos(NULL, x_, y_, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        return this; 
    }
    
    ComboBox* size(int w, int h) { 
        Component::size(w, h);
        if (combo_.IsWindow()) {
            combo_.SetWindowPos(NULL, 0, 0, width_, height_, SWP_NOMOVE | SWP_NOZORDER);
        }
        return this; 
    }
    
    ComboBox* addItem(const std::wstring& item) {
        items_.push_back(item);
        if (combo_.IsWindow()) {
            combo_.AddString(item.c_str());
        }
        return this;
    }
    
    ComboBox* setSelectedIndex(int index) {
        if (combo_.IsWindow()) {
            combo_.SetCurSel(index);
        }
        return this;
    }
    
    int getSelectedIndex() const {
        if (!combo_.IsWindow()) return -1;
        return combo_.GetCurSel();
    }
    
    std::wstring getSelectedText() const {
        int index = getSelectedIndex();
        if (index < 0 || index >= (int)items_.size()) return L"";
        return items_[index];
    }
    
    void triggerChange() {
        if (onChange_) onChange_(getSelectedText());
    }
};

// ============================================================================
// PROGRESSBAR - WTL
// ============================================================================
class ProgressBar : public Component {
private:
    WTL::CProgressBarCtrl progress_;
    int min_ = 0;
    int max_ = 100;
    int value_ = 0;
    
public:
    ProgressBar(HWND parent) {
        parent_ = parent;
        width_ = DPIManager::Scale(200);
        height_ = DPIManager::Scale(20);
        
        progress_.Create(parent_, WTL::CRect(x_, y_, x_ + width_, y_ + height_),
                        NULL, WS_CHILD | WS_VISIBLE | PBS_SMOOTH);
        
        hwnd_ = progress_.m_hWnd;
        
        if (!hwnd_) {
            throw std::runtime_error("Failed to create progressbar");
        }
        
        ::SetWindowTheme(hwnd_, L"Explorer", NULL);
        progress_.SetRange(min_, max_);
    }
    
    ProgressBar* position(int x, int y) { 
        Component::position(x, y);
        if (progress_.IsWindow()) {
            progress_.SetWindowPos(NULL, x_, y_, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        return this; 
    }
    
    ProgressBar* size(int w, int h) { 
        Component::size(w, h);
        if (progress_.IsWindow()) {
            progress_.SetWindowPos(NULL, 0, 0, width_, height_, SWP_NOMOVE | SWP_NOZORDER);
        }
        return this; 
    }
    
    ProgressBar* setRange(int min, int max) {
        min_ = min;
        max_ = max;
        if (progress_.IsWindow()) {
            progress_.SetRange(min_, max_);
        }
        return this;
    }
    
    ProgressBar* setValue(int value) {
        value_ = value;
        if (progress_.IsWindow()) {
            progress_.SetPos(value_);
        }
        return this;
    }
    
    int getValue() const { return value_; }
};

// ============================================================================
// GROUPBOX - WTL
// ============================================================================
class GroupBox : public Component {
private:
    WTL::CButton group_;
    std::vector<std::shared_ptr<Component>> children_;
    
public:
    GroupBox(HWND parent, const std::wstring& text = L"") {
        parent_ = parent;
        text_ = text;
        width_ = DPIManager::Scale(300);
        height_ = DPIManager::Scale(200);
        
        group_.Create(parent_, WTL::CRect(x_, y_, x_ + width_, y_ + height_),
                     text_.c_str(), WS_CHILD | WS_VISIBLE | BS_GROUPBOX);
        
        hwnd_ = group_.m_hWnd;
        
        if (!hwnd_) {
            throw std::runtime_error("Failed to create groupbox");
        }
        
        setDefaultFont();
    }
    
    GroupBox* position(int x, int y) { 
        Component::position(x, y);
        if (group_.IsWindow()) {
            group_.SetWindowPos(NULL, x_, y_, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        return this; 
    }
    
    GroupBox* size(int w, int h) { 
        Component::size(w, h);
        if (group_.IsWindow()) {
            group_.SetWindowPos(NULL, 0, 0, width_, height_, SWP_NOMOVE | SWP_NOZORDER);
        }
        return this; 
    }
    
    Label* addLabel(const std::wstring& text) {
        auto c = std::make_shared<Label>(hwnd_, text);
        children_.push_back(c);
        return c.get();
    }
    
    Button* addButton(const std::wstring& text) {
        auto c = std::make_shared<Button>(hwnd_, text);
        children_.push_back(c);
        return c.get();
    }
    
    Input* addInput(const std::wstring& placeholder = L"") {
        auto c = std::make_shared<Input>(hwnd_, placeholder);
        children_.push_back(c);
        return c.get();
    }
    
    CheckBox* addCheckBox(const std::wstring& text) {
        auto c = std::make_shared<CheckBox>(hwnd_, text);
        children_.push_back(c);
        return c.get();
    }
    
    ComboBox* addComboBox() {
        auto c = std::make_shared<ComboBox>(hwnd_);
        children_.push_back(c);
        return c.get();
    }
    
    ProgressBar* addProgressBar() {
        auto c = std::make_shared<ProgressBar>(hwnd_);
        children_.push_back(c);
        return c.get();
    }
};

// ============================================================================
// SIMPLIFIED NOTIFICATION SYSTEM
// ============================================================================
class Notification {
private:
    static bool initialized_;
    static std::wstring appId_;
    
public:
    static void show(const std::wstring& title, const std::wstring& message, int duration = 3000) {
        std::thread([title, message]() {
            ::MessageBoxW(NULL, message.c_str(), title.c_str(), 
                         MB_OK | MB_ICONINFORMATION | MB_TOPMOST | MB_SETFOREGROUND);
        }).detach();
    }
    
    static void showError(const std::wstring& title, const std::wstring& message) {
        std::thread([title, message]() {
            ::MessageBoxW(NULL, message.c_str(), title.c_str(), 
                         MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
        }).detach();
    }
    
    static void showWarning(const std::wstring& title, const std::wstring& message) {
        std::thread([title, message]() {
            ::MessageBoxW(NULL, message.c_str(), title.c_str(), 
                         MB_OK | MB_ICONWARNING | MB_TOPMOST | MB_SETFOREGROUND);
        }).detach();
    }
    
    static void showQuestion(const std::wstring& title, const std::wstring& message, 
                            std::function<void(bool)> callback = nullptr) {
        std::thread([title, message, callback]() {
            int result = ::MessageBoxW(NULL, message.c_str(), title.c_str(), 
                                      MB_YESNO | MB_ICONQUESTION | MB_TOPMOST | MB_SETFOREGROUND);
            if (callback) {
                callback(result == IDYES);
            }
        }).detach();
    }
};

bool Notification::initialized_ = false;
std::wstring Notification::appId_ = L"UIBro.WTL.Application";

// ============================================================================
// MAIN WINDOW - WTL CFrameWindowImpl
// ============================================================================
class Window : public WTL::CFrameWindowImpl<Window> {
private:
    std::wstring title_ = L"UIBro WTL Application";
    int width_ = 800;
    int height_ = 600;
    bool centerOnScreen_ = true;
    std::vector<std::shared_ptr<Component>> components_;
    std::map<std::wstring, Component*> componentMap_;
    ThreadSafeQueue<std::function<void()>> messageQueue_;
    std::atomic<bool> running_{false};
    std::thread messageThread_;
    WTL::CBrush bgBrush_;
    
public:
    DECLARE_FRAME_WND_CLASS(L"UIBroWTLWindow", IDR_MAINFRAME)
    
    BEGIN_MSG_MAP(Window)
        MSG_WM_CREATE(OnCreate)
        MSG_WM_DESTROY(OnDestroy)
        MSG_WM_CLOSE(OnClose)
        MSG_WM_CTLCOLORSTATIC(OnCtlColorStatic)
        MSG_WM_DPICHANGED(OnDpiChanged)
        COMMAND_CODE_HANDLER(BN_CLICKED, OnButtonClicked)
        COMMAND_CODE_HANDLER(EN_CHANGE, OnEditChanged)
        COMMAND_CODE_HANDLER(CBN_SELCHANGE, OnComboSelChange)
        CHAIN_MSG_MAP(WTL::CFrameWindowImpl<Window>)
    END_MSG_MAP()
    
    Window() {
        bgBrush_.CreateSolidBrush(RGB(240, 240, 240));
    }
    
    ~Window() {
        running_ = false;
        if (messageThread_.joinable()) {
            messageThread_.join();
        }
    }
    
    int OnCreate(LPCREATESTRUCT lpcs) {
        ::SetWindowTheme(m_hWnd, L"Explorer", NULL);
        
        BOOL useDarkMode = FALSE;
        DwmSetWindowAttribute(m_hWnd, 19, &useDarkMode, sizeof(useDarkMode));
        
        running_ = true;
        messageThread_ = std::thread(&Window::processMessageQueue, this);
        
        return 0;
    }
    
    void OnDestroy() {
        running_ = false;
        ::PostQuitMessage(0);
    }
    
    void OnClose() {
        DestroyWindow();
    }
    
    HBRUSH OnCtlColorStatic(WTL::CDCHandle dc, WTL::CStatic wndStatic) {
        dc.SetBkColor(RGB(240, 240, 240));
        return bgBrush_;
    }
    
    void OnDpiChanged(UINT nDpiX, UINT nDpiY, PRECT pRect) {
        DPIManager::Initialize();
        ::SetWindowPos(m_hWnd, NULL, 
            pRect->left, pRect->top,
            pRect->right - pRect->left,
            pRect->bottom - pRect->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
    }
    
    LRESULT OnButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
        for (auto& comp : components_) {
            if (comp->getHandle() == hWndCtl) {
                if (auto btn = dynamic_cast<Button*>(comp.get())) {
                    btn->triggerClick();
                }
                else if (auto chk = dynamic_cast<CheckBox*>(comp.get())) {
                    chk->triggerChange();
                }
                break;
            }
        }
        return 0;
    }
    
    LRESULT OnEditChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
        for (auto& comp : components_) {
            if (comp->getHandle() == hWndCtl) {
                if (auto input = dynamic_cast<Input*>(comp.get())) {
                    input->triggerChange();
                }
                break;
            }
        }
        return 0;
    }
    
    LRESULT OnComboSelChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
        for (auto& comp : components_) {
            if (comp->getHandle() == hWndCtl) {
                if (auto combo = dynamic_cast<ComboBox*>(comp.get())) {
                    combo->triggerChange();
                }
                break;
            }
        }
        return 0;
    }
    
    void processMessageQueue() {
        while (running_) {
            std::function<void()> task;
            if (messageQueue_.pop(task)) {
                task();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
    Window* title(const std::wstring& t) { 
        title_ = t; 
        if (IsWindow()) {
            SetWindowText(title_.c_str());
        }
        return this; 
    }
    
    Window* size(int w, int h) { 
        width_ = w; 
        height_ = h; 
        return this; 
    }
    
    Window* center(bool enable = true) { 
        centerOnScreen_ = enable; 
        return this; 
    }
    
    HWND getHandle() const { 
        return m_hWnd; 
    }
    
    Label* addLabel(const std::wstring& text) {
        auto c = std::make_shared<Label>(m_hWnd, text);
        components_.push_back(c);
        if (!c->getId().empty()) {
            componentMap_[c->getId()] = c.get();
        }
        return c.get();
    }
    
    Button* addButton(const std::wstring& text) {
        auto c = std::make_shared<Button>(m_hWnd, text);
        components_.push_back(c);
        if (!c->getId().empty()) {
            componentMap_[c->getId()] = c.get();
        }
        return c.get();
    }
    
    Input* addInput(const std::wstring& placeholder = L"") {
        auto c = std::make_shared<Input>(m_hWnd, placeholder);
        components_.push_back(c);
        if (!c->getId().empty()) {
            componentMap_[c->getId()] = c.get();
        }
        return c.get();
    }
    
    CheckBox* addCheckBox(const std::wstring& text) {
        auto c = std::make_shared<CheckBox>(m_hWnd, text);
        components_.push_back(c);
        if (!c->getId().empty()) {
            componentMap_[c->getId()] = c.get();
        }
        return c.get();
    }
    
    ComboBox* addComboBox() {
        auto c = std::make_shared<ComboBox>(m_hWnd);
        components_.push_back(c);
        if (!c->getId().empty()) {
            componentMap_[c->getId()] = c.get();
        }
        return c.get();
    }
    
    ProgressBar* addProgressBar() {
        auto c = std::make_shared<ProgressBar>(m_hWnd);
        components_.push_back(c);
        if (!c->getId().empty()) {
            componentMap_[c->getId()] = c.get();
        }
        return c.get();
    }
    
    GroupBox* addGroupBox(const std::wstring& text = L"") {
        auto c = std::make_shared<GroupBox>(m_hWnd, text);
        components_.push_back(c);
        if (!c->getId().empty()) {
            componentMap_[c->getId()] = c.get();
        }
        return c.get();
    }
    
    Component* findById(const std::wstring& id) {
        auto it = componentMap_.find(id);
        return (it != componentMap_.end()) ? it->second : nullptr;
    }
    
    void runAsync(std::function<void()> task) {
        messageQueue_.push(task);
    }
    
    void create() {
        if (IsWindow()) return;
        
        WTL::CRect rect(0, 0, width_, height_);
        
        HWND hwnd = Create(NULL, rect, title_.c_str(), 
                          WS_OVERLAPPEDWINDOW | WS_VISIBLE);
        
        if (!hwnd) {
            throw std::runtime_error("Failed to create window");
        }
        
        if (centerOnScreen_) {
            CenterWindow();
        }
        
        ShowWindow(SW_SHOW);
        UpdateWindow();
    }
    
    int run() {
        if (!IsWindow()) create();
        
        WTL::CMessageLoop theLoop;
        _Module.AddMessageLoop(&theLoop);
        
        int nRet = theLoop.Run();
        
        _Module.RemoveMessageLoop();
        return nRet;
    }
};

} // namespace UIBro

#endif // UIBRO_HPP
