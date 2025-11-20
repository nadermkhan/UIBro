#include "UIBro.hpp"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, PWSTR /*pCmdLine*/, int /*nCmdShow*/) {
    // Initialize WTL
    HRESULT hRes = ::CoInitialize(NULL);
    ATLASSERT(SUCCEEDED(hRes));
    
    AtlInitCommonControls(ICC_BAR_CLASSES);
    
    hRes = _Module.Init(NULL, hInstance);
    ATLASSERT(SUCCEEDED(hRes));
    
    // Initialize DPI
    UIBro::DPIManager::Initialize();
    
    // Create window
    UIBro::Window window;
    window.title(L"UIBro WTL Demo")
          ->size(600, 400)
          ->center();
    
    // Add components
    auto* titleLabel = window.addLabel(L"Welcome to UIBro WTL");
    titleLabel->position(20, 20)->size(560, 30)->font(18, FW_BOLD);
    
    auto* nameLabel = window.addLabel(L"Enter your name:");
    nameLabel->position(20, 60)->size(150, 20);
    
    auto* nameInput = window.addInput(L"Type here...");
    nameInput->position(20, 85)->size(300, 25);
    
    auto* submitBtn = window.addButton(L"Submit");
    submitBtn->position(20, 120)->size(100, 30);
    submitBtn->setDefault(true);
    submitBtn->onClick([&nameInput]() {
        std::wstring name = nameInput->getValue();
        if (!name.empty()) {
            UIBro::Notification::show(L"Hello", L"Hello, " + name + L"!");
        } else {
            UIBro::Notification::showWarning(L"Warning", L"Please enter your name first!");
        }
    });
    
    auto* clearBtn = window.addButton(L"Clear");
    clearBtn->position(130, 120)->size(100, 30);
    clearBtn->onClick([&nameInput]() {
        nameInput->setValue(L"");
    });
    
    auto* agreeCheck = window.addCheckBox(L"I agree to terms");
    agreeCheck->position(20, 160)->size(200, 20);
    
    // GroupBox with controls
    auto* group = window.addGroupBox(L"Options");
    group->position(20, 190)->size(350, 150);
    
    auto* optLabel = group->addLabel(L"Select options:");
    optLabel->position(10, 25)->size(200, 20);
    
    auto* opt1 = group->addCheckBox(L"Option 1");
    opt1->position(10, 50)->size(150, 20);
    
    auto* opt2 = group->addCheckBox(L"Option 2");
    opt2->position(10, 75)->size(150, 20);
    
    auto* opt3 = group->addCheckBox(L"Option 3");
    opt3->position(10, 100)->size(150, 20);
    
    // Run the app
    int nRet = window.run();
    
    _Module.Term();
    ::CoUninitialize();
    
    return nRet;
}
