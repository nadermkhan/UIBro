// UIBro.hpp - Production Win32 Framework with Windows 10 Native UI & Toast Notifications
// Version: 1.0
// License: MIT
// Author: Nader Mahbub Khan

#ifndef UIBRO_HPP
#define UIBRO_HPP

#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <iostream>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <shlobj.h>
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
#include <cctype>
#include <variant>
#include <stdexcept>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

// Windows 10 Toast Notification imports
#include <wrl/client.h>
#include <wrl/implements.h>
#include <windows.ui.notifications.h>
#include <notificationactivationcallback.h>
#include <roapi.h>

#pragma comment(lib, "runtimeobject.lib")

using namespace Microsoft::WRL;
using namespace ABI::Windows::Data::Xml::Dom;
using namespace ABI::Windows::UI::Notifications;

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
        // Set DPI awareness for the process
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        
        HDC screen = GetDC(NULL);
        dpi_ = GetDeviceCaps(screen, LOGPIXELSX);
        ReleaseDC(NULL, screen);
        
        scale_ = dpi_ / 96.0f;
    }
    
    static int Scale(int value) {
        return static_cast<int>(value * scale_);
    }
    
    static int GetDPI() {
        return dpi_;
    }
    
    static float GetScale() {
        return scale_;
    }
    
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
// WINDOWS 10 TOAST NOTIFICATION HANDLER
// ============================================================================
class ToastNotificationHandler : public RuntimeClass<RuntimeClassFlags<ClassicCom>, INotificationActivationCallback> {
public:
    virtual HRESULT STDMETHODCALLTYPE Activate(
        LPCWSTR appUserModelId,
        LPCWSTR invokedArgs,
        const NOTIFICATION_USER_INPUT_DATA* data,
        ULONG count) override {
        return S_OK;
    }
};

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
// COMPONENT BASE CLASS
// ============================================================================
class Component {
protected:
    HWND hwnd_ = nullptr;
    HWND parent_ = nullptr;
    int x_ = 0, y_ = 0, width_ = 100, height_ = 30;
    std::wstring text_;
    bool visible_ = true;
    bool enabled_ = true;
    HFONT font_ = nullptr;
    std::function<void()> onClick_;
    std::function<void(const std::wstring&)> onChange_;
    std::wstring id_;
    
public:
    virtual ~Component() {
        if (font_) DeleteObject(font_);
        if (hwnd_ && IsWindow(hwnd_)) {
            DestroyWindow(hwnd_);
        }
    }
    
    HWND getHandle() const { return hwnd_; }
    std::wstring getId() const { return id_; }
    
    Component* position(int x, int y) {
        x_ = DPIManager::Scale(x);
        y_ = DPIManager::Scale(y);
        if (hwnd_ && IsWindow(hwnd_)) {
            SetWindowPos(hwnd_, NULL, x_, y_, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        return this;
    }
    
    Component* size(int width, int height) {
        width_ = DPIManager::Scale(width);
        height_ = DPIManager::Scale(height);
        if (hwnd_ && IsWindow(hwnd_)) {
            SetWindowPos(hwnd_, NULL, 0, 0, width_, height_, SWP_NOMOVE | SWP_NOZORDER);
        }
        return this;
    }
    
    Component* text(const std::wstring& txt) {
        text_ = txt;
        if (hwnd_ && IsWindow(hwnd_)) {
            SetWindowTextW(hwnd_, text_.c_str());
        }
        return this;
    }
    
    std::wstring text() const {
        if (!hwnd_ || !IsWindow(hwnd_)) return text_;
        int len = GetWindowTextLengthW(hwnd_);
        if (len == 0) return L"";
        std::vector<wchar_t> buffer(len + 1);
        GetWindowTextW(hwnd_, buffer.data(), len + 1);
        return std::wstring(buffer.data());
    }
    
    Component* id(const std::wstring& identifier) {
        id_ = identifier;
        return this;
    }
    
    Component* show(bool visible = true) {
        visible_ = visible;
        if (hwnd_ && IsWindow(hwnd_)) {
            ShowWindow(hwnd_, visible ? SW_SHOW : SW_HIDE);
        }
        return this;
    }
    
    Component* enable(bool enabled = true) {
        enabled_ = enabled;
        if (hwnd_ && IsWindow(hwnd_)) {
            EnableWindow(hwnd_, enabled);
        }
        return this;
    }
    
    bool isVisible() const { return visible_; }
    bool isEnabled() const { return enabled_; }
    
protected:
    void setDefaultFont() {
        if (font_) DeleteObject(font_);
        font_ = DPIManager::CreateScaledFont(9, FW_NORMAL, L"Segoe UI");
        if (hwnd_ && IsWindow(hwnd_)) {
            SendMessage(hwnd_, WM_SETFONT, (WPARAM)font_, TRUE);
        }
    }
};

// ============================================================================
// BUTTON COMPONENT
// ============================================================================
class Button : public Component {
private:
    bool isDefault_ = false;
    
public:
    Button(HWND parent, const std::wstring& text) {
        parent_ = parent;
        text_ = text;
        width_ = DPIManager::Scale(100);
        height_ = DPIManager::Scale(30);
        
        hwnd_ = CreateWindowExW(
            0,
            L"BUTTON",
            text_.c_str(),
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            x_, y_, width_, height_,
            parent_,
            NULL,
            GetModuleHandle(NULL),
            NULL
        );
        
        if (!hwnd_) {
            throw std::runtime_error("Failed to create button");
        }
        
        // Enable visual styles
        SetWindowTheme(hwnd_, L"Explorer", NULL);
        setDefaultFont();
    }
    
    Button* position(int x, int y) { 
        Component::position(x, y); 
        return this; 
    }
    
    Button* size(int w, int h) { 
        Component::size(w, h); 
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
        if (hwnd_ && IsWindow(hwnd_)) {
            LONG style = GetWindowLong(hwnd_, GWL_STYLE);
            style &= ~(BS_DEFPUSHBUTTON | BS_PUSHBUTTON);
            style |= isDefault ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON;
            SetWindowLong(hwnd_, GWL_STYLE, style);
            InvalidateRect(hwnd_, NULL, TRUE);
        }
        return this;
    }
    
    void triggerClick() {
        if (onClick_) onClick_();
    }
};

// ============================================================================
// INPUT (EDIT CONTROL)
// ============================================================================
class Input : public Component {
private:
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
        
        hwnd_ = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            L"",
            style,
            x_, y_, width_, height_,
            parent_,
            NULL,
            GetModuleHandle(NULL),
            NULL
        );
        
        if (!hwnd_) {
            throw std::runtime_error("Failed to create input");
        }
        
        setDefaultFont();
        
        if (!placeholder_.empty()) {
            SendMessageW(hwnd_, EM_SETCUEBANNER, TRUE, (LPARAM)placeholder_.c_str());
        }
    }
    
    Input* position(int x, int y) { 
        Component::position(x, y); 
        return this; 
    }
    
    Input* size(int w, int h) { 
        Component::size(w, h); 
        return this; 
    }
    
    Input* onChange(std::function<void(const std::wstring&)> callback) { 
        onChange_ = callback; 
        return this; 
    }
    
    Input* multiline(bool enable = true) {
        multiline_ = enable;
        if (hwnd_) {
            DestroyWindow(hwnd_);
            
            DWORD style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | 
                          ES_AUTOVSCROLL | WS_VSCROLL;
            if (multiline_) style |= ES_MULTILINE;
            
            hwnd_ = CreateWindowExW(
                WS_EX_CLIENTEDGE,
                L"EDIT",
                text_.c_str(),
                style,
                x_, y_, width_, height_,
                parent_,
                NULL,
                GetModuleHandle(NULL),
                NULL
            );
            
            setDefaultFont();
        }
        return this;
    }
    
    Input* password(bool enable = true) {
        password_ = enable;
        if (hwnd_ && IsWindow(hwnd_)) {
            LONG style = GetWindowLong(hwnd_, GWL_STYLE);
            if (enable) {
                style |= ES_PASSWORD;
            } else {
                style &= ~ES_PASSWORD;
            }
            SetWindowLong(hwnd_, GWL_STYLE, style);
        }
        return this;
    }
    
