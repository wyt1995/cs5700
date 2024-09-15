#include <algorithm>
#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <nlohmann/json.hpp>
#include "client.h"

using json = nlohmann::json;

bool parse_argv(int argc, char* argv[],
                int& port, bool& secure, std::string& hostname, std::string& username) {
    // parse -p and -s options
    int option;
    while ((option = getopt(argc, argv, "p:s")) != -1) {
        switch (option) {
            case 'p':
                port = std::stoi(optarg);
                break;
            case 's':
                secure = true;
                if (port == DEFAULT_PORT) {
                    port = DEFAULT_PORT_TLS;
                }
                break;
            default:
                std::cerr << "Invalid option argument" << std::endl;
                return false;
        }
    }

    // parse hostname and username
    if (optind + 1 >= argc) {
        std::cerr << "Host name and user name are required" << std::endl;
        return false;
    }
    hostname = argv[optind];
    username = argv[optind + 1];
    return true;
}

void initialize_ssl() {
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_ssl_algorithms();
}

void clean_ssl() {
    ERR_free_strings();
    EVP_cleanup();
}

int connect_server(std::string& hostname, int port_number, bool tls, SSL_CTX **ctx, SSL **ssl) {
    int sockfd = -1, rc = -1;
    struct addrinfo hints, *addr_list, *ptr;

    char port_buf[6];  // port is a 16-bit int
    snprintf(port_buf, sizeof(port_buf), "%d", port_number);

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_ADDRCONFIG;
    if ((rc = getaddrinfo(hostname.c_str(), port_buf, &hints, &addr_list)) != 0) {
        std::cerr << "getaddrinfo error " << gai_strerror(rc) << std::endl;
        return -1;
    }

    // iterate over result list, connect to the first address
    for (ptr = addr_list; ptr; ptr = ptr->ai_next) {
        // create socket descriptor; try the next one if failed
        if ((sockfd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) < 0) {
            continue;
        }

        // connect to the server
        if (connect(sockfd, ptr->ai_addr, ptr->ai_addrlen) != -1) {
            break;
        }
        // connection failed, try another
        close(sockfd);
    }

    // clean up data structure
    freeaddrinfo(addr_list);

    // if all connection failed
    if (!ptr) {
        return -1;
    }
    // TLS encrypted connection
    if (tls) {
        const SSL_METHOD *method = TLS_client_method();
        *ctx = SSL_CTX_new(method);
        if (!*ctx) {
            ERR_print_errors_fp(stderr);
            close(sockfd);
            return -1;
        }
        if (!SSL_CTX_set_default_verify_paths(*ctx)) {
            ERR_print_errors_fp(stderr);
            SSL_CTX_free(*ctx);
            close(sockfd);
            return -1;
        }
        SSL_CTX_set_verify(*ctx, SSL_VERIFY_PEER, NULL);
        *ssl = SSL_new(*ctx);
        SSL_set_fd(*ssl, sockfd);
        if (SSL_connect(*ssl) <= 0) {
            ERR_print_errors_fp(stderr);
            SSL_free(*ssl);
            SSL_CTX_free(*ctx);
            close(sockfd);
            return -1;
        }
    }
    return sockfd;
}

std::string play_game(int sockfd, const std::string& username, bool tls, SSL* ssl) {
    std::vector<std::string> words = read_from_file();
    std::set<char> nonexist;
    std::map<char, std::vector<int>> correct;
    std::map<char, std::vector<int>> wrong_positions;

    std::string game_id = start_game(sockfd, username, tls, ssl);
    bool game_over = false;
    int round = 0;
    std::string secret_flag;

    while (!game_over) {
        std::string guess = FIRST_GUESS;
        if (round != 0) {
            guess = choose_word(words, correct, nonexist, wrong_positions);
        }

        json guess_msg;
        guess_msg["type"] = "guess";
        guess_msg["id"] = game_id;
        guess_msg["word"] = guess;
        std::string sending = guess_msg.dump() + '\n';
        send_message(sockfd, sending, tls, ssl);
        round += 1;

        std::string received = receive_message(sockfd, tls, ssl);
        json retry = json::parse(received);
        if (retry["type"] == "bye" && retry["id"] == game_id) {
            game_over = true;
            secret_flag = retry["flag"];
        } else if (retry["type"] == "retry" && retry["id"] == game_id) {
            std::array<int, LEN> result{};
            const auto& prev = retry["guesses"];
            auto latest = prev.back();
            if (latest["word"] != guess) {
                for (const auto& trial : prev) {
                    if (trial.contains("word") && trial["word"] == guess) {
                        latest = trial;
                    }
                }
            }
            if (latest.contains("marks") && latest["marks"].is_array()) {
                result = latest["marks"];
                handle_marks(guess, result, correct, nonexist, wrong_positions);
            }
        } else if (retry["type"] == "error") {
            std::cerr << retry["message"] << std::endl;
            exit(1);
        } else {
            std::cerr << "unknown error: " << received << std::endl;
            exit(1);
        }
    }
    return secret_flag;
}

