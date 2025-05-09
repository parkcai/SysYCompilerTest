#include <iostream>
#include <string>
#include <unordered_map>
#include <cassert>
#include <vector>
#include <set>
#include <stack>

enum class SymbolType_CVP { CONSTANT, VARIABLE, PRARM };
enum class SymbolType_GL { GLOBAL, LOCAL };
enum class SymbolType_AN { ARRAY, NUMBER };

class LocalSymbolTable;

class Symbol {
public:
    std::string ident;
    LocalSymbolTable* location;
    SymbolType_CVP type_cv;
    SymbolType_AN type_an;
    SymbolType_GL type_gl;
    int value;
    std::vector<int> dims;
    std::vector<int> array_value;
    std::string ir_name;
    Symbol(std::string id, LocalSymbolTable *loc, SymbolType_CVP t1, SymbolType_AN t2, SymbolType_GL t3, int v);
    Symbol(std::string id, LocalSymbolTable *loc, SymbolType_CVP t1, SymbolType_AN t2, SymbolType_GL t3, vector<int> dims, vector<int> array_value);
};


class LocalSymbolTable {
public:
    const std::vector<int> key;
    std::unordered_map<std::string, Symbol*> symbolMap;
    LocalSymbolTable(std::vector<int> key): key(key) {}
    ~LocalSymbolTable() {
        for (auto& pair : symbolMap) {
            delete pair.second;
        }
    } 
    void insertArray(std::string ident, SymbolType_CVP type_cvp, vector<int> dims, vector<int> array_value=std::vector<int>()){
        symbolMap[ident] = new Symbol(ident, this, type_cvp, SymbolType_AN::ARRAY, key.empty() ? SymbolType_GL::GLOBAL : SymbolType_GL::LOCAL, dims, array_value);
    }
    void insertNumber(std::string ident, SymbolType_CVP type_cvp, int value=0){
        assert(!ident.empty());
        symbolMap[ident] = new Symbol(ident, this, type_cvp, SymbolType_AN::NUMBER, key.empty() ? SymbolType_GL::GLOBAL : SymbolType_GL::LOCAL, value);
    }
    Symbol* findSymbol(const std::string& ident) const {
        auto it = symbolMap.find(ident);
        if (it != symbolMap.end()) {
            return it->second;
        }
        return nullptr;
    }
    bool symbolExists(const std::string& ident) const {
        return symbolMap.find(ident) != symbolMap.end();
    }
};

Symbol::Symbol(std::string id, LocalSymbolTable *loc, SymbolType_CVP t1, SymbolType_AN t2, SymbolType_GL t3, vector<int> dims, vector<int> array_value) 
: ident(id), location(loc), type_cv(t1), type_an(t2), type_gl(t3), dims(dims), array_value(array_value)
{
    assert(t2==SymbolType_AN::ARRAY);
    ir_name = "@_";
    for (auto i : location->key) {
        ir_name.append(std::to_string(i));
        ir_name.append("_");
    }
    ir_name.append(id+"_");
}
Symbol::Symbol(std::string id, LocalSymbolTable *loc, SymbolType_CVP t1, SymbolType_AN t2, SymbolType_GL t3, int v) 
: ident(id), location(loc), type_cv(t1), type_an(t2), type_gl(t3), value(v) 
{
    assert(t2==SymbolType_AN::NUMBER);
    ir_name = "@_";
    for (auto i : location->key) {
        ir_name.append(std::to_string(i));
        ir_name.append("_");
    }
    ir_name.append(id+"_");
    if (type_cv==SymbolType_CVP::PRARM) ir_name.append("P");
}


class SymbolTable {
private:
    SymbolTable() {
        key = vector<int>();
        key_record = vector<int>();
        symMap[key] = new LocalSymbolTable(key);
    }
    SymbolTable(const SymbolTable&) = delete;
    SymbolTable& operator=(const SymbolTable&) = delete;
    
    struct VectorHash {
    template <typename T>
    size_t operator ()(const std::vector<T>& vec) const {
        size_t hash = 0;
        for (const T& elem : vec) {
            hash ^= std::hash<T>()(elem) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
        return hash;
    }
    };

public:
    std::unordered_map<std::vector<int>, LocalSymbolTable*, VectorHash> symMap;
    std::set<std::string> void_func_name = {"putint", "putch", "putarray", "starttime", "stoptime"};
    bool still_in_block = false;
    vector<int> key_record;
    vector<int> key;
    static SymbolTable& getInstance() {
        // 单例模式
        static SymbolTable instance;
        return instance;
    }
    ~SymbolTable() {
        for (auto ppair : symMap) {
            delete ppair.second;
        }
    }
    void reset()
    {
        key.clear();
        key_record.clear();
        still_in_block = false;
    }
    void enterScope() {
        if (!still_in_block){
            if (key_record.size()>key.size())
            {
                int temp = key.size();
                key.push_back(++key_record[temp]);
            }else{
                key.push_back(0);
                key_record.push_back(0);
            }
        } else {
            still_in_block = false;
            return;
        }
        if (symMap.find(key) == symMap.end()) {
            symMap[key] = new LocalSymbolTable(key);
        }

    }
    void exitScope() {
        assert(!key.empty() && "No scope to exit!");
        while(key_record.size()>key.size()) key_record.pop_back();
        key.pop_back();
    }
    void dont_exitScope() {
        still_in_block = true;
    }
    void insertArray(std::string ident, SymbolType_CVP type_cvp, vector<int> dims, vector<int> array_value=std::vector<int>()){
        symMap[key]->insertArray(ident, type_cvp, dims, array_value);
    }
    void insertNumber(std::string ident, SymbolType_CVP type_cvp, int value=0){
        symMap[key]->insertNumber(ident, type_cvp, value);
    }
    Symbol* findSymbol(const std::string& ident) const {
        vector<int> temp_key = key;
        while(!temp_key.empty())
        {
            Symbol* result = symMap.find(temp_key)->second->findSymbol(ident);
            if(result) return result;
            temp_key.pop_back();
        }
        return symMap.find(temp_key)->second->findSymbol(ident);
    }
    bool symbolExistsInCurrentScope(const std::string& ident) const {
        return symMap.find(key)->second->symbolExists(ident);
    }
};

