#include "UIBro.hpp"
#include <iostream>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

void printUsage() {
    std::cout << R"(
UIBro - Windows 10 UI Framework
Version 3.1.0

USAGE:
    uibro <script.ui>       Run a UI script file
    uibro --help            Show this help message
    uibro --version         Show version information

EXAMPLES:
    uibro app.ui            Run app.ui script
    uibro myform.ui         Run myform.ui script

SCRIPT SYNTAX:
    // Create window
    win = Window.title("My App").size(800, 600).center();
    
    // Add components
    label = win.addLabel("Hello World").position(20, 20).size(300, 30);
    btn = win.addButton("Click Me").position(20, 60).size(100, 30);
    input = win.addInput("Type here").position(20, 100).size(200, 24);
    
    // Add event handlers
    btn.onClick();
    
    // Conditional logic
    if (condition) {
        Notification.show("Title", "Message");
    }

COMPONENTS:
    Window, Button, Label, Input, CheckBox, ComboBox, 
    ProgressBar, GroupBox, Notification

For more information, visit: https://github.com/nadermkhan/UIBro/
)" << std::endl;
}

void printVersion() {
    std::cout << "UIBro v3.1.0 - Windows 10 Native UI Framework" << std::endl;
    std::cout << "Built with MSVC - Windows Runtime" << std::endl;
    std::cout << "Copyright (c) 2024 - MIT License" << std::endl;
}

bool fileExists(const std::string& filename) {
    return PathFileExistsA(filename.c_str()) == TRUE;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    // Get command line arguments
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    
    // Convert to regular strings
    std::vector<std::string> args;
    for (int i = 0; i < argc; i++) {
        int size = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, NULL, 0, NULL, NULL);
        std::string str(size, 0);
        WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, &str[0], size, NULL, NULL);
        str.resize(size - 1); // Remove null terminator
        args.push_back(str);
    }
    LocalFree(argv);
    
    // Allocate console for output
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    freopen_s(&fp, "CONIN$", "r", stdin);
    
    // Parse arguments
    if (argc == 1) {
        // No arguments - show usage
        printUsage();
        std::cout << "\nError: No script file specified!" << std::endl;
        std::cout << "\nPress Enter to exit...";
        std::cin.get();
        return 1;
    }
    
    std::string firstArg = args[1];
    
    // Handle flags
    if (firstArg == "--help" || firstArg == "-h") {
        printUsage();
        std::cout << "\nPress Enter to exit...";
        std::cin.get();
        return 0;
    }
    
    if (firstArg == "--version" || firstArg == "-v") {
        printVersion();
        std::cout << "\nPress Enter to exit...";
        std::cin.get();
        return 0;
    }
    
    // Check if file exists
    if (!fileExists(firstArg)) {
        std::cerr << "Error: File '" << firstArg << "' not found!" << std::endl;
        std::cerr << "\nMake sure the file exists and the path is correct." << std::endl;
        std::cout << "\nPress Enter to exit...";
        std::cin.get();
        return 1;
    }
    
    // Run the script
    std::cout << "Loading script: " << firstArg << std::endl;
    std::cout << "Initializing UIBro..." << std::endl;
    
    UIBro::Window* window = UIBro::Script::runFile(firstArg);
    
    if (!window) {
        std::cerr << "\nError: Failed to create window from script!" << std::endl;
        std::cerr << "Check the script syntax and try again." << std::endl;
        std::cout << "\nPress Enter to exit...";
        std::cin.get();
        return 1;
    }
    
    std::cout << "Script loaded successfully!" << std::endl;
    std::cout << "Starting UI..." << std::endl;
    
    // Close console and run GUI
    FreeConsole();
    return window->run();
}