    Input* readonly(bool enable = true) {
        readonly_ = enable;
        if (hwnd_ && IsWindow(hwnd_)) {
            SendMessage(hwnd_, EM_SETREADONLY, enable, 0);
        }
        return this;
    }
    
    std::wstring getValue() const {
        if (!hwnd_ || !IsWindow(hwnd_)) return L"";
        int len = GetWindowTextLengthW(hwnd_);
        if (len == 0) return L"";
        std::vector<wchar_t> buffer(len + 1);
        GetWindowTextW(hwnd_, buffer.data(), len + 1);
        return std::wstring(buffer.data());
    }
    
    void setValue(const std::wstring& value) {
        text(value);
    }
    
    void triggerChange() {
        if (onChange_) onChange_(getValue());
    }
};

// ============================================================================
// LABEL (STATIC TEXT)
// ============================================================================
class Label : public Component {
public:
    Label(HWND parent, const std::wstring& text) {
        parent_ = parent;
        text_ = text;
        width_ = DPIManager::Scale(200);
        height_ = DPIManager::Scale(20);
        
        hwnd_ = CreateWindowExW(
            0,
            L"STATIC",
            text_.c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            x_, y_, width_, height_,
            parent_,
            NULL,
            GetModuleHandle(NULL),
            NULL
        );
        
        if (!hwnd_) {
            throw std::runtime_error("Failed to create label");
        }
        
        setDefaultFont();
    }
    
    Label* position(int x, int y) { 
        Component::position(x, y); 
        return this; 
    }
    
    Label* size(int w, int h) { 
        Component::size(w, h); 
        return this; 
    }
    
    Label* text(const std::wstring& txt) {
        Component::text(txt);
        return this;
    }
    
    Label* font(int size, int weight = FW_NORMAL, const wchar_t* face = L"Segoe UI") {
        if (font_) DeleteObject(font_);
        
        font_ = DPIManager::CreateScaledFont(size, weight, face);
        
        if (hwnd_ && IsWindow(hwnd_)) {
            SendMessage(hwnd_, WM_SETFONT, (WPARAM)font_, TRUE);
        }
        return this;
    }
    
    Label* bold(bool enable = true) {
        return font(9, enable ? FW_BOLD : FW_NORMAL);
    }
};

// ============================================================================
// CHECKBOX
// ============================================================================
class CheckBox : public Component {
private:
    bool checked_ = false;
    
public:
    CheckBox(HWND parent, const std::wstring& text) {
        parent_ = parent;
        text_ = text;
        width_ = DPIManager::Scale(200);
        height_ = DPIManager::Scale(20);
        
        hwnd_ = CreateWindowExW(
            0,
            L"BUTTON",
            text_.c_str(),
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
            x_, y_, width_, height_,
            parent_,
            NULL,
            GetModuleHandle(NULL),
            NULL
        );
        
        if (!hwnd_) {
            throw std::runtime_error("Failed to create checkbox");
        }
        
        SetWindowTheme(hwnd_, L"Explorer", NULL);
        setDefaultFont();
    }
    
    CheckBox* position(int x, int y) { 
        Component::position(x, y); 
        return this; 
    }
    
    CheckBox* size(int w, int h) { 
        Component::size(w, h); 
        return this; 
    }
    
    CheckBox* onChange(std::function<void(bool)> callback) {
        onChange_ = [callback](const std::wstring&) {
            // Will be called with checked state
        };
        return this;
    }
    
    bool isChecked() const {
        if (!hwnd_ || !IsWindow(hwnd_)) return checked_;
        return SendMessage(hwnd_, BM_GETCHECK, 0, 0) == BST_CHECKED;
    }
    
    CheckBox* setChecked(bool checked) {
        checked_ = checked;
        if (hwnd_ && IsWindow(hwnd_)) {
            SendMessage(hwnd_, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
        }
        return this;
    }
    
    void triggerChange() {
        // Can be implemented if needed
    }
};

// ============================================================================
// COMBOBOX (DROPDOWN)
// ============================================================================
class ComboBox : public Component {
private:
    std::vector<std::wstring> items_;
    
public:
    ComboBox(HWND parent) {
        parent_ = parent;
        width_ = DPIManager::Scale(200);
        height_ = DPIManager::Scale(200); // Dropdown height
        
        hwnd_ = CreateWindowExW(
            0,
            L"COMBOBOX",
            L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
            x_, y_, width_, height_,
            parent_,
            NULL,
            GetModuleHandle(NULL),
            NULL
        );
        
        if (!hwnd_) {
            throw std::runtime_error("Failed to create combobox");
        }
        
        setDefaultFont();
    }
    
    ComboBox* position(int x, int y) { 
        Component::position(x, y); 
        return this; 
    }
    
    ComboBox* size(int w, int h) { 
        Component::size(w, h); 
        return this; 
    }
    
    ComboBox* addItem(const std::wstring& item) {
        items_.push_back(item);
        if (hwnd_ && IsWindow(hwnd_)) {
            SendMessageW(hwnd_, CB_ADDSTRING, 0, (LPARAM)item.c_str());
        }
        return this;
    }
    
    ComboBox* setSelectedIndex(int index) {
        if (hwnd_ && IsWindow(hwnd_)) {
            SendMessage(hwnd_, CB_SETCURSEL, index, 0);
        }
        return this;
    }
    
