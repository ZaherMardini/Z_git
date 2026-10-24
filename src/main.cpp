
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

int init_command()
{
    try {
        fs::create_directories(".z_git/objects");
        fs::create_directories(".z_git/refs");

        std::ofstream headFile(".z_git/HEAD");

        if (!headFile) {
            std::cerr << "Failed to open .z_git/HEAD\n";
            return EXIT_FAILURE;
        }

        headFile << "ref: refs/heads/main";
        headFile.close();

        if (!headFile) {
            std::cerr << "Failed to write to .z_git/HEAD\n";
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: ./build_your_git <command>\n";
        return EXIT_FAILURE;
    }

    const std::string command = argv[1];

    if (command == "init") {
        if (init_command() != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }

        std::cout << "Git initialized successfully\n";
        return EXIT_SUCCESS;
    }

    std::cerr << "Unknown command: " << command << '\n';
    return EXIT_FAILURE;
}
