#include <iostream>
#include <string>
#include <unordered_map>
#include <cassert>
#include <stdexcept>


class FuncFrame {
private:
    std::unordered_map<std::string, int> frameByName;
    std::unordered_map<long long, int> frameByAddr;
    int totalSize;
    int init_offset(int size = 4) {
        totalSize += size;
        return totalSize - size - 4; // 4 bytes for return address
    }

public:
    FuncFrame() {totalSize = 4;} // 4 bytes for return address
    void addVariable(const std::string& name, int size=4) { frameByName[name] = init_offset(size);}

    void addVariable(long long addr, int size=4) { frameByAddr[addr] = init_offset(size);}

    int get_offset(const std::string& varName) const {
        auto it = frameByName.find(varName);
        if (it != frameByName.end()) return it->second;
        cout << "Variable not found: " << varName << endl;
        return -1;
    }

    int get_offset(long long varAddr) const {
        auto it = frameByAddr.find(varAddr);
        if (it != frameByAddr.end()) return it->second;
        cout << "Variable not found: " << varAddr << endl;
        return -1;
    }

    int align(int alignment = 16) {
        if (totalSize % alignment != 0) {
            totalSize = ((totalSize + alignment - 1) / alignment) * alignment;
        }
        return totalSize;
    }

    std::string show_frame() {
        std::string res = "";
        for (auto it = frameByName.begin(); it != frameByName.end(); ++it) {
            res += it->first + " " + std::to_string(it->second) + "\n";
        }
        for (auto it = frameByAddr.begin(); it != frameByAddr.end(); ++it) {
            res += std::to_string(it->first) + " " + std::to_string(it->second) + "\n";
        }
        return res;
    }
};

class StackManager {
private:
    std::unordered_map<std::string, FuncFrame> frame_map;
    StackManager(const StackManager&) = delete;
    StackManager& operator=(const StackManager&) = delete;
    StackManager(){};
public:
    std::string cur_func;

    static StackManager& getInstance() {
        // 单例模式
        static StackManager instance;
        return instance;
    }

    void new_func(const std::string& funcName) {
        cur_func = funcName;
        frame_map[cur_func] = FuncFrame();
    }

    void push_in(const std::string& varName, int size = 4) {
        if (frame_map.find(cur_func) == frame_map.end()) {
            frame_map[cur_func] = FuncFrame();
        }
        frame_map[cur_func].addVariable(varName, size);
    }

    void push_in(long long varAddr, int size = 4) {
        if (frame_map.find(cur_func) == frame_map.end()) {
            frame_map[cur_func] = FuncFrame();
        }
        frame_map[cur_func].addVariable(varAddr, size);
    }

    int get_offset(const std::string& varName) const {
        auto it = frame_map.find(cur_func);
        if (it != frame_map.end()) return it->second.get_offset(varName);
        throw std::runtime_error("Function not found");
    }

    int get_offset(long long varAddr) const {
        auto it = frame_map.find(cur_func);
        if (it != frame_map.end()) return it->second.get_offset(varAddr);
        throw std::runtime_error("Function not found");
    }

    int align() {
        return frame_map[cur_func].align(16);
    }

    std::string show_frame() {
        return frame_map[cur_func].show_frame();
    }
};