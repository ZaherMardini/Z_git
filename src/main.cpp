
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <zlib.h>
#include <vector>
using namespace std;
namespace fs = std::filesystem;

void init(){
    fs::create_directories(".z_git/objects");
    fs::create_directories(".z_git/refs");

    ofstream headFile(".z_git/HEAD");

    if (!headFile) {
        throw runtime_error("Failed to open .z_git/HEAD");
    }

    headFile << "ref: refs/heads/main\n";

    if (!headFile) {
        throw runtime_error("Failed to write to .z_git/HEAD");
    }

    headFile.close();

    if (!headFile) {
        throw runtime_error("Failed to close .z_git/HEAD");
    }
}

string createPath(const string& hash){
    if (hash.size() != 40) {
        throw invalid_argument("Invalid SHA-1 hash length");
    }
    for (unsigned char c : hash) {
        if (!isxdigit(c)) {
            throw invalid_argument("Invalid SHA-1 hash");
        }
    }
    return ".git/objects/" + hash.substr(0, 2) + "/" + hash.substr(2);// .git just for valid example
}

string catFile(string hash){
    string path = createPath(hash);
    // Start: load the compressed file from it's directory to the buffer
    ifstream compressedBytes(path,ios::binary);
    if(!compressedBytes){
        throw runtime_error("Could not open object: " + path);
    }
    compressedBytes.seekg(0,ios::end);
    streamsize size = compressedBytes.tellg();
    if (size <= 0) {
        throw runtime_error("Object file is empty or unreadable");
    }
    compressedBytes.seekg(0,ios::beg);
    vector<char> source(static_cast<size_t>(size));

    if (!compressedBytes.read(source.data(), size)) {
        throw runtime_error("Failed to read object file");
    }
    // char type is necessary to make the reading process happy, but then we need to reinterpret it to Bytef
    // to make "uncompress" happy
    compressedBytes.read(source.data(),size);
    // End: load the compressed file from it's directory to the buffer
    ulong destLen = size * 2048;
    vector<Bytef>dest(destLen);
    int result = uncompress(dest.data(), &destLen, reinterpret_cast<const Bytef*>(source.data()), source.size());
    if (result != Z_OK) {
        throw runtime_error(
            "Decompression failed. Zlib error code: " + to_string(result)
        );
    }
    dest.resize(destLen);
    // Start: Extracting the content
    size_t headerEndIndex = 0;
    while(headerEndIndex < dest.size() && dest[headerEndIndex] != '\0'){
        headerEndIndex++;
    }
    if (headerEndIndex == dest.size()) {
        throw runtime_error("Invalid Git object: missing header separator");
    }
    int contentStartIndex = headerEndIndex++;
    string content = ""; 
    for (size_t i = contentStartIndex; i < dest.size(); i++) {
        content += static_cast<char>(dest[i]);
    }
    return content;
    // End: Extracting the content
}

int main(int argc, char* argv[])
{
    try {
        if (argc < 2) {
            throw invalid_argument("Usage: ./<bin> <command>");
        }

        const string command = argv[1];

        if (command == "init") {
            if (argc != 2) {
                throw invalid_argument("Usage: ./<bin> init");
            }

            init();

            cout << "Git initialized successfully\n";
            return EXIT_SUCCESS;
        }

        if (command == "cat-file") {
            if (argc != 4 || string(argv[2]) != "-p") {
                throw invalid_argument(
                    "Usage: ./<bin> cat-file -p <hash>"
                );
            }

            string hash = argv[3];
            string content = catFile(hash);

            cout.write(content.data(), content.size());

            return EXIT_SUCCESS;
        }

        throw invalid_argument("Unknown command: " + command);
    }
    catch (const exception& e) {
        cerr << "Error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
}
// Super important and need documentation:
// Dealing with blobs: https://chatgpt.com/c/6adcb49a-2b78-83ed-8234-d90d06844090
// Error handling:     https://chatgpt.com/c/6ae4f995-f124-83ed-93e9-b74e24849ca0