    int getSelectedIndex() const {
        if (!hwnd_ || !IsWindow(hwnd_)) return -1;
        return (int)SendMessage(hwnd_, CB_GETCURSEL, 0, 0);
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
// PROGRESSBAR
// ============================================================================
class ProgressBar : public Component {
private:
    int min_ = 0;
    int max_ = 100;
    int value_ = 0;
    
public:
    ProgressBar(HWND parent) {
        parent_ = parent;
        width_ = DPIManager::Scale(200);
        height_ = DPIManager::Scale(20);
        
        hwnd_ = CreateWindowExW(
            0,
            PROGRESS_CLASS,
            L"",
            WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
            x_, y_, width_, height_,
            parent_,
            NULL,
            GetModuleHandle(NULL),
            NULL
        );
        
        if (!hwnd_) {
            throw std::runtime_error("Failed to create progressbar");
        }
        
        SetWindowTheme(hwnd_, L"Explorer", NULL);
        SendMessage(hwnd_, PBM_SETRANGE, 0, MAKELPARAM(min_, max_));
    }
    
    ProgressBar* position(int x, int y) { 
        Component::position(x, y); 
        return this; 
    }
    
    ProgressBar* size(int w, int h) { 
        Component::size(w, h); 
        return this; 
    }
    
    ProgressBar* setRange(int min, int max) {
        min_ = min;
        max_ = max;
        if (hwnd_ && IsWindow(hwnd_)) {
            SendMessage(hwnd_, PBM_SETRANGE, 0, MAKELPARAM(min_, max_));
        }
        return this;
    }
    
    ProgressBar* setValue(int value) {
        value_ = value;
        if (hwnd_ && IsWindow(hwnd_)) {
            SendMessage(hwnd_, PBM_SETPOS, value_, 0);
        }
        return this;
    }
    
    int getValue() const {
        return value_;
    }
};

// ============================================================================
// GROUPBOX
// ============================================================================
class GroupBox : public Component {
private:
    std::vector<std::shared_ptr<Component>> children_;
    
public:
    GroupBox(HWND parent, const std::wstring& text = L"") {
        parent_ = parent;
        text_ = text;
        width_ = DPIManager::Scale(300);
        height_ = DPIManager::Scale(200);
        
        hwnd_ = CreateWindowExW(
            0,
            L"BUTTON",
            text_.c_str(),
            WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
            x_, y_, width_, height_,
            parent_,
            NULL,
            GetModuleHandle(NULL),
            NULL
        );
        
        if (!hwnd_) {
            throw std::runtime_error("Failed to create groupbox");
        }
        
        setDefaultFont();
    }
    
    GroupBox* position(int x, int y) { 
        Component::position(x, y); 
        return this; 
    }
    
    GroupBox* size(int w, int h) { 
        Component::size(w, h); 
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
// WINDOWS 10 NOTIFICATION SYSTEM
// ============================================================================
class Notification {
private:
    static bool initialized_;
    static std::wstring appId_;
    
    static bool InitializeToastNotifications() {
        if (initialized_) return true;
        
        HRESULT hr = RoInitialize(RO_INIT_MULTITHREADED);
        if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
            return false;
        }
        
        initialized_ = true;
        appId_ = L"UIBro.Application";
        
        return true;
    }
    
    static std::wstring CreateToastXml(const std::wstring& title, const std::wstring& message) {
        std::wstring xml = L"<toast>"
                          L"<visual>"
                          L"<binding template='ToastGeneric'>"
                          L"<text>" + title + L"</text>"
                          L"<text>" + message + L"</text>"
                          L"</binding>"
                          L"</visual>"
                          L"</toast>";
        return xml;
    }
    
public:
    static void show(const std::wstring& title, const std::wstring& message, int duration = 3000) {
        if (!InitializeToastNotifications()) {
            // Fallback to message box if toast notifications fail
            MessageBoxW(NULL, message.c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION);
            return;
        }
        
        HRESULT hr = S_OK;
        
        // Create toast notifier factory
        ComPtr<IToastNotificationManagerStatics> toastStatics;
        hr = Windows::Foundation::GetActivationFactory(
            HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(),
            &toastStatics
        );
        
        if (FAILED(hr)) {
            MessageBoxW(NULL, message.c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION);
            return;
        }
        
        // Create XML document
        ComPtr<IXmlDocument> toastXml;
        std::wstring xml = CreateToastXml(title, message);
        
        ComPtr<IToastNotificationManagerStatics2> toastStatics2;
        hr = toastStatics.As(&toastStatics2);
        if (SUCCEEDED(hr)) {
            ComPtr<IXmlDocument> xmlDoc;
            hr = toastStatics2->GetTemplateContent(ToastTemplateType_ToastText02, &xmlDoc);
            
            if (SUCCEEDED(hr)) {
                // Get text elements
                ComPtr<IXmlNodeList> textElements;
                HStringReference textTag(L"text");
                hr = xmlDoc->GetElementsByTagName(textTag.Get(), &textElements);
                
                if (SUCCEEDED(hr)) {
                    UINT32 textLength;
                    hr = textElements->get_Length(&textLength);
                    
                    if (SUCCEEDED(hr) && textLength >= 2) {
                        // Set title
                        ComPtr<IXmlNode> titleNode;
                        hr = textElements->Item(0, &titleNode);
                        if (SUCCEEDED(hr)) {
                            ComPtr<IXmlText> titleText;
                            ComPtr<IXmlDocument> xmlDocForText;
                            hr = xmlDoc.As(&xmlDocForText);
                            if (SUCCEEDED(hr)) {
                                HStringReference titleStr(title.c_str());
                                hr = xmlDocForText->CreateTextNode(titleStr.Get(), &titleText);
                                if (SUCCEEDED(hr)) {
                                    ComPtr<IXmlNode> titleTextNode;
                                    hr = titleText.As(&titleTextNode);
                                    if (SUCCEEDED(hr)) {
                                        ComPtr<IXmlNode> appendedChild;
                                        hr = titleNode->AppendChild(titleTextNode.Get(), &appendedChild);
                                    }
                                }
                            }
                        }
                        
                        // Set message
                        ComPtr<IXmlNode> messageNode;
                        hr = textElements->Item(1, &messageNode);
                        if (SUCCEEDED(hr)) {
                            ComPtr<IXmlText> messageText;
                            ComPtr<IXmlDocument> xmlDocForText;
                            hr = xmlDoc.As(&xmlDocForText);
                            if (SUCCEEDED(hr)) {
                                HStringReference messageStr(message.c_str());
                                hr = xmlDocForText->CreateTextNode(messageStr.Get(), &messageText);
                                if (SUCCEEDED(hr)) {
                                    ComPtr<IXmlNode> messageTextNode;
                                    hr = messageText.As(&messageTextNode);
                                    if (SUCCEEDED(hr)) {
                                        ComPtr<IXmlNode> appendedChild;
                                        hr = messageNode->AppendChild(messageTextNode.Get(), &appendedChild);
                                    }
                                }
                            }
                        }
                    }
                }
                
                // Create toast notification
                ComPtr<IToastNotificationFactory> toastFactory;
                hr = Windows::Foundation::GetActivationFactory(
                    HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotification).Get(),
                    &toastFactory
                );
                
                if (SUCCEEDED(hr)) {
                    ComPtr<IToastNotification> toast;
                    hr = toastFactory->CreateToastNotification(xmlDoc.Get(), &toast);
                    
                    if (SUCCEEDED(hr)) {
                        // Create toast notifier
                        ComPtr<IToastNotifier> notifier;
                        HStringReference appIdStr(appId_.c_str());
                        hr = toastStatics->CreateToastNotifierWithId(appIdStr.Get(), &notifier);
                        
                        if (SUCCEEDED(hr)) {
                            // Show toast
                            hr = notifier->Show(toast.Get());
                            
                            if (SUCCEEDED(hr)) {
                                return; // Success!
                            }
                        }
                    }
                }
            }
        }
        
        // If we got here, toast notification failed, use message box as fallback
        MessageBoxW(NULL, message.c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION);
    }
};

bool Notification::initialized_ = false;
std::wstring Notification::appId_ = L"UIBro.Application";

// ============================================================================
// WINDOW
// ============================================================================
class Window {
private:
    HWND hwnd_ = nullptr;
    std::wstring title_ = L"UIBro Application";
    int width_ = 800;
    int height_ = 600;
    bool centerOnScreen_ = true;
    std::vector<std::shared_ptr<Component>> components_;
    std::map<std::wstring, Component*> componentMap_;
    ThreadSafeQueue<std::function<void()>> messageQueue_;
    std::atomic<bool> running_{false};
    std::thread messageThread_;
    COLORREF bgColor_ = RGB(240, 240, 240); // Windows 10 default background
    
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        Window* window = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        
        switch (msg) {
            case WM_DPICHANGED: {
                // Handle DPI changes for per-monitor DPI awareness
                RECT* newRect = reinterpret_cast<RECT*>(lParam);
                SetWindowPos(hwnd, NULL, 
                    newRect->left, newRect->top,
                    newRect->right - newRect->left,
                    newRect->bottom - newRect->top,
                    SWP_NOZORDER | SWP_NOACTIVATE);
                
                // Update DPI
                DPIManager::Initialize();
                return 0;
            }
            
            case WM_CTLCOLORSTATIC: {
                HDC hdcStatic = (HDC)wParam;
                SetBkColor(hdcStatic, window ? window->bgColor_ : RGB(240, 240, 240));
                return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
            }
            
            case WM_COMMAND: {
                if (window) {
                    HWND controlHwnd = (HWND)lParam;
                    WORD notificationCode = HIWORD(wParam);
                    
                    // Find the component
                    for (auto& comp : window->components_) {
                        if (comp->getHandle() == controlHwnd) {
                            // Handle button clicks
                            if (auto btn = dynamic_cast<Button*>(comp.get())) {
                                if (notificationCode == BN_CLICKED) {
                                    btn->triggerClick();
                                }
                            }
                            // Handle checkbox changes
                            else if (auto chk = dynamic_cast<CheckBox*>(comp.get())) {
                                if (notificationCode == BN_CLICKED) {
                                    chk->triggerChange();
                                }
                            }
                            // Handle edit control changes
                            else if (auto input = dynamic_cast<Input*>(comp.get())) {
                                if (notificationCode == EN_CHANGE) {
                                    input->triggerChange();
                                }
                            }
                            // Handle combobox selection
                            else if (auto combo = dynamic_cast<ComboBox*>(comp.get())) {
                                if (notificationCode == CBN_SELCHANGE) {
                                    combo->triggerChange();
                                }
                            }
                            break;
                        }
                    }
                }
                break;
            }
            
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
                
            case WM_CLOSE:
                DestroyWindow(hwnd);
                return 0;
        }
        
        return DefWindowProc(hwnd, msg, wParam, lParam);
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
    
public:
    Window() {
        INITCOMMONCONTROLSEX icex = {
            sizeof(INITCOMMONCONTROLSEX),
            ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES | ICC_PROGRESS_CLASS
        };
        InitCommonControlsEx(&icex);
    }
    
    ~Window() {
        running_ = false;
        if (messageThread_.joinable()) {
            messageThread_.join();
        }
        if (hwnd_ && IsWindow(hwnd_)) {
            DestroyWindow(hwnd_);
        }
    }
    
    Window* title(const std::wstring& t) { 
        title_ = t; 
        if (hwnd_ && IsWindow(hwnd_)) {
            SetWindowTextW(hwnd_, title_.c_str()); 
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
        return hwnd_; 
    }
    
    Label* addLabel(const std::wstring& text) {
        if (!hwnd_) create();
        auto c = std::make_shared<Label>(hwnd_, text);
        components_.push_back(c);
        if (!c->getId().empty()) {
            componentMap_[c->getId()] = c.get();
        }
        return c.get();
    }
    
    Button* addButton(const std::wstring& text) {
        if (!hwnd_) create();
        auto c = std::make_shared<Button>(hwnd_, text);
        components_.push_back(c);
        if (!c->getId().empty()) {
            componentMap_[c->getId()] = c.get();
        }
        return c.get();
    }
    
    Input* addInput(const std::wstring& placeholder = L"") {
        if (!hwnd_) create();
        auto c = std::make_shared<Input>(hwnd_, placeholder);
        components_.push_back(c);
        if (!c->getId().empty()) {
            componentMap_[c->getId()] = c.get();
        }
        return c.get();
    }
    
    CheckBox* addCheckBox(const std::wstring& text) {
        if (!hwnd_) create();
        auto c = std::make_shared<CheckBox>(hwnd_, text);
        components_.push_back(c);
        if (!c->getId().empty()) {
            componentMap_[c->getId()] = c.get();
        }
        return c.get();
    }
    
    ComboBox* addComboBox() {
        if (!hwnd_) create();
        auto c = std::make_shared<ComboBox>(hwnd_);
        components_.push_back(c);
        if (!c->getId().empty()) {
            componentMap_[c->getId()] = c.get();
        }
        return c.get();
    }
    
    ProgressBar* addProgressBar() {
        if (!hwnd_) create();
        auto c = std::make_shared<ProgressBar>(hwnd_);
        components_.push_back(c);
        if (!c->getId().empty()) {
            componentMap_[c->getId()] = c.get();
        }
        return c.get();
    }
    
    GroupBox* addGroupBox(const std::wstring& text = L"") {
        if (!hwnd_) create();
        auto c = std::make_shared<GroupBox>(hwnd_, text);
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
        if (hwnd_) return;
        
        WNDCLASSEXW wc = {sizeof(WNDCLASSEXW)};
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = L"UIBroWindow";
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
        
        RegisterClassExW(&wc);
        
        RECT rect = {0, 0, width_, height_};
        AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0);
        
        int x = CW_USEDEFAULT;
        int y = CW_USEDEFAULT;
        
        if (centerOnScreen_) {
            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);
            x = (screenWidth - (rect.right - rect.left)) / 2;
            y = (screenHeight - (rect.bottom - rect.top)) / 2;
        }
        
        hwnd_ = CreateWindowExW(
            0,
            L"UIBroWindow",
            title_.c_str(),
            WS_OVERLAPPEDWINDOW,
            x, y,
            rect.right - rect.left,
            rect.bottom - rect.top,
            NULL,
            NULL,
            GetModuleHandle(NULL),
            NULL
        );
        
        if (!hwnd_) {
            throw std::runtime_error("Failed to create window");
        }
        
        SetWindowLongPtr(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        
        // Enable modern Windows 10 visual styles
        SetWindowTheme(hwnd_, L"Explorer", NULL);
        
        // Enable dark mode title bar on Windows 10 1809+
        BOOL useDarkMode = FALSE; // Set to TRUE for dark mode
        DwmSetWindowAttribute(hwnd_, 19, &useDarkMode, sizeof(useDarkMode)); // DWMWA_USE_IMMERSIVE_DARK_MODE = 19
        
        ShowWindow(hwnd_, SW_SHOW);
        UpdateWindow(hwnd_);
        
        running_ = true;
        messageThread_ = std::thread(&Window::processMessageQueue, this);
    }
    
    int run() {
        if (!hwnd_) create();
        
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        running_ = false;
        return static_cast<int>(msg.wParam);
    }
};

// ============================================================================
// ADVANCED SCRIPTING ENGINE WITH CONDITIONALS
// ============================================================================

enum class TokenType {
    IDENTIFIER, STRING, NUMBER, BOOLEAN,
    LPAREN, RPAREN, LBRACE, RBRACE,
    DOT, COMMA, SEMICOLON, EQUALS, COLON,
    PLUS, MINUS, STAR, SLASH,
    EQUAL_EQUAL, NOT_EQUAL, LESS, LESS_EQUAL, GREATER, GREATER_EQUAL,
    AND, OR, NOT,
    IF, ELSE, ELSEIF, WHILE, FOR,
    KEYWORD, END, UNKNOWN
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
};

class Lexer {
private:
    std::string source_;
    size_t pos_ = 0;
    int line_ = 1;
    int column_ = 1;
    
    char peek(int offset = 0) const {
        size_t index = pos_ + offset;
        return index < source_.size() ? source_[index] : '\0';
    }
    
    char advance() {
        if (pos_ < source_.size()) {
            char c = source_[pos_++];
            column_++;
            if (c == '\n') {
                line_++;
                column_ = 1;
            }
            return c;
        }
        return '\0';
    }
    
    void skipWhitespace() {
        while (std::isspace(peek())) {
            advance();
        }
    }
    
    void skipComment() {
        if (peek() == '/' && peek(1) == '/') {
            while (peek() != '\n' && peek() != '\0') {
                advance();
            }
        }
    }
    
    Token makeToken(TokenType type, const std::string& value = "") {
        return {type, value, line_, column_};
    }
    
    std::string readString(char quote) {
        std::string str;
        advance(); // Skip opening quote
        
        while (peek() != quote && peek() != '\0') {
            if (peek() == '\\') {
                advance();
                char escaped = advance();
                switch (escaped) {
                    case 'n': str += '\n'; break;
                    case 't': str += '\t'; break;
                    case 'r': str += '\r'; break;
                    case '\\': str += '\\'; break;
                    case '"': str += '"'; break;
                    case '\'': str += '\''; break;
                    default: str += escaped;
                }
            } else {
                str += advance();
            }
        }
        
        if (peek() == quote) advance(); // Skip closing quote
        return str;
    }
    
    std::string readNumber() {
        std::string num;
        while (std::isdigit(peek()) || peek() == '.') {
            num += advance();
        }
        return num;
    }
    
    std::string readIdentifier() {
        std::string id;
        while (std::isalnum(peek()) || peek() == '_') {
            id += advance();
        }
        return id;
    }
    
    TokenType getKeywordType(const std::string& word) {
        static const std::map<std::string, TokenType> keywords = {
            {"if", TokenType::IF},
            {"else", TokenType::ELSE},
            {"elseif", TokenType::ELSEIF},
            {"while", TokenType::WHILE},
            {"for", TokenType::FOR},
            {"true", TokenType::BOOLEAN},
            {"false", TokenType::BOOLEAN},
            {"Window", TokenType::KEYWORD},
            {"Button", TokenType::KEYWORD},
            {"Label", TokenType::KEYWORD},
            {"Input", TokenType::KEYWORD},
            {"CheckBox", TokenType::KEYWORD},
            {"GroupBox", TokenType::KEYWORD},
            {"ComboBox", TokenType::KEYWORD},
            {"ProgressBar", TokenType::KEYWORD},
            {"Notification", TokenType::KEYWORD}
        };
        
        auto it = keywords.find(word);
        return it != keywords.end() ? it->second : TokenType::IDENTIFIER;
    }
    
public:
    Lexer(const std::string& source) : source_(source) {}
    
    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        
        while (pos_ < source_.size()) {
            skipWhitespace();
            skipComment();
            
            if (pos_ >= source_.size()) break;
            
            char c = peek();
            
            // Strings
            if (c == '"' || c == '\'') {
                std::string str = readString(c);
                tokens.push_back(makeToken(TokenType::STRING, str));
                continue;
            }
            
            // Numbers
            if (std::isdigit(c)) {
                std::string num = readNumber();
                tokens.push_back(makeToken(TokenType::NUMBER, num));
                continue;
            }
            
            // Identifiers and keywords
            if (std::isalpha(c) || c == '_') {
                std::string id = readIdentifier();
                TokenType type = getKeywordType(id);
                tokens.push_back(makeToken(type, id));
                continue;
            }
            
            // Operators and punctuation
            switch (c) {
                case '(':
                    tokens.push_back(makeToken(TokenType::LPAREN));
                    advance();
                    break;
                case ')':
                    tokens.push_back(makeToken(TokenType::RPAREN));
                    advance();
                    break;
                case '{':
                    tokens.push_back(makeToken(TokenType::LBRACE));
                    advance();
                    break;
                case '}':
                    tokens.push_back(makeToken(TokenType::RBRACE));
                    advance();
                    break;
                case '.':
                    tokens.push_back(makeToken(TokenType::DOT));
                    advance();
                    break;
                case ',':
                    tokens.push_back(makeToken(TokenType::COMMA));
                    advance();
                    break;
                case ';':
                    tokens.push_back(makeToken(TokenType::SEMICOLON));
                    advance();
                    break;
                case ':':
                    tokens.push_back(makeToken(TokenType::COLON));
                    advance();
                    break;
                case '+':
                    tokens.push_back(makeToken(TokenType::PLUS));
                    advance();
                    break;
                case '-':
                    tokens.push_back(makeToken(TokenType::MINUS));
                    advance();
                    break;
                case '*':
                    tokens.push_back(makeToken(TokenType::STAR));
                    advance();
                    break;
                case '/':
                    tokens.push_back(makeToken(TokenType::SLASH));
                    advance();
                    break;
                case '=':
                    advance();
                    if (peek() == '=') {
                        advance();
                        tokens.push_back(makeToken(TokenType::EQUAL_EQUAL));
                    } else {
                        tokens.push_back(makeToken(TokenType::EQUALS));
                    }
                    break;
                case '!':
                    advance();
                    if (peek() == '=') {
                        advance();
                        tokens.push_back(makeToken(TokenType::NOT_EQUAL));
                    } else {
                        tokens.push_back(makeToken(TokenType::NOT));
                    }
                    break;
                case '<':
                    advance();
                    if (peek() == '=') {
                        advance();
                        tokens.push_back(makeToken(TokenType::LESS_EQUAL));
                    } else {
                        tokens.push_back(makeToken(TokenType::LESS));
                    }
                    break;
                case '>':
                    advance();
                    if (peek() == '=') {
                        advance();
                        tokens.push_back(makeToken(TokenType::GREATER_EQUAL));
                    } else {
                        tokens.push_back(makeToken(TokenType::GREATER));
                    }
                    break;
                case '&':
                    advance();
                    if (peek() == '&') {
                        advance();
                        tokens.push_back(makeToken(TokenType::AND));
                    }
                    break;
                case '|':
                    advance();
                    if (peek() == '|') {
                        advance();
                        tokens.push_back(makeToken(TokenType::OR));
                    }
                    break;
                default:
                    advance(); // Skip unknown characters
            }
        }
        
        tokens.push_back(makeToken(TokenType::END));
        return tokens;
    }
};

// ============================================================================
// AST NODES
// ============================================================================

struct ASTNode {
    virtual ~ASTNode() = default;
};

struct ExpressionNode : ASTNode {
    virtual ~ExpressionNode() = default;
};

struct LiteralNode : ExpressionNode {
    std::string value;
    TokenType type;
};

struct BinaryOpNode : ExpressionNode {
    std::shared_ptr<ExpressionNode> left;
    std::shared_ptr<ExpressionNode> right;
    TokenType op;
};

struct CallNode : ASTNode {
    std::string function;
    std::vector<std::shared_ptr<ExpressionNode>> args;
};

struct ChainNode : ASTNode {
    std::string object;
    std::vector<std::shared_ptr<CallNode>> calls;
};

struct AssignmentNode : ASTNode {
    std::string varName;
    std::shared_ptr<ASTNode> value;
};

struct IfNode : ASTNode {
    std::shared_ptr<ExpressionNode> condition;
    std::vector<std::shared_ptr<ASTNode>> thenBlock;
    std::vector<std::shared_ptr<ASTNode>> elseBlock;
};

struct BlockNode : ASTNode {
    std::vector<std::shared_ptr<ASTNode>> statements;
};

struct ProgramNode : ASTNode {
    std::vector<std::shared_ptr<ASTNode>> statements;
};

// ============================================================================
// PARSER
// ============================================================================

class Parser {
private:
    std::vector<Token> tokens_;
    size_t pos_ = 0;
    
