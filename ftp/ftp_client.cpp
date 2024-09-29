#include "ftp_client.h"
#include <iostream>
#include <map>
#include <string>

static bool verbose = false;

bool parse_command(int argc, char *argv[], bool& help, std::string& operation,
                   std::string& param1, std::string& param2) {
    const std::map<std::string, int> valid_commands = {
        {"ls", 2},
        {"mkdir", 2},
        {"rm", 2},
        {"rmdir", 2},
        {"cp", 3},
        {"mv", 3}
    };
    std::vector<std::string> arguments;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            help = true;
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else {
            arguments.push_back(arg);
        }
    }

    if (help) {
        return true;
    }
    if (arguments.size() < 2 || !valid_commands.contains(arguments[0])
        || valid_commands.at(arguments[0]) != arguments.size()) {
        return false;
    }
    operation = arguments[0];
    param1 = arguments[1];
    param2 = arguments.size() > 2 ? arguments[2] : "";
    return true;
}


void print_help() {
    std::cout << "Usage: ./4700ftp [-h] [--verbose] operation params [params ...]" << "\n\n";
    std::cout << "FTP client for listing, copying, moving, and deleting files and directories "
                 "on remote FTP servers." << "\n\n";
    std::cout << "positional arguments:\n";
    std::cout << "operation" << "\t\t" << "The operation to execute. Valid operations are 'ls', 'rm', 'rmdir', "
                                        "'mkdir', 'cp', and 'mv'" << '\n';
    std::cout << "params" << "\t\t\t" << "Parameters for the given operation. "
                                         "Will be one or two paths and/or URLs.\n\n";
    std::cout << "optional arguments:\n";
    std::cout << "-h, --help" << "\t\t" << "show this help message and exit\n";
    std::cout << "--verbose, -v" << "\t\t" << "Print all messages to and from the FTP server\n\n";
    std::cout << "This FTP client supports the following operations:\n";
    std::cout << "ls <URL>" << "\t\t" << "Print out the directory listing from the FTP server at the given URL\n";
    std::cout << "mkdir <URL>" << "\t\t" << "Create a new directory on the FTP server at the given URL\n";
    std::cout << "rm <URL>" << "\t\t" << "Delete the file on the FTP server at the given URL\n";
    std::cout << "rmdir <URL>" << "\t\t" << "Delete the directory on the FTP server at the given URL\n";
    std::cout << "cp <ARG1> <ARG2>" << '\t' << "Copy the file given by ARG1 to the file given by ARG2. "
                                               "If ARG1 is a local file, then ARG2 must be a URL, and vice-versa.\n";
    std::cout << "mv <ARG1> <ARG2>" << '\t' << "Move the file given by ARG1 to the file given by ARG2. "
                                               "If ARG1 is a local file, then ARG2 must be a URL, and vice-versa.\n";

    exit(0);
}


int main(int argc, char *argv[]) {
    std::string operation, param1, param2;
    bool help = false;
    if (!parse_command(argc, argv, help, operation, param1, param2) || help) {
        print_help();
    }

}
