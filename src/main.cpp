#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
using namespace std;
int main(int argc, char *argv[])
{
    cout << unitbuf;
    cerr << unitbuf;
    if(argc < 2){
        cout << "Usage: ./<program name> <command>\n";
        return EXIT_FAILURE;
    }
    string command = argv[1];
    if(command == "init"){
        try{
            filesystem::create_directories(".z_git/objects");
            filesystem::create_directories(".z_git/refs");
            // making the file and writing to it
            ofstream headFile(".z_git/HEAD");
            if(headFile.is_open()){
                headFile << "ref: refs/heads/main";
                headFile.close();
            }
            if(!headFile){
                cerr << "Writing to file failed\n";
            }
        }
        catch(filesystem::filesystem_error &e){
            cerr << e.what();
            return EXIT_FAILURE;
        }
    }
    else{
        cerr << "Unknown command\n";
        return EXIT_FAILURE;
    }
}
