#ifndef FTP_FTP_CLIENT_H
#define FTP_FTP_CLIENT_H

bool parse_command(int argc, char *argv[], bool& help, std::string& operation,
                   std::string& param1, std::string& param2);

#endif
