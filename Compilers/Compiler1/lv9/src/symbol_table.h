#pragma once
#include <unordered_map>
#include <vector>
#include <string>
#include <stdexcept>
#include <iomanip>
using namespace std;

class SymbolTable {
    public:
        // 初始化符号表
        void initialize() {
            symbolMap.clear(); 
            indexes.clear();     
        }

        // 向符号表中插入符号，并赋相应的值
        // 对常量，值就是它的大小
        // 对变量，值是它当前所处的koopa register
        // 如果变量没有初始化，那么当前所处的koopa register为-1
        int insert_constant(const string& name, int value) {
            return insert(name, value, true); 
        }
        int insert_variable(const string& name, int value) {
            return insert(name, value, false); 
        }

        // 确认符号是否存在于符号表中
        bool exists(const string& name) const {
            return symbolMap.find(name) != symbolMap.end();
        }

        // 确认符号是否是常量
        bool is_constant(const string& name) const {
            auto it = symbolMap.find(name);
            if (it == symbolMap.end()) {
                throw runtime_error("Symbol '" + name + "' not defined.");
            }
            return it->second.is_constant;
        }

        // 获取符号对应的的index
        int get_index(const string& name) const {
            auto it = symbolMap.find(name);
            if (it == symbolMap.end()) {
                throw runtime_error("Symbol '" + name + "' not defined.");
            }
            return it->second.index;
        }

        //  获取index对应的符号
        string get_symbol(int index) const {
            if (index < 0 || index >= indexes.size()) {
                throw out_of_range("Index out of range.");
            }
            return indexes[index];
        }

        // 获取符号对应的value
        int get_value(const string& name) const {
            auto it = symbolMap.find(name);
            if (it == symbolMap.end()) {
                throw runtime_error("Symbol '" + name + "' not defined.");
            }
            return it->second.value;
        }

        // 获取index对应的value
        int get_value(int index) const {
            if (index < 0 || index >= indexes.size()) {
                throw out_of_range("Index out of range.");
            }
            return symbolMap.at(indexes[index]).value;
        }

        // 获取符号表大小
        int get_size() const {
            return static_cast<int>(symbolMap.size()); 
        }

        // 打印符号表
        void print() const {
            cout << left << setw(10) << "Index" 
                    << setw(10) << "Ident" 
                    << setw(10) << "Value" << endl;

            for (const auto& name : indexes) {
                const auto& info = symbolMap.at(name);
                cout << left << setw(10) << info.index 
                        << setw(10) << name 
                        << setw(10) << (info.is_constant ? to_string(info.value)
                                                                : "%" + to_string(info.value)) 
                        << endl;
            }
        }

        // 更新符号表中变量的值
        void update_variable(const std::string& name, int newValue) {
            auto it = symbolMap.find(name);
            
            // name必须在符号表中且为变量
            if (it == symbolMap.end()) {
                throw std::runtime_error("Symbol '" + name + "' not defined.");
            }
            if (it->second.is_constant) {
                throw std::runtime_error("Cannot update constant '" + name + "'.");
            }

            it->second.value = newValue;
        }

        int insert(const string& name, int value, bool is_constant) {
            if (symbolMap.find(name) != symbolMap.end()) {
                throw runtime_error("Symbol '" + name + "' already defined.");
            }
            symbolMap[name] = {value, static_cast<int>(indexes.size()), is_constant};
            indexes.push_back(name); 
            return indexes.size() - 1; 
        }

        struct SymbolInfo {
            int value; 
            int index; 
            bool is_constant;
        };

        unordered_map<string, SymbolInfo> symbolMap; 
        vector<string> indexes; 
};

class SymbolTableList {
public:
    // 初始化，确保列表为空
    void initialize() {
        tables.clear();
        currentIndex = -1; // 设置当前指标为 -1
        if (verbose) print(); 
    }