std::vector<std::string> read_from_file() {
    std::ifstream word_file(FILE_NAME);
    if (!word_file) {
        std::cerr << "Failed to read from word list file" << std::endl;
        exit(1);
    }

    std::vector<std::string> word_list;
    std::string word;
    while (std::getline(word_file, word)) {
        word.erase(std::remove(word.begin(), word.end(), '\r' ), word.end());
        word.erase(std::remove(word.begin(), word.end(), '\n' ), word.end());
        word_list.push_back(word);
    }
    word_file.close();
    return word_list;
}

void send_message(int sockfd, const std::string& message, bool tls, SSL *ssl) {
    ssize_t bytes_sent;
    if (tls) {
        bytes_sent = SSL_write(ssl, message.c_str(), message.length());
    } else {
        bytes_sent = send(sockfd, message.c_str(), message.length(), 0);
    }
    if (bytes_sent < 0) {
        std::cerr << "failed to send hello message" << std::endl;
        exit(1);
    }
}

std::string receive_message(int sockfd, bool tls, SSL *ssl) {
    char buffer[2048];
    std::string message;
    ssize_t bytes_received;

    while (true) {
        if (tls) {
            bytes_received = SSL_read(ssl, buffer, sizeof(buffer));
        } else {
            bytes_received = recv(sockfd, buffer, sizeof(buffer), 0);
        }
        if (bytes_received <= 0) {
            std::cerr << "failed to receive message from server" << std::endl;
            exit(1);
        }
        message.append(buffer, bytes_received);

        // no \n character appears inside the JSON data
        std::string::size_type newline = message.find('\n');
        if (newline != std::string::npos) {
            return message.substr(0, newline);
        }
    }
}

std::string start_game(int sockfd, const std::string& username, bool tls, SSL *ssl) {
    // send hello message to the server
    json hello_msg;
    hello_msg["type"] = "hello";
    hello_msg["northeastern_username"] = username;
    std::string sending = hello_msg.dump() + '\n';
    send_message(sockfd, sending, tls, ssl);

    // receive a start message from the server
    std::string received = receive_message(sockfd, tls, ssl);
    json start_msg = json::parse(received);
    if (start_msg["type"] != "start") {
        std::cerr << "start message error: " << start_msg.dump() << std::endl;
        exit(1);
    }
    return start_msg["id"];
}

std::string choose_word(std::vector<std::string>& all_words, std::map<char, std::vector<int>>& correct,
                        std::set<char>& nonexist, std::map<char, std::vector<int>>& position) {
    for (std::string& word : all_words) {
        bool flag = true;

        // check no letter is in the non-exist set
        for (char c : word) {
            if (nonexist.find(c) != nonexist.end()) {
                flag = false;
                break;
            }
        }
        if (!flag) {
            continue;
        }

        // check letters in the correct position
        for (const auto& [key, indices] : correct) {
            for (const int index : indices) {
                if (index < 0 || index >= LEN || word[index] != key) {
                    flag = false;
                    break;
                }
            }
        }
        if (!flag) {
            continue;
        }

        // check other letter exists but not in any known wrong positions
        for (const auto& [key, wrong_positions] : position) {
            if (word.find(key) == std::string::npos) {
                flag = false;
                break;
            }
            for (int pos : wrong_positions) {
                if (pos >= 0 && pos < LEN && word[pos] == key) {
                    flag = false;
                    break;
                }
            }
            if (!flag) {
                break;
            }
        }
        if (!flag) {
            continue;
        }

        // valid word option if its made this far
        return word;
    }
    return "";
}

void handle_marks(std::string& word, std::array<int, LEN>& marks, std::map<char, std::vector<int>>& correct,
                  std::set<char>& nonexist, std::map<char, std::vector<int>>& position) {
    // handle correct characters first
    for (int i = 0; i < LEN; i++) {
        char c = word[i];
        if (marks[i] == 2) {
            correct[c].push_back(i);
        }
    }
    for (int i = 0; i < LEN; i++) {
        char c = word[i];
        int mark = marks[i];
        if (mark == 0 && correct.find(c) == correct.end()) {
            nonexist.insert(c);
        } else if (mark == 0 && correct.find(c) != correct.end()) {
            position[c].push_back(i);
        } else if (mark == 1) {
            position[c].push_back(i);
        }
    }
}

int main(int argc, char* argv[]) {
    // parse command line arguments
    std::string hostname;
    std::string username;
    int port = DEFAULT_PORT;
    bool secure = false;
    if (!parse_argv(argc, argv, port, secure, hostname, username)) {
        std::cerr << "Usage: ./client <-p port> <-s> <hostname> <Northeastern-username>" << std::endl;
        exit(1);
    }

    SSL_CTX* ctx = nullptr;
    SSL* ssl = nullptr;
    if (secure) {
        initialize_ssl();
    }

    // connect to the server
    int socketfd;
    if ((socketfd = connect_server(hostname, port, secure, &ctx, &ssl)) < 0) {
        std::cerr << "Failed to connect to " << hostname << std::endl;
        if (secure) {
            clean_ssl();
        }
        exit(1);
    }

    // play wordle game and print the secret flag if successful
    std::string secret_flag = play_game(socketfd, username, secure, ssl);
    std::cout << secret_flag << std::endl;

    // close connection
    if (secure) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        clean_ssl();
    }
    close(socketfd);
    return 0;
}