    Token peek(int offset = 0) const {
        size_t index = pos_ + offset;
        return index < tokens_.size() ? tokens_[index] : tokens_.back();
    }
    
    Token advance() {
        return pos_ < tokens_.size() ? tokens_[pos_++] : tokens_.back();
    }
    
    bool match(TokenType type) {
        if (peek().type == type) {
            advance();
            return true;
        }
        return false;
    }
    
    bool check(TokenType type) const {
        return peek().type == type;
    }
    
    std::shared_ptr<ExpressionNode> parseExpression() {
        return parseLogicalOr();
    }
    
    std::shared_ptr<ExpressionNode> parseLogicalOr() {
        auto left = parseLogicalAnd();
        
        while (match(TokenType::OR)) {
            auto node = std::make_shared<BinaryOpNode>();
            node->left = left;
            node->op = TokenType::OR;
            node->right = parseLogicalAnd();
            left = node;
        }
        
        return left;
    }
    
    std::shared_ptr<ExpressionNode> parseLogicalAnd() {
        auto left = parseEquality();
        
        while (match(TokenType::AND)) {
            auto node = std::make_shared<BinaryOpNode>();
            node->left = left;
            node->op = TokenType::AND;
            node->right = parseEquality();
            left = node;
        }
        
        return left;
    }
    
