#include "ThreadSafeMap.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>



using namespace std;

#ifndef _THREADSAFE_CPP
#define _THREADSAFE_CPP
template <typename K, typename V>
void ThreadSafeMap<K, V>::add(const K& key, const V& value) {
    std::unique_lock<std::mutex> lock(mutex_);
    map_[key] = value;
}

template <typename K, typename V>
void ThreadSafeMap<K, V>::edit(const K& key, const V& value) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (map_.count(key)) {
        map_[key] = value;
    }
}

template <typename K, typename V>
void ThreadSafeMap<K, V>::remove(const K& key) {
    std::unique_lock<std::mutex> lock(mutex_);
    map_.erase(key);
}

template <typename K, typename V>
V ThreadSafeMap<K, V>::get(const K& key) const {
    std::unique_lock<std::mutex> lock(mutex_);
    auto iter = map_.find(key);
    if (iter != map_.end()) {
        return iter->second;
    } else {
        return V();
    }
}

template <typename K, typename V>
bool ThreadSafeMap<K, V>::contains(const K& key) const {
    std::unique_lock<std::mutex> lock(mutex_);
    auto iter = map_.find(key);
    return (iter != map_.end());
}

template<typename K, typename V>
void ThreadSafeMap<K, V>::writeToFile(const std::string& filename, const char delimiter) const {
    std::unique_lock<std::mutex> lock(mutex_);
    std::ofstream outfile(filename);
    for (const auto& entry : map_) {
        outfile << entry.first << delimiter << entry.second << std::endl;
    }
}

template<typename K, typename V>
void ThreadSafeMap<K, V>::readFromFile(const string& filename, char delimiter) {
    std::ifstream infile(filename);
    if (infile.is_open()) {
        string line;
        while (std::getline(infile, line)) {
            std::istringstream iss(line);
            K key;
            V value;
            if (std::getline(iss, key, delimiter) && std::getline(iss, value)) {
                add(key, value);
            }
        }
        infile.close();
    }
}
template <typename K, typename V>
void ThreadSafeMap<K, V>::print() const {
    std::unique_lock<std::mutex> lock(mutex_);
    for (const auto& pair : map_) {
        std::cout << "Key: " << pair.first << " Value: " << pair.second << std::endl;
    }
}




#endif