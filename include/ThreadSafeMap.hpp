#ifndef THREADSAFEMAP_HPP
#define THREADSAFEMAP_HPP

#include <map>
#include <mutex>
#include <fstream>
#include <iostream>
#include <string>

template <typename K, typename V>
class ThreadSafeMap {
public:
    void add(const K& key, const V& value);
    void edit(const K& key, const V& value);
    void remove(const K& key);
    V get(const K& key) const;
    bool contains(const K& key) const;
    void writeToFile(const std::string& filename, const char delimiter) const;
    void readFromFile(const std::string& filename, const char delimiter);
    void print() const;    
private:
    std::map<K, V> map_;
    mutable std::mutex mutex_;
};


#endif // THREADSAFEMAP_HPP
