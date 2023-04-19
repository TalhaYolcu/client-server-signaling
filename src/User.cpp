#ifndef __USER
#define __USER

#include <iostream>



class User {
public:
    User(const std::string& username, const std::string& password, bool islogin, int socket_fd)
        : username_(username), password_(password), islogin_(islogin), socket_fd_(socket_fd) {}

    // Copy constructor
    User(const User& other)
        : username_(other.username_), password_(other.password_), islogin_(other.islogin_), socket_fd_(other.socket_fd_) {}

    User() {}

    // Getter and setter methods
    std::string getUsername() const { return username_; }
    void setUsername(const std::string& username) { username_ = username; }

    std::string getPassword() const { return password_; }
    void setPassword(const std::string& password) { password_ = password; }

    bool getIslogin() const { return islogin_; }
    void setIslogin(bool islogin) { islogin_ = islogin; }

    int getSocketFd() const { return socket_fd_; }
    void setSocketFd(int socket_fd) { socket_fd_ = socket_fd; }

private:
    std::string username_;
    std::string password_;
    bool islogin_;
    int socket_fd_;
};

#endif