    std::shared_ptr<ExpressionNode> parseEquality() {
        auto left = parseComparison();
        
        while (check(TokenType::EQUAL_EQUAL) || check(TokenType::NOT_EQUAL)) {
            auto node = std::make_shared<BinaryOpNode>();
            node->left = left;
            node->op = advance().type;
            node->right = parseComparison();
            left = node;
        }
        
        return left;
    }
    
    std::shared_ptr<ExpressionNode> parseComparison() {
        auto left = parseAdditive();
        
        while (check(TokenType::LESS) || check(TokenType::LESS_EQUAL) ||
               check(TokenType::GREATER) || check(TokenType::GREATER_EQUAL)) {
            auto node = std::make_shared<BinaryOpNode>();
            node->left = left;
            node->op = advance().type;
            node->right = parseAdditive();
            left = node;
        }
        
        return left;
    }
    
    std::shared_ptr<ExpressionNode> parseAdditive() {
        auto left = parseMultiplicative();
        
        while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
            auto node = std::make_shared<BinaryOpNode>();
            node->left = left;
            node->op = advance().type;
            node->right = parseMultiplicative();
            left = node;
        }
        
        return left;
    }
    
    std::shared_ptr<ExpressionNode> parseMultiplicative() {
        auto left = parsePrimary();
        
        while (check(TokenType::STAR) || check(TokenType::SLASH)) {
            auto node = std::make_shared<BinaryOpNode>();
            node->left = left;
            node->op = advance().type;
            node->right = parsePrimary();
            left = node;
        }
        
        return left;
    }
    
