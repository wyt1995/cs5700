#ifndef FTP_FTP_CLIENT_H
#define FTP_FTP_CLIENT_H

#define DEFAULT_NAME "anonymous"
#define DEFAULT_PORT "21"

#define CODE_STXFR 150
#define CODE_CMPLT 200
#define CODE_READY 220
#define CODE_CLOSE 221
#define CODE_DSUCC 226
#define CODE_PSVMD 227
#define CODE_LOGIN 230
#define CODE_FSUCC 250
#define CODE_CRDIR 257
#define CODE_REQPW 331

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
 * @param host domain name of the server.
 * @param port the TCP port in decimal that the server is listening on.
 * @return the socket descriptor if success, -1 on error.
 */
int open_clientfd(const std::string& host, const std::string& port);

/**
 * Send a message to the server; exit the program if failed.
 * @param sockfd the socket descriptor used by I/O system calls.
 * @param cmd the command to be sent.
 * @param param an option parameter, default to be an empty string.
 */
void send_message(int sockfd, const std::string& cmd, const std::string& param);

/**
 * Receive a message from server. The message must ends with '\r\n'.
 * @param sockfd the socket descriptor used by I/O system calls.
 * @return the string message received by the client.
 */
std::string read_response(int sockfd);

/**
 * Parse server response message. If verbose mode is set, print out the response.
 * @param response the string response received by the client.
 * @return the FTP server response code.
 */
int response_code(std::string& response);

/**
 * Send USER, PASS, TYPE, MODE, STRU commands to the FTP server before any file operation.
 * @param sockfd the socket descriptor used by I/O system calls.
 * @param ftp a struct containing the FTP server info.
 * @return true if all commands are successful, false on error.
 */
bool pre_operation(int sockfd, const FTP& ftp);

/**
 * Send a QUIT command to the FTP server.
 */
void quit_connection(int sockfd);

/**
 * Parse the server response to the PASV command.
 * @param msg the response message.
 * @param ip output parameter for the IP address.
 * @param port output parameter for the port number.
 * @return true if okay, false on error.
 */
bool parse_pasv_response(const std::string& msg, std::string& ip, std::string& port);

/**
 * Open a data channel for uploading or downloading files.
 * It sends a PASV command to the server, receives a response, and opens a new socket connection.
 * @param control_sockfd the socket descriptor of the control channel.
 * @return the socket descriptor of the data channel if success, -1 on any error.
 */
int open_data_channel(int control_sockfd);

/**
 * List all files under the given directory in the FTP server.
 * @param control_sockfd the socket descriptor of the control channel.
 * @param path the remote directory path.
 * @return true if okay, false on error.
 */
bool list_directory(int control_sockfd, const std::string& path);

/**
 * Make a new directory under the given path.
 * @param sockfd the socket descriptor of the control channel.
 * @param dir the remote directory path.
 * @return true if okay, false on error.
 */
bool make_directory(int sockfd, const std::string& dir);

/**
 * Remove the specified directory from the remote server.
 * @param sockfd the socket descriptor of the control channel.
 * @param dir the remote directory path.
 * @return true if okay, false on error.
 */
bool remove_directory(int sockfd, const std::string& dir);

/**
 * Remove the given file from the remote server.
 * @param sockfd the socket descriptor of the control channel.
 * @param dir the remote directory path.
 * @return true if okay, false on error.
 */
bool remove_file(int sockfd, const std::string& dir);

/**
 * Upload local file to the remote FTP server.
 * @param control_sockfd the socket descriptor of the control channel.
 * @param local_path path to the local file.
 * @param remote_path path to the remote file.
 * @return true if okay, false on error.
 */
bool upload_file(int control_sockfd, std::string& local_path, std::string& remote_path);

/**
 * Download file from the remote FTP server.
 * @param control_sockfd the socket descriptor of the control channel.
 * @param remote_path path to the remote file.
 * @param local_path path to the local file.
 * @return true if okay, false on error.
 */
bool download_file(int control_sockfd, std::string& remote_path, std::string& local_path);

#endif
