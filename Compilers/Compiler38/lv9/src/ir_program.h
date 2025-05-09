#include <iostream>
#include <string>
#include <unordered_map>
#include <cassert>
#include <vector>
#include <stack>

class ir_block{
public:
    static int block_num;
    std::string name;
    std::vector<std::string> stmts;
    ir_block(){name = "%Block" + std::to_string(block_num++);}
    void push_stmt(std::string code) {
        stmts.push_back(code);
    }
    std::string dump() {
        std::string code = "  " + name + ":\n";
        for (auto stmt : stmts)
            code += "    " + stmt;
        return code;
    }
};

int ir_block::block_num = 0;

class ir_function{
public:
    std::string name;
    std::string input_type;
    std::string ret_type;
    std::vector<ir_block*> blocks;
    ir_block *cur_block;
    ir_function(std::string name, std::string input_type, std::string ret_type):name(name), input_type(input_type), ret_type(ret_type){
        push_block();
    };
    ~ir_function(){
        for(auto& block: blocks){
            delete block;
        }
    }
    std::string push_block() {
        cur_block = new ir_block();
        blocks.push_back(cur_block);
        return cur_block->name;
    }
    std::string dump() {
        std::string code;
        if(ret_type.empty())
            code = "fun @" + name + "(" + input_type + ")\n{\n";
        else
            code = "fun @" + name + "(" + input_type + "):" + ret_type + "\n{\n";
        for (auto block : blocks)
            if(!block->stmts.empty()) code += block->dump();
        return code + "}\n";
    }
};

class ir_program{
private:
    ir_program(){}
    ir_program(const ir_program&) = delete;
    ir_program& operator=(const ir_program&) = delete;
public:
    std::vector<ir_function*> funcs;
    ir_function *cur_func = nullptr; 
    std::vector<std::string> global_stmts;
    static ir_program& getInstance() {
        // 单例模式
        static ir_program instance;
        return instance;
    }
    ~ir_program() {
        for(auto& func: funcs){
            delete func;
        }
    }
    void push_function(std::string name, std::string input, std::string output) {
        cur_func = new ir_function(name, input, output);
        funcs.push_back(cur_func);
    }
    void push_global_stmt(std::string code) {
        global_stmts.push_back(code);
    }

    std::string dump()
    {
        std::string code = "";
        code += "decl @getint(): i32\n";
        code += "decl @getch(): i32\n";
        code += "decl @getarray(*i32): i32\n";
        code += "decl @putint(i32)\n";
        code += "decl @putch(i32)\n";
        code += "decl @putarray(i32, *i32)\n";
        code += "decl @starttime()\n";
        code += "decl @stoptime()\n\n";
        for (auto gl_stmt : global_stmts)
            code += gl_stmt;
        for (auto func : funcs)
            code += func->dump();
        return code;
    }
    
};