    std::shared_ptr<ExpressionNode> parsePrimary() {
        if (match(TokenType::NOT)) {
            auto node = std::make_shared<BinaryOpNode>();
            node->op = TokenType::NOT;
            node->right = parsePrimary();
            return node;
        }
        
        if (match(TokenType::LPAREN)) {
            auto expr = parseExpression();
            match(TokenType::RPAREN);
            return expr;
        }
        
        if (check(TokenType::STRING) || check(TokenType::NUMBER) || 
            check(TokenType::BOOLEAN) || check(TokenType::IDENTIFIER)) {
            auto node = std::make_shared<LiteralNode>();
            Token tok = advance();
            node->value = tok.value;
            node->type = tok.type;
            return node;
        }
        
        return nullptr;
    }
    
    std::shared_ptr<CallNode> parseCall() {
        auto node = std::make_shared<CallNode>();
        node->function = advance().value;
        
        if (match(TokenType::LPAREN)) {
            while (!check(TokenType::RPAREN) && !check(TokenType::END)) {
                node->args.push_back(parseExpression());
                if (!match(TokenType::COMMA)) break;
            }
            match(TokenType::RPAREN);
        }
        
        return node;
    }
    
    std::shared_ptr<ChainNode> parseChain() {
        auto node = std::make_shared<ChainNode>();
        
        if (check(TokenType::KEYWORD) || check(TokenType::IDENTIFIER)) {
            node->object = advance().value;
        }
        
        while (match(TokenType::DOT)) {
            if (check(TokenType::IDENTIFIER) || check(TokenType::KEYWORD)) {
                node->calls.push_back(parseCall());
            }
        }
        
        return node;
    }
    
    std::shared_ptr<IfNode> parseIf() {
        auto node = std::make_shared<IfNode>();
        
        match(TokenType::IF);
        match(TokenType::LPAREN);
        node->condition = parseExpression();
        match(TokenType::RPAREN);
        match(TokenType::LBRACE);
        
        while (!check(TokenType::RBRACE) && !check(TokenType::END)) {
            node->thenBlock.push_back(parseStatement());
        }
        
        match(TokenType::RBRACE);
        
        // Handle else if and else
        while (check(TokenType::ELSEIF)) {
            advance(); // consume 'elseif'
            
            auto elseIfNode = std::make_shared<IfNode>();
            match(TokenType::LPAREN);
            elseIfNode->condition = parseExpression();
            match(TokenType::RPAREN);
            match(TokenType::LBRACE);
            
            while (!check(TokenType::RBRACE) && !check(TokenType::END)) {
                elseIfNode->thenBlock.push_back(parseStatement());
            }
            
            match(TokenType::RBRACE);
            
            // Chain else-if as nested else block
            node->elseBlock.push_back(elseIfNode);
            node = elseIfNode; // Continue chaining
        }
        
        if (match(TokenType::ELSE)) {
            match(TokenType::LBRACE);
            
            while (!check(TokenType::RBRACE) && !check(TokenType::END)) {
                node->elseBlock.push_back(parseStatement());
            }
            
            match(TokenType::RBRACE);
        }
        
        return node;
    }
    
    std::shared_ptr<ASTNode> parseStatement() {
        // If statement
        if (check(TokenType::IF)) {
            return parseIf();
        }
        
        // Assignment or chain
        if (check(TokenType::IDENTIFIER)) {
            Token current = peek();
            Token next = peek(1);
            
            if (next.type == TokenType::EQUALS) {
                auto assignment = std::make_shared<AssignmentNode>();
                assignment->varName = advance().value;
                advance(); // consume '='
                
                if (check(TokenType::KEYWORD) || check(TokenType::IDENTIFIER)) {
                    assignment->value = parseChain();
                } else {
                    assignment->value = parseExpression();
                }
                
                match(TokenType::SEMICOLON);
                return assignment;
            } else if (next.type == TokenType::DOT) {
                auto chain = parseChain();
                match(TokenType::SEMICOLON);
                return chain;
            }
        }
        
        // Keyword statement
        if (check(TokenType::KEYWORD)) {
            auto chain = parseChain();
            match(TokenType::SEMICOLON);
            return chain;
        }
        
        // Skip unknown tokens
        advance();
        return nullptr;
    }
    
public:
    Parser(const std::vector<Token>& tokens) : tokens_(tokens) {}
    
    std::shared_ptr<ProgramNode> parse() {
        auto program = std::make_shared<ProgramNode>();
        
        while (!check(TokenType::END)) {
            auto stmt = parseStatement();
            if (stmt) {
                program->statements.push_back(stmt);
            }
        }
        
        return program;
    }
};

// ============================================================================
// INTERPRETER
// ============================================================================

class Interpreter {
private:
    struct Value {
        enum class Type { String, Number, Boolean, Object };
        Type type;
        std::string strValue;
        double numValue;
        bool boolValue;
        std::variant<Window*, Button*, Label*, Input*, CheckBox*, GroupBox*, ComboBox*, ProgressBar*> objValue;
    };
    
    std::map<std::string, Value> variables_;
    std::vector<std::shared_ptr<std::variant<Window*, Button*, Label*, Input*, CheckBox*, GroupBox*, ComboBox*, ProgressBar*>>> storage_;
    Window* currentWindow_ = nullptr;
    
    std::wstring toWString(const std::string& str) {
        return std::wstring(str.begin(), str.end());
    }
    
    std::string toString(const std::wstring& wstr) {
        return std::string(wstr.begin(), wstr.end());
    }
    
    int toInt(const std::string& str) {
        try { return std::stoi(str); }
        catch (...) { return 0; }
    }
    
    double toDouble(const std::string& str) {
        try { return std::stod(str); }
        catch (...) { return 0.0; }
    }
    
    bool toBool(const std::string& str) {
        return str == "true" || str == "1" || !str.empty();
    }
    
    Value evaluateExpression(std::shared_ptr<ExpressionNode> expr) {
        if (auto literal = std::dynamic_pointer_cast<LiteralNode>(expr)) {
            Value val;
            
            if (literal->type == TokenType::STRING) {
                val.type = Value::Type::String;
                val.strValue = literal->value;
            } else if (literal->type == TokenType::NUMBER) {
                val.type = Value::Type::Number;
                val.numValue = toDouble(literal->value);
            } else if (literal->type == TokenType::BOOLEAN) {
                val.type = Value::Type::Boolean;
                val.boolValue = literal->value == "true";
            } else if (literal->type == TokenType::IDENTIFIER) {
                // Look up variable
                auto it = variables_.find(literal->value);
                if (it != variables_.end()) {
                    return it->second;
                }
                val.type = Value::Type::String;
                val.strValue = literal->value;
            }
            
            return val;
        }
        
        if (auto binOp = std::dynamic_pointer_cast<BinaryOpNode>(expr)) {
            if (binOp->op == TokenType::NOT) {
                Value right = evaluateExpression(binOp->right);
                Value result;
                result.type = Value::Type::Boolean;
                result.boolValue = !getValue<bool>(right);
                return result;
            }
            
            Value left = evaluateExpression(binOp->left);
            Value right = evaluateExpression(binOp->right);
            Value result;
            
            switch (binOp->op) {
                case TokenType::PLUS:
                    result.type = Value::Type::Number;
                    result.numValue = getValue<double>(left) + getValue<double>(right);
                    break;
                    
                case TokenType::MINUS:
                    result.type = Value::Type::Number;
                    result.numValue = getValue<double>(left) - getValue<double>(right);
                    break;
                    
                case TokenType::STAR:
                    result.type = Value::Type::Number;
                    result.numValue = getValue<double>(left) * getValue<double>(right);
                    break;
                    
                case TokenType::SLASH:
                    result.type = Value::Type::Number;
                    result.numValue = getValue<double>(left) / getValue<double>(right);
                    break;
                    
                case TokenType::EQUAL_EQUAL:
                    result.type = Value::Type::Boolean;
                    result.boolValue = compareValues(left, right, true);
                    break;
                    
                case TokenType::NOT_EQUAL:
                    result.type = Value::Type::Boolean;
                    result.boolValue = !compareValues(left, right, true);
                    break;
                    
                case TokenType::LESS:
                    result.type = Value::Type::Boolean;
                    result.boolValue = getValue<double>(left) < getValue<double>(right);
                    break;
                    
                case TokenType::LESS_EQUAL:
                    result.type = Value::Type::Boolean;
                    result.boolValue = getValue<double>(left) <= getValue<double>(right);
                    break;
                    
                case TokenType::GREATER:
                    result.type = Value::Type::Boolean;
                    result.boolValue = getValue<double>(left) > getValue<double>(right);
                    break;
                    
                case TokenType::GREATER_EQUAL:
                    result.type = Value::Type::Boolean;
                    result.boolValue = getValue<double>(left) >= getValue<double>(right);
                    break;
                    
                case TokenType::AND:
                    result.type = Value::Type::Boolean;
                    result.boolValue = getValue<bool>(left) && getValue<bool>(right);
                    break;
                    
                case TokenType::OR:
                    result.type = Value::Type::Boolean;
                    result.boolValue = getValue<bool>(left) || getValue<bool>(right);
                    break;
                    
                default:
                    result.type = Value::Type::Boolean;
                    result.boolValue = false;
            }
            
            return result;
        }
        
        Value val;
        val.type = Value::Type::Boolean;
        val.boolValue = false;
        return val;
    }
    
