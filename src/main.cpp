
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <zlib.h>
#include <vector>
#include <openssl/sha.h>
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
    return "Cat-file-test/" + hash.substr(0, 2) + "/" + hash.substr(2);// .git just for valid example
    // return ".git/objects/" + hash.substr(0, 2) + "/" + hash.substr(2);// .git just for valid example
}

string catFile(string hash){
    string path = createPath(hash);
    // Start: load the compressed contentFile from it's directory to the buffer
    ifstream compressedBytes(path,ios::binary);
    if(!compressedBytes){
        throw runtime_error("Could not open object: " + path);
    }
    compressedBytes.seekg(0,ios::end);
    streamsize size = compressedBytes.tellg();// bug: very large/ negative
    size = static_cast<size_t>(size);
    if (size < 0) {
        throw runtime_error("Object contentFile is unreadable");
    }
    compressedBytes.seekg(0,ios::beg);
    vector<char> source(size);

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

string toHex(vector<unsigned char>& bytes){ // vector contains 20 raw bytes, each byte can make 2 hex numbers 
    const char hexChars[] = "0123456789abcdef";// array of hex characters (lookup table)
    string result = "";
    result.reserve(bytes.size()*2); // each hex number is 0.5 byte (4bits)
    for(unsigned char byte : bytes){  
         // bitwise operation to construct a hex number
        result += hexChars[byte >> 4]; // shifting 4 bits to the right to get the first 4 bits
        result += hexChars[byte & 0x0F]; // bitwise & with the binary 1111 to get the last 4 bits
    }   
    return result;
}

string hashObject(string filePath){
    // Uncompressed blob -> compute hash -> compress

    // Construct the blob header
    ifstream contentFile(filePath);
    if(!contentFile){
        throw runtime_error("File failed to open");
    }
    contentFile.seekg(0,ios::end);
    size_t contentFileSize = contentFile.tellg();
    if(contentFileSize < 0){
        throw runtime_error("Content file is unreadable!");
    }
    contentFile.seekg(0,ios::beg);
    string blob = "blob " + to_string(contentFileSize) + '\0';
    // Load the blob header then content into a buffer
    vector<char>contentBuffer(contentFileSize);
    if(!contentFile.read(contentBuffer.data(), contentBuffer.size())){
        throw runtime_error("Failed to read file content");
    };
    for(char c : contentBuffer){
        blob += c;
    }
    if (blob.size() > numeric_limits<uLong>::max()){
        throw length_error("Blob is too large for zlib");
    }
    // Compute hash and create hash string
    vector<unsigned char>hashedContent(SHA_DIGEST_LENGTH);
    SHA1(reinterpret_cast<const unsigned char*>(blob.data()), blob.size(), hashedContent.data());// produces a vector with 20 raw bytes 
    // error handling if the hash isn't done 
    string hash = toHex(hashedContent); // why not .data()?
    // Write the new object to .git/objects/hash.substr(0,2)/hash.substr(2)
    contentBuffer.clear();
    uLong compressSize = compressBound(static_cast<ulong>(blob.size()));
    contentBuffer.resize(compressSize);
    int result = 
    compress(
        reinterpret_cast<Bytef*>(contentBuffer.data()), 
        &compressSize, 
        reinterpret_cast<Bytef*>(blob.data()), 
        blob.size()
    );
    if(result != Z_OK){
        throw runtime_error("Compression failed!");
    }
    contentBuffer.resize(compressSize);
    string path = "Cat-file-test/" + hash.substr(0,2);
    fs::create_directories(path);  
    path = path + "/" + hash.substr(2);
    ofstream objectFile(path,ios::binary);
    if(!objectFile){
        throw runtime_error("Failed to open the file");
    }
    objectFile.write(reinterpret_cast<char*>(contentBuffer.data()), contentBuffer.size());
    if(!objectFile){
        throw runtime_error("Failed to write on the file");
    }
    return hash;
}
int main(int argc, char* argv[])
{
    try {
        if (argc < 2) {
            throw invalid_argument("Usage: ./<bin> <command>");
        }

        const string command = argv[1];
        const string flag    = argv[2];

        if (command == "init") {
            if (argc != 2) {
                throw invalid_argument("Usage: ./<bin> init");
            }

            init();

            cout << "Git initialized successfully\n";
            return EXIT_SUCCESS;
        }

        if (command == "cat-file") {
            if (argc != 4 || flag != "-p") {
                throw invalid_argument(
                    "Usage: ./<bin> cat-file -p <hash>"
                );
            }

            string hash = argv[3];
            string content = catFile(hash);

            cout.write(content.data(), content.size());

            return EXIT_SUCCESS;
        }

        if(command == "hash-object"){
            if(argc < 4 || flag != "-w"){
                throw invalid_argument("Usage: ./<Bin> hash-object -w <filePath>");
            }
            string filePath = argv[3];
            hashObject(filePath);
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