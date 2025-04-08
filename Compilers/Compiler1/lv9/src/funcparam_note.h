#pragma once
#include <iostream>
#include <map>
#include <string>
#include <utility>
using namespace std;

class FuncparamNote {
public:
    void initialize() {
        params.clear();
    }

    void insert(const std::string& ident, int index, const std::string& btype) {
        auto key = std::make_pair(ident, index); 
        params[key] = btype;
    }

    std::string check(const std::string& ident, int index) {
        auto key = std::make_pair(ident, index);
        auto it = params.find(key);
        if (it != params.end()) {
            return it->second;
        } else {
            return "未找到参数类型";
        }
    }

private:
    std::map<std::pair<std::string, int>, std::string> params;
};

extern FuncparamNote funcparam_note;