    template<typename T>
    T getValue(const Value& val) {
        if constexpr (std::is_same_v<T, std::string>) {
            if (val.type == Value::Type::String) return val.strValue;
            if (val.type == Value::Type::Number) return std::to_string(val.numValue);
            if (val.type == Value::Type::Boolean) return val.boolValue ? "true" : "false";
        } else if constexpr (std::is_same_v<T, double>) {
            if (val.type == Value::Type::Number) return val.numValue;
            if (val.type == Value::Type::String) return toDouble(val.strValue);
            if (val.type == Value::Type::Boolean) return val.boolValue ? 1.0 : 0.0;
        } else if constexpr (std::is_same_v<T, bool>) {
            if (val.type == Value::Type::Boolean) return val.boolValue;
            if (val.type == Value::Type::Number) return val.numValue != 0.0;
            if (val.type == Value::Type::String) return !val.strValue.empty();
        }
        return T{};
    }
    
    bool compareValues(const Value& left, const Value& right, bool equals) {
        if (left.type == Value::Type::Number && right.type == Value::Type::Number) {
            return left.numValue == right.numValue;
        }
        if (left.type == Value::Type::Boolean && right.type == Value::Type::Boolean) {
            return left.boolValue == right.boolValue;
        }
        if (left.type == Value::Type::String || right.type == Value::Type::String) {
            return getValue<std::string>(left) == getValue<std::string>(right);
        }
        return false;
    }
    
    void executeCall(Value& result, CallNode* call) {
        if (!call) return;
        
        // Evaluate arguments
        std::vector<Value> args;
        for (auto& argExpr : call->args) {
            args.push_back(evaluateExpression(argExpr));
        }
        
        // Window methods
        if (result.type == Value::Type::Object && std::holds_alternative<Window*>(result.objValue)) {
            auto win = std::get<Window*>(result.objValue);
            
            if (call->function == "title" && !args.empty()) {
                win->title(toWString(getValue<std::string>(args[0])));
            } else if (call->function == "size" && args.size() >= 2) {
                win->size(static_cast<int>(getValue<double>(args[0])), 
                         static_cast<int>(getValue<double>(args[1])));
            } else if (call->function == "center") {
                win->center(true);
            } else if (call->function == "addLabel" && !args.empty()) {
                result.objValue = win->addLabel(toWString(getValue<std::string>(args[0])));
            } else if (call->function == "addButton" && !args.empty()) {
                result.objValue = win->addButton(toWString(getValue<std::string>(args[0])));
            } else if (call->function == "addInput") {
                std::wstring placeholder = args.empty() ? L"" : toWString(getValue<std::string>(args[0]));
                result.objValue = win->addInput(placeholder);
            } else if (call->function == "addCheckBox" && !args.empty()) {
                result.objValue = win->addCheckBox(toWString(getValue<std::string>(args[0])));
            } else if (call->function == "addComboBox") {
                result.objValue = win->addComboBox();
            } else if (call->function == "addProgressBar") {
                result.objValue = win->addProgressBar();
            } else if (call->function == "addGroupBox") {
                std::wstring text = args.empty() ? L"" : toWString(getValue<std::string>(args[0]));
                result.objValue = win->addGroupBox(text);
            }
        }
        // Button methods
        else if (result.type == Value::Type::Object && std::holds_alternative<Button*>(result.objValue)) {
            auto btn = std::get<Button*>(result.objValue);
            
            if (call->function == "position" && args.size() >= 2) {
                btn->position(static_cast<int>(getValue<double>(args[0])), 
                             static_cast<int>(getValue<double>(args[1])));
            } else if (call->function == "size" && args.size() >= 2) {
                btn->size(static_cast<int>(getValue<double>(args[0])), 
                         static_cast<int>(getValue<double>(args[1])));
            } else if (call->function == "text" && !args.empty()) {
                btn->text(toWString(getValue<std::string>(args[0])));
            } else if (call->function == "onClick") {
                btn->onClick([this]() {
                    Notification::show(L"Button Clicked", L"The button was clicked!");
                });
            } else if (call->function == "setDefault") {
                bool isDefault = args.empty() ? true : getValue<bool>(args[0]);
                btn->setDefault(isDefault);
            }
        }
        // Label methods
        else if (result.type == Value::Type::Object && std::holds_alternative<Label*>(result.objValue)) {
            auto lbl = std::get<Label*>(result.objValue);
            
            if (call->function == "position" && args.size() >= 2) {
                lbl->position(static_cast<int>(getValue<double>(args[0])), 
                             static_cast<int>(getValue<double>(args[1])));
            } else if (call->function == "size" && args.size() >= 2) {
                lbl->size(static_cast<int>(getValue<double>(args[0])), 
                         static_cast<int>(getValue<double>(args[1])));
            } else if (call->function == "text" && !args.empty()) {
                lbl->text(toWString(getValue<std::string>(args[0])));
            } else if (call->function == "font" && args.size() >= 1) {
                int size = static_cast<int>(getValue<double>(args[0]));
                int weight = args.size() >= 2 ? static_cast<int>(getValue<double>(args[1])) : FW_NORMAL;
                lbl->font(size, weight);
            } else if (call->function == "bold") {
                bool enable = args.empty() ? true : getValue<bool>(args[0]);
                lbl->bold(enable);
            }
        }
        // Input methods
        else if (result.type == Value::Type::Object && std::holds_alternative<Input*>(result.objValue)) {
            auto inp = std::get<Input*>(result.objValue);
            
            if (call->function == "position" && args.size() >= 2) {
                inp->position(static_cast<int>(getValue<double>(args[0])), 
                             static_cast<int>(getValue<double>(args[1])));
            } else if (call->function == "size" && args.size() >= 2) {
                inp->size(static_cast<int>(getValue<double>(args[0])), 
                         static_cast<int>(getValue<double>(args[1])));
            } else if (call->function == "text" && !args.empty()) {
                inp->text(toWString(getValue<std::string>(args[0])));
            } else if (call->function == "multiline") {
                bool enable = args.empty() ? true : getValue<bool>(args[0]);
                inp->multiline(enable);
            } else if (call->function == "password") {
                bool enable = args.empty() ? true : getValue<bool>(args[0]);
                inp->password(enable);
            } else if (call->function == "onChange") {
                inp->onChange([this](const std::wstring& value) {
                    // Handle change
                });
            }
        }
        // CheckBox methods
        else if (result.type == Value::Type::Object && std::holds_alternative<CheckBox*>(result.objValue)) {
            auto chk = std::get<CheckBox*>(result.objValue);
            
            if (call->function == "position" && args.size() >= 2) {
                chk->position(static_cast<int>(getValue<double>(args[0])), 
                             static_cast<int>(getValue<double>(args[1])));
            } else if (call->function == "size" && args.size() >= 2) {
                chk->size(static_cast<int>(getValue<double>(args[0])), 
                         static_cast<int>(getValue<double>(args[1])));
            } else if (call->function == "setChecked") {
                bool checked = args.empty() ? true : getValue<bool>(args[0]);
                chk->setChecked(checked);
            }
        }
        // ComboBox methods
        else if (result.type == Value::Type::Object && std::holds_alternative<ComboBox*>(result.objValue)) {
            auto combo = std::get<ComboBox*>(result.objValue);
            
            if (call->function == "position" && args.size() >= 2) {
                combo->position(static_cast<int>(getValue<double>(args[0])), 
                               static_cast<int>(getValue<double>(args[1])));
            } else if (call->function == "size" && args.size() >= 2) {
                combo->size(static_cast<int>(getValue<double>(args[0])), 
                           static_cast<int>(getValue<double>(args[1])));
            } else if (call->function == "addItem" && !args.empty()) {
                combo->addItem(toWString(getValue<std::string>(args[0])));
            } else if (call->function == "setSelectedIndex" && !args.empty()) {
                combo->setSelectedIndex(static_cast<int>(getValue<double>(args[0])));
            }
        }
        // ProgressBar methods
        else if (result.type == Value::Type::Object && std::holds_alternative<ProgressBar*>(result.objValue)) {
            auto prog = std::get<ProgressBar*>(result.objValue);
            
            if (call->function == "position" && args.size() >= 2) {
                prog->position(static_cast<int>(getValue<double>(args[0])), 
                              static_cast<int>(getValue<double>(args[1])));
            } else if (call->function == "size" && args.size() >= 2) {
                prog->size(static_cast<int>(getValue<double>(args[0])), 
                          static_cast<int>(getValue<double>(args[1])));
            } else if (call->function == "setValue" && !args.empty()) {
                prog->setValue(static_cast<int>(getValue<double>(args[0])));
            } else if (call->function == "setRange" && args.size() >= 2) {
                prog->setRange(static_cast<int>(getValue<double>(args[0])),
                              static_cast<int>(getValue<double>(args[1])));
            }
        }
        // GroupBox methods
        else if (result.type == Value::Type::Object && std::holds_alternative<GroupBox*>(result.objValue)) {
            auto grp = std::get<GroupBox*>(result.objValue);
            
            if (call->function == "position" && args.size() >= 2) {
                grp->position(static_cast<int>(getValue<double>(args[0])), 
                             static_cast<int>(getValue<double>(args[1])));
            } else if (call->function == "size" && args.size() >= 2) {
                grp->size(static_cast<int>(getValue<double>(args[0])), 
                         static_cast<int>(getValue<double>(args[1])));
            } else if (call->function == "addLabel" && !args.empty()) {
                result.objValue = grp->addLabel(toWString(getValue<std::string>(args[0])));
            } else if (call->function == "addButton" && !args.empty()) {
                result.objValue = grp->addButton(toWString(getValue<std::string>(args[0])));
            } else if (call->function == "addInput") {
                std::wstring placeholder = args.empty() ? L"" : toWString(getValue<std::string>(args[0]));
                result.objValue = grp->addInput(placeholder);
            } else if (call->function == "addCheckBox" && !args.empty()) {
                result.objValue = grp->addCheckBox(toWString(getValue<std::string>(args[0])));
            } else if (call->function == "addComboBox") {
                result.objValue = grp->addComboBox();
            } else if (call->function == "addProgressBar") {
                result.objValue = grp->addProgressBar();
            }
        }
    }
    
