#pragma once
#include <fstream>
#include <string>
#include <cassert>
#include <cstdlib>
#include <stdexcept>
#include <filesystem>
using namespace std;

// 刷新文档path
inline void refresh_file(const string& path) {
    // 以写入模式打开文件，清空文件内容
    std::ofstream outFile(path); 
    if (!outFile.is_open()) {
        throw runtime_error("无法创建文件: " + path);
    }
    // 文件已创建或清空，关闭文件
    outFile.close();
}

// 将content_str的内容以追加的形式储存到dest_str（内存中的字符串/输出文档）中
inline void save_to(string& dest_str, const string& content_str, const string save_mode) {
    if (save_mode == "string"){
        dest_str += content_str;
    }else if (save_mode == "output_file"){
       ofstream outFile(dest_str, ios::app);
        if (outFile.is_open()) {
            outFile << content_str;
            outFile.close();
        } else {
            throw runtime_error("无法打开文件: " + dest_str);
        } 
    }else{
        cout << "Unknown mode!" << endl;
        assert(false);
    }
}

inline void print_to_file(const string& dest_str, const string& content_str){
    string dest_str_2 = dest_str;
    save_to(dest_str_2, content_str, "output_file");
}

inline string forward_count(string father_name, int count_num){
    int num = stoi(father_name.substr(1)) - count_num;
    return "%" + to_string(num);
}

inline bool is_special_func_name(string func_name){
    return func_name == "main" || func_name == "getint" || func_name == "getch" || func_name == "getarray" || func_name == "putint" || func_name == "putch" || func_name == "putarray" || func_name == "starttime" || func_name == "stoptime";
}

inline void execute_command(const string& command) {
    int result = system(command.c_str());
    if (result != 0) {
        cerr << "Error: Command execution failed with code " << result << endl;
    }
}

inline string get_file_extension(const string& filename) {
    size_t dotPosition = filename.find_last_of('.');
    if (dotPosition != std::string::npos) {
        return filename.substr(dotPosition + 1); // 提取点后面的部分
    }
    return ""; // 如果没有找到点，返回空字符串
}

extern string global_dest_str;
extern string global_save_mode;

inline void save_string(const string& content_str) {
    save_to(global_dest_str, content_str, global_save_mode);
}

extern int getint_overridden;
extern int getch_overridden;
extern int getarray_overridden;
extern int putint_overridden;
extern int putch_overridden;
extern int putarray_overridden;
extern int starttime_overridden;
extern int stoptime_overridden;

inline int is_library_function(string func_name) {
    if (func_name == "getint" && !getint_overridden) {
        return 1;
    }else if (func_name == "getch" && !getch_overridden) {
        return 1;
    }else if (func_name == "getarray" && !getarray_overridden) {
        return 1;
    }else if (func_name == "putint" && !putint_overridden) {
        return 1;
    }else if (func_name == "putch" && !putch_overridden) {
        return 1;
    }else if (func_name == "putarray" && !putarray_overridden) {
        return 1;
    }else if (func_name == "starttime" && !starttime_overridden) {
        return 1;
    }else if (func_name == "stoptime" && !stoptime_overridden) {
        return 1;
    }else{
        return 0;
    }
}

inline void delete_file(const string& path) {
    try {
        if (filesystem::exists(path) && filesystem::is_regular_file(path)) {
            filesystem::remove(path);
            return;
        } else {
            cerr << "Error: " << path << " does not exist or is not a regular file." << endl;
            return;
        }
    } catch (const filesystem::filesystem_error& e) {
        cerr << "Filesystem error: " << e.what() << endl;
        return;
    } catch (const exception& e) {
        cerr << "Standard exception: " << e.what() << endl;
        return;
    }
}