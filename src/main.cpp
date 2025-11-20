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
    uibro example.ui        Run the example script

SCRIPT SYNTAX:
    win = Window.title("My App").size(800, 600).center();
    btn = win.addButton("Click Me").position(10, 10).size(100, 30);
    btn.onClick();
    
    if (condition) {
        Notification.show("Title", "Message");
    }

For more information, visit: https://github.com/yourusername/UIBro
)" << std::endl;
}

void printVersion() {
    std::cout << "UIBro v3.1.0 - Windows 10 Native UI Framework" << std::endl;
    std::cout << "Built with MSVC - Windows Runtime" << std::endl;
}

bool fileExists(const std::string& filename) {
    return PathFileExistsA(filename.c_str()) == TRUE;
}

std::string getExecutableDirectory() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");
    return std::string(buffer).substr(0, pos);
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
    if (argc > 1) {
        AllocConsole();
        FILE* fp;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
    }
    
    // Parse arguments
    if (argc == 1) {
        // No arguments - show usage
        printUsage();
        
        // Run default example if it exists
        std::string exeDir = getExecutableDirectory();
        std::string examplePath = exeDir + "\\example.ui";
        
        if (fileExists(examplePath)) {
            std::cout << "\nRunning example.ui...\n" << std::endl;
            UIBro::Window* window = UIBro::Script::runFile(examplePath);
            if (window) {
                FreeConsole();
                return window->run();
            }
        }
        
        std::cout << "\nPress Enter to exit...";
        std::cin.get();
        return 0;
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
        std::cout << "\nPress Enter to exit...";
        std::cin.get();
        return 1;
    }
    
    // Run the script
    std::cout << "Loading script: " << firstArg << std::endl;
    UIBro::Window* window = UIBro::Script::runFile(firstArg);
    
    if (!window) {
        std::cerr << "Error: Failed to create window from script!" << std::endl;
        std::cout << "\nPress Enter to exit...";
        std::cin.get();
        return 1;
    }
    
    // Close console and run GUI
    FreeConsole();
    return window->run();
}