    // 拓展
    void expand() {
        SymbolTable newTable;
        newTable.initialize();
        tables.push_back(newTable);
        if (currentIndex < static_cast<int>(parallel_block_numbers.size()) - 1) {
            currentIndex += 1;
            parallel_block_numbers[currentIndex] += 1;
        }else{
            parallel_block_numbers.push_back(0);
            currentIndex++;
        }
        if (verbose) print(); 
    }

    // 收缩，删除当前符号表并将当前指标减一
    void shrink() {
        if (currentIndex < 0) {
            throw runtime_error("No symbol tables to shrink.");
        }
        tables.pop_back(); // 删除当前符号表
        currentIndex--; // 当前指标减一
        if (verbose) print(); 
    }

    // 向最新的符号表插入常量
    int insert_constant(const string& name, int value) {
        return get_latest_table().insert_constant(name, value);
    }

    // 向最新的符号表插入变量
    int insert_variable(const string& name, int value) {
        return get_latest_table().insert_variable(name, value);
    }

    // 确认符号是否在符号表列中
    bool exists(const string& name) const {
        for (int i = currentIndex; i >= 0; --i) {
            if (tables[i].exists(name)) {
                return true;
            }
        }
        return false;
    }

    string get_stack_position(string name) const {
        for (int i = currentIndex; i >= 0; --i) {
            if (tables[i].exists(name)) {
                /* 修复这个bug时，我怀疑测试用例不是静态的 */
                // if (!i) return "@" + name;
                if (!i) return "@" + name + "_global";
                string res = "@"+name+"_lv"+to_string(i)+"_no"+to_string(parallel_block_numbers[i]+1);
                return res;
            }
        }
        return "InvalidStackPositionError";
    }

    // 确认符号是否是常量
    bool is_constant(const string& name) const {
        for (int i = currentIndex; i >= 0; --i) {
            if (tables[i].exists(name)) {
                return tables[i].is_constant(name);
            }
        }
        throw runtime_error("Symbol '" + name + "' not defined in any scope.");
    }

    // 获取符号的值
    int get_value(const string& name) const {
        
        for (int i = currentIndex; i >= 0; --i) {
            if (tables[i].exists(name)) {
                return tables[i].get_value(name);
            }
        }
        throw runtime_error("Symbol '" + name + "' not defined in any scope.");
    }

    // 打印符号表列
    void print() const {
        cout << " ------------------------------------- \n";
        if (currentIndex == -1) {
            cout << "| ====>                               |" << endl << "|                                     |" <<endl;
        }
        for (size_t i = 0; i < tables.size(); i++) {
            if (i) {
                cout << "|                                     |" << endl;
            }
            cout << "|       -------------------------     |\n";
            if (i == currentIndex){
                cout << "| ====> ";
            }else{
                cout << "|       ";
            }
            cout << left << setw(10) << "Index" 
                    << setw(10) << "Ident" 
                    << setw(10) << "Value" << "|" << endl;
            cout << "|       -------------------------     |\n";

            for (const auto& name : tables[i].indexes) {
                const auto& info = tables[i].symbolMap.at(name);
                cout << "|       ";
                cout << left << setw(10) << info.index 
                        << setw(10) << name 
                        << setw(10) << (info.is_constant ? to_string(info.value)
                                                                : "%" + to_string(info.value)) ;
                cout << "|" << endl;
            }
        }
        cout << " ------------------------------------- \n";
        cout << endl << endl;
    }

    // 更新符号表中变量的值
    void update_variable(const string& name, int newValue) {
        for (int i = currentIndex; i >= 0; --i) {
            if (tables[i].exists(name)) {
                tables[i].update_variable(name, newValue);
                if (verbose) print(); 
                return;
            }
        }
        throw runtime_error("Symbol '" + name + "' not defined in any scope.");
    }

    vector<SymbolTable> tables;
    vector<int> parallel_block_numbers;
    int currentIndex = -1; // 当前指标，初始化为 -1
    bool verbose;

    // 获取最新的符号表
    SymbolTable& get_latest_table() {
        if (currentIndex < 0) {
            throw runtime_error("No symbol tables available.");
        }
        return tables[currentIndex];
    }
};

extern SymbolTableList symbol_table_list;