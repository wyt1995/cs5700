#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include "ftp_client.h"

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
        || valid_commands.at(arguments[0]) != static_cast<int>(arguments.size())) {
        return false;
    }
    operation = arguments[0];
    param1 = arguments[1];
    param2 = arguments.size() > 2 ? arguments[2] : "";
    return true;
}


bool parse_url(const std::string& param1, const std::string& param2, FTP& ftp_url) {
    std::regex url_pattern(R"(^(ftp):\/\/(?:([^:@]+)(?::([^@]+))?@)?([^:\/?#]+)(?::(\d+))?(\/[^?#]*)?$)");
    std::smatch matches;

    for (const std::string& param: {param1, param2}) {
        if (std::regex_match(param, matches, url_pattern)) {
            ftp_url.protocol = matches[1].str();
            ftp_url.username = matches[2].length() ? matches[2].str() : DEFAULT_NAME;
            ftp_url.password = matches[3].str();
            ftp_url.host = matches[4].str();
            ftp_url.port = matches[5].length() ? matches[5].str() : DEFAULT_PORT;
            ftp_url.path = matches[6].length() ? matches[6].str() : "/";
            return !ftp_url.host.empty();  // host name is required
        }
    }
    return false;
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


int open_clientfd(const std::string& host, const std::string& port) {
    int sockfd = -1, rc = -1;
    struct addrinfo hints, *addr_list, *ptr;

    // get a list of potential server addresses
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if ((rc = getaddrinfo(host.c_str(), port.c_str(), &hints, &addr_list)) != 0) {
        std::cerr << "getaddrinfo error " << gai_strerror(rc) << std::endl;
        return -1;
    }

    // iterate over result list for one that we can successfully connect to
    for (ptr = addr_list; ptr; ptr = ptr->ai_next) {
        if ((sockfd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) < 0) {
            continue;
        }
        if (connect(sockfd, ptr->ai_addr, ptr->ai_addrlen) != -1) {
            break;
        }
        // connection failed, try another
        close(sockfd);
    }

    // clean up data structure
    freeaddrinfo(addr_list);
    return ptr ? sockfd : -1;
}


void send_message(int sockfd, const std::string& cmd, const std::string& param="") {
    std::string msg;
    if (param.empty()) {
        msg = cmd + "\r\n";
    } else {
        msg = cmd + " " + param + "\r\n";
    }
    const char* buf = msg.c_str();

    ssize_t total = 0, bytes_sent = 0;
    auto msg_size = static_cast<ssize_t>(msg.length());
    while (total < msg_size) {
        if ((bytes_sent = send(sockfd, buf + total, msg_size - total, 0)) == -1) {
            std::cerr << "Failed to send command: " << msg;
            exit(1);
        }
        total += bytes_sent;
    }
}


std::string read_response(int sockfd) {
    char buf[1028];
    std::string message;
    ssize_t bytes_received;

    while (true) {
        if ((bytes_received = recv(sockfd, buf, sizeof(buf), 0)) <= 0) {
            std::cerr << "Failed to read response from server" << std::endl;
            exit(1);
        }
        message.append(buf, bytes_received);

        std::string::size_type endline = message.find("\r\n");
        if (endline != std::string::npos) {
            return message.substr(0, endline);
        }
    }
}


int response_code(std::string& response) {
    if (verbose) {
        std::cout << response << '\n';
    }
    if (response.length() < 3) {
        return -1;
    }
    return std::stoi(response.substr(0, 3));
}


bool pre_operation(int sockfd, const FTP& ftp) {
    std::string response;
    int code;

    // read hello message from the FTP server
    response = read_response(sockfd);
    if ((code = response_code(response)) != CODE_READY) {
        std::cerr << "Unexpected welcome message: " << response << '\n';
        return false;
    }

    // send username and password
    send_message(sockfd, "USER", ftp.username);
    response = read_response(sockfd);
    if ((code = response_code(response)) == CODE_LOGIN) {
        // user logged in, proceed with no further action
        ;
    } else if (code == CODE_REQPW) {
        send_message(sockfd, "PASS", ftp.password);
        response = read_response(sockfd);
        if ((code = response_code(response)) != CODE_LOGIN) {
            std::cerr << "Password error: " << response << '\n';
            return false;
        }
    } else {
        std::cerr << "Username error: " << response << '\n';
        return false;
    }

    // set the connection to 8-bit binary data mode
    send_message(sockfd, "TYPE", "I");
    response = read_response(sockfd);
    if ((code = response_code(response)) != CODE_CMPLT) {
        std::cerr << "Type command error: " << response << '\n';
        return false;
    }

    // set the connection to stream mode
    send_message(sockfd, "MODE", "S");
    response = read_response(sockfd);
    if ((code = response_code(response)) != CODE_CMPLT) {
        std::cerr << "Mode command error: " << response << '\n';
        return false;
    }

    send_message(sockfd, "STRU", "F");
    response = read_response(sockfd);
    if ((code = response_code(response)) != CODE_CMPLT) {
        std::cerr << "Structure command error: " << response << '\n';
        return false;
    }

    // all operations succeeded
    return true;
}


bool parse_pasv_response(const std::string& msg, std::string& ip, std::string& port) {
    std::regex pasv_regex(R"(\((\d+),(\d+),(\d+),(\d+),(\d+),(\d+)\))");
    std::smatch matches;

    if (std::regex_search(msg, matches, pasv_regex)) {
        ip = matches[1].str() + "." + matches[2].str() + "." + matches[3].str() + "." + matches[4].str();
        int p1 = std::stoi(matches[5].str());
        int p2 = std::stoi(matches[6].str());
        port = std::to_string((p1 << 8) + p2);
        return true;
    }
    return false;
}


int open_data_channel(int control_sockfd) {
    send_message(control_sockfd, "PASV");
    std::string response = read_response(control_sockfd);
    int code = response_code(response);
    if (code != CODE_PSVMD) {
        std::cerr << "Entering passive mode error: " << response << '\n';
        return -1;
    }

    std::string ip;
    std::string port;
    if (!parse_pasv_response(response, ip, port)) {
        std::cerr << "Failed to parse PASV response: " << response << '\n';
        return -1;
    }

    int data_sockfd = open_clientfd(ip, port);
    if (data_sockfd < 0) {
        std::cerr << "Failed to open data channel\n";
        return -1;
    }
    return data_sockfd;
}


bool list_directory(int control_sockfd, const std::string& path) {
    // open data channel for file transfer
    int data_sockfd;
    if ((data_sockfd = open_data_channel(control_sockfd)) < 0) {
        return false;
    }

    // send command through control channel
    send_message(control_sockfd, "LIST", path);
    std::string response = read_response(control_sockfd);
    int code = response_code(response);
    if (code != CODE_STXFR) {
        std::cerr << "Failed to start ls command " << response << '\n';
        close(data_sockfd);
        return false;
    }

    // receive file listing, then close data channel
    char buffer[4096];
    std::string listing;
    ssize_t bytes_received;
    while ((bytes_received = recv(data_sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        listing.append(buffer, bytes_received);
    }
    if (bytes_received < 0) {
        std::cerr << "Data recv error\n";
        close(data_sockfd);
        return false;
    }
    close(data_sockfd);

    // check control channel status
    response = read_response(control_sockfd);
    if ((code = response_code(response)) != CODE_DSUCC) {
        std::cerr << "Failed to finish ls command " << response << '\n';
        return false;
    }

    std::cout << listing;
    return true;
}


bool make_directory(int sockfd, const std::string& dir) {
    send_message(sockfd, "MKD", dir);
    std::string response = read_response(sockfd);
    int code = response_code(response);
    if (code != CODE_CRDIR) {
        std::cerr << "Failed to create directory " << dir << '\n';
        return false;
    }
    if (verbose) {
        std::cout << "Created directory " << dir << '\n';
    }
    return true;
}


bool remove_directory(int sockfd, const std::string& dir) {
    send_message(sockfd, "RMD", dir);
    std::string response = read_response(sockfd);
    int code = response_code(response);
    if (code != CODE_FSUCC) {
        std::cerr << "Failed to remove directory " << dir << '\n';
        return false;
    }
    if (verbose) {
        std::cout << "Removed directory " << dir << '\n';
    }
    return true;
}


bool remove_file(int sockfd, const std::string& dir) {
    send_message(sockfd, "DELE", dir);
    std::string response = read_response(sockfd);
    int code = response_code(response);
    if (code != CODE_FSUCC) {
        std::cerr << "Failed to remove file " << dir << '\n';
        return false;
    }
    if (verbose) {
        std::cout << "Removed file " << dir << '\n';
    }
    return true;
}


bool upload_file(int control_sockfd, std::string& local_path, std::string& remote_path) {
    // handle remote file name
    if (remote_path.back() == '/') {
        std::string filename = local_path.substr(local_path.find_last_of('/') + 1);
        remote_path += filename;
    }

    // open local file for reading
    std::ifstream local_file(local_path, std::ios::binary);
    if (!local_file) {
        std::cerr << "Failed to open local file for reading " << local_path << '\n';
        return false;
    }

    // open data channel
    int data_sockfd;
    if ((data_sockfd = open_data_channel(control_sockfd)) < 0) {
        local_file.close();
        return false;
    }

    // send STOR command through control channel
    send_message(control_sockfd, "STOR", remote_path);
    std::string response = read_response(control_sockfd);
    int code = response_code(response);
    if (code != CODE_STXFR) {
        std::cerr << "Failed to start upload " << response << '\n';
        close(data_sockfd);
        local_file.close();
        return false;
    }

    // send binary file through data channel
    char buffer[4096];
    ssize_t total, sent, remain;
    while (local_file.read(buffer, sizeof(buffer)) || local_file.gcount() > 0) {
        total = 0;
        remain = local_file.gcount();
        while (total < remain) {
            if ((sent = send(data_sockfd, buffer + total, remain - total, 0)) == -1) {
                std::cerr << "Error sending file: " << strerror(errno) << '\n';
                close(data_sockfd);
                local_file.close();
                return false;
            }
            total += sent;
        }
    }

    // clean up
    local_file.close();
    close(data_sockfd);

    response = read_response(control_sockfd);
    if ((code = response_code(response)) != CODE_DSUCC) {
        std::cerr << "Failed to finish STOR command " << response << '\n';
        return false;
    }
    if (verbose) {
        std::cout << "Success: file uploaded as " << remote_path << '\n';
    }
    return true;
}


bool download_file(int control_sockfd, std::string& remote_path, std::string& local_path) {
    // handle local file name
    if (local_path.empty() || local_path.back() == '/') {
        std::string filename = remote_path.substr(remote_path.find_last_of('/') + 1);
        local_path += filename;
    }

    // open local file for writing
    std::ofstream outfile(local_path, std::ios::binary);
    if (!outfile) {
        std::cerr << "Failed to open local file for writing " << local_path << '\n';
        return false;
    }

    // open data channel
    int data_sockfd;
    if ((data_sockfd = open_data_channel(control_sockfd)) < 0) {
        outfile.close();
        return false;
    }

    // send RETR command through control channel
    send_message(control_sockfd, "RETR", remote_path);
    std::string response = read_response(control_sockfd);
    int code = response_code(response);
    if (code != CODE_STXFR) {
        std::cerr << "Failed to start download " << response << '\n';
        close(data_sockfd);
        outfile.close();
        return false;
    }

    // receive file data through data channel
    char buffer[4096];
    ssize_t bytes_received;
    while ((bytes_received = recv(data_sockfd, buffer, sizeof(buffer), 0)) > 0) {
        outfile.write(buffer, bytes_received);
    }
    if (bytes_received < 0) {
        std::cerr << "Error receiving file: " << strerror(errno) << '\n';
        close(data_sockfd);
        outfile.close();
        return false;
    }

    // clean up
    outfile.close();
    close(data_sockfd);
    response = read_response(control_sockfd);
    if ((code = response_code(response)) != CODE_DSUCC) {
        std::cerr << "Failed to finish RETR command " << response << '\n';
        return false;
    }
    if (verbose) {
        std::cout << "Success: file downloaded as " << local_path << '\n';
    }
    return true;
}


void quit_connection(int sockfd) {
    send_message(sockfd, "QUIT");
    std::string response = read_response(sockfd);
    int code = response_code(response);
    if (code != CODE_CLOSE) {
        std::cerr << "Quit error" << response << '\n';
        close(sockfd);
        exit(1);
    }
}


int main(int argc, char *argv[]) {
    std::string operation, param1, param2;
    bool help = false;
    if (!parse_command(argc, argv, help, operation, param1, param2) || help) {
        print_help();
    }

    FTP ftp_info;
    if (!parse_url(param1, param2, ftp_info)) {
        std::cerr << "URL format - ftp://[USER[:PASSWORD]@]HOST[:PORT]/PATH" << std::endl;
        exit(1);
    }

    int sockfd;
    if ((sockfd = open_clientfd(ftp_info.host, ftp_info.port)) < 0) {
        std::cerr << "Failed to connect to " << ftp_info.host << std::endl;
        exit(1);
    }

    if (!pre_operation(sockfd, ftp_info)) {
        std::cerr << "Connection error." << std::endl;
        close(sockfd);
        exit(1);
    }

    bool success = false;
    if (operation == "ls") {
        success = list_directory(sockfd, ftp_info.path);
    } else if (operation == "mkdir") {
        success = make_directory(sockfd, ftp_info.path);
    } else if (operation == "rmdir") {
        success = remove_directory(sockfd, ftp_info.path);
    } else if (operation == "rm") {
        success = remove_file(sockfd, ftp_info.path);
    } else {
        bool is_download = param1.find("ftp://") == 0;
        if (is_download) {
            success = download_file(sockfd, ftp_info.path, param2);
        } else {
            success = upload_file(sockfd, param1, ftp_info.path);
        }

        // mv command remove file from source
        if (operation == "mv" && success) {
            if (is_download) {
                success = remove_file(sockfd, ftp_info.path);
            } else {
                success = std::remove(param1.c_str());
            }
        }
    }

    if (!success) {
        close(sockfd);
        return 0;
    }

    quit_connection(sockfd);
    close(sockfd);
    return 0;
}