    void executeStatement(std::shared_ptr<ASTNode> stmt) {
        if (auto assignment = std::dynamic_pointer_cast<AssignmentNode>(stmt)) {
            Value result;
            
            if (auto chain = std::dynamic_pointer_cast<ChainNode>(assignment->value)) {
                // Object creation or method call
                if (chain->object == "Window") {
                    if (!currentWindow_) {
                        currentWindow_ = new Window();
                    }
                    result.type = Value::Type::Object;
                    result.objValue = currentWindow_;
                } else if (chain->object == "Notification") {
                    // Handle static Notification.show() calls
                    if (!chain->calls.empty() && chain->calls[0]->function == "show") {
                        auto& args = chain->calls[0]->args;
                        if (args.size() >= 2) {
                            Value title = evaluateExpression(args[0]);
                            Value message = evaluateExpression(args[1]);
                            int duration = args.size() >= 3 ? 
                                static_cast<int>(getValue<double>(evaluateExpression(args[2]))) : 3000;
                            
                            Notification::show(
                                toWString(getValue<std::string>(title)),
                                toWString(getValue<std::string>(message)),
                                duration
                            );
                        }
                    }
                    return; // Don't store static calls
                } else {
                    // Variable reference
                    auto it = variables_.find(chain->object);
                    if (it != variables_.end()) {
                        result = it->second;
                    }
                }
                
                // Execute method chain
                for (auto& call : chain->calls) {
                    executeCall(result, call.get());
                }
                
            } else if (auto expr = std::dynamic_pointer_cast<ExpressionNode>(assignment->value)) {
                result = evaluateExpression(expr);
            }
            
            // Store variable
            variables_[assignment->varName] = result;
            
            // Keep objects alive
            if (result.type == Value::Type::Object) {
                auto stored = std::make_shared<std::variant<Window*, Button*, Label*, Input*, CheckBox*, GroupBox*, ComboBox*, ProgressBar*>>(result.objValue);
                storage_.push_back(stored);
            }
            
        } else if (auto ifNode = std::dynamic_pointer_cast<IfNode>(stmt)) {
            // Evaluate condition
            Value conditionValue = evaluateExpression(ifNode->condition);
            bool condition = getValue<bool>(conditionValue);
            
            if (condition) {
                // Execute then block
                for (auto& s : ifNode->thenBlock) {
                    executeStatement(s);
                }
            } else {
                // Execute else block
                for (auto& s : ifNode->elseBlock) {
                    executeStatement(s);
                }
            }
            
        } else if (auto chain = std::dynamic_pointer_cast<ChainNode>(stmt)) {
            Value result;
            
            // Handle static calls like Notification.show()
            if (chain->object == "Notification") {
                if (!chain->calls.empty() && chain->calls[0]->function == "show") {
                    auto& args = chain->calls[0]->args;
                    if (args.size() >= 2) {
                        Value title = evaluateExpression(args[0]);
                        Value message = evaluateExpression(args[1]);
                        int duration = args.size() >= 3 ? 
                            static_cast<int>(getValue<double>(evaluateExpression(args[2]))) : 3000;
                        
                        Notification::show(
                            toWString(getValue<std::string>(title)),
                            toWString(getValue<std::string>(message)),
                            duration
                        );
                    }
                }
                return;
            }
            
            // Variable reference
            auto it = variables_.find(chain->object);
            if (it != variables_.end()) {
                result = it->second;
                
                // Execute method chain
                for (auto& call : chain->calls) {
                    executeCall(result, call.get());
                }
            }
        }
    }
    
public:
    void execute(std::shared_ptr<ProgramNode> program) {
        for (auto& stmt : program->statements) {
            executeStatement(stmt);
        }
    }
    
    Window* getWindow() { 
        return currentWindow_; 
    }
};

// ============================================================================
// SCRIPT ENGINE
// ============================================================================

class Script {
public:
    static Window* run(const std::string& code) {
        try {
            // Initialize DPI awareness
            DPIManager::Initialize();
            
            Lexer lexer(code);
            auto tokens = lexer.tokenize();
            
            Parser parser(tokens);
            auto ast = parser.parse();
            
            Interpreter interpreter;
            interpreter.execute(ast);
            
            Window* win = interpreter.getWindow();
            
            if (win && !win->getHandle()) {
                win->create();
            }
            
            return win;
            
        } catch (const std::exception& e) {
            std::cerr << "Script error: " << e.what() << "\n";
            MessageBoxA(NULL, e.what(), "Script Error", MB_OK | MB_ICONERROR);
            return nullptr;
        } catch (...) {
            std::cerr << "Unknown script error\n";
            MessageBoxA(NULL, "An unknown error occurred while executing the script", 
                       "Script Error", MB_OK | MB_ICONERROR);
            return nullptr;
        }
    }
    
    static Window* runFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::string error = "Could not open file: " + filename;
            std::cerr << error << "\n";
            MessageBoxA(NULL, error.c_str(), "File Error", MB_OK | MB_ICONERROR);
            return nullptr;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        return run(buffer.str());
    }
};

} // namespace UIBro

#endif // UIBRO_HPP
