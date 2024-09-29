#ifndef FTP_FTP_CLIENT_H
#define FTP_FTP_CLIENT_H

#define DEFAULT_NAME "anonymous"
#define DEFAULT_PORT "21"

struct FTP {
    std::string protocol;
    std::string username;
    std::string password;
    std::string host;
    std::string port;
    std::string path;
};

/**
 * Parse command line arguments, which should have the format `./4700ftp [operation] [param1] [param2]`.
 * @param argc number of arguments.
 * @param argv array of arguments.
 * @param help reference to an optional help variable.
 * @param operation reference to the operation variable that is to be executed.
 * @param param1 reference to the first parameter.
 * @param param2 reference to the second parameter.
 * @return true if the command line argument is valid, false on error.
 */
bool parse_command(int argc, char *argv[], bool& help, std::string& operation,
                   std::string& param1, std::string& param2);

/**
 * Parse the URL of the FTP server, which should have the format `ftp://[USER[:PASSWORD]@]HOST[:PORT]/PATH`.
 * @param param1 the first parameter in the command-line arguments.
 * @param param2 the second parameter in the command-line arguments.
 * @param ftp_url an output parameter for the FTP struct.
 * @return true if one of the input successfully matched with the URL format, false on error.
 */
bool parse_url(const std::string& param1, const std::string& param2, FTP& ftp_url);

/**
 * Print a help message to the shell and exit the program.
 */
void print_help();

/**
 * Establish a connection with the FTP server.
 * @param ftp a struct containing the FTP server info.
 * @return the socket descriptor if success, -1 on error.
 */
int open_clientfd(const FTP& ftp);

#endif
