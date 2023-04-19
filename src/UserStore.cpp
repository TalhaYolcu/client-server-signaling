#include <unordered_map>
#include <mutex>
#include "User.cpp"
#include <iostream>
#include <thread>

class UserStore {
private:
    std::unordered_map<std::string, User> users;
    mutable std::mutex mtx;

public:
    // Add a user to the store
    void addUser(const User& user) {
        const std::lock_guard<std::mutex> lock(mtx);
        users[user.getUsername()].setUsername(user.getUsername());
        users[user.getUsername()].setPassword(user.getPassword());
        users[user.getUsername()].setIslogin(user.getIslogin());
        users[user.getUsername()].setSocketFd(user.getSocketFd());

    }

    // Get a user from the store
    bool loginUser(const std::string& username,const std::string& password){
        std::lock_guard<std::mutex> lock(mtx);
        auto it = users.find(username);
        if (it != users.end()) {
            if(users[username].getPassword()==password) {
                users[username].setIslogin(true);
                return true;
            }
            return false;

        }
        // Handle error case where user is not found
        return false;
    }
    void logOutUser(const std::string& username) {
        users[username].setIslogin(false);
    }

    // Check if a user exists in the store
    bool hasUser(const std::string& username) {
        std::lock_guard<std::mutex> lock(mtx);
        return users.count(username) > 0;
    }

    bool isLogin(const std::string& username)  {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = users.find(username);
        if (it != users.end()) {
            return users[username].getIslogin();
        }
        // Handle error case where user is not found
        return false;
    }
    // Get a user from the store
    User getUser(const std::string& username) const {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = users.find(username);
        if (it != users.end()) {
            return it->second;
        }
        // Handle error case where user is not found
        throw std::runtime_error("User not found");
    }    
};