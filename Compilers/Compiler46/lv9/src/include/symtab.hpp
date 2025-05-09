#ifndef SYMTAB_H
#define SYMTAB_H

#include <string>
#include <unordered_map>
#include <utility>
#include <memory>
#include <vector>
#include <deque>
#include <cassert>

// #define DEBUG
#ifdef DEBUG
#define dbg_printf(...) fprintf(stderr, __VA_ARGS__)
#else
#define dbg_printf(...)
#endif

/**
 * 符号表symbol table.
 * 符号表符号的定义与ast中的Koopa IR符号不同.
 * Koopa IR符号包括源程序中的常量对应的符号（以数字表示）、
 * 变量对应的符号（以@[变量符号]_[序号]表示），
 * 以及为表达式生成的、以"%"开头的符号.
 * 符号表符号只包括源程序中定义的变量和常量对应的符号.
 */

enum class SymbolTag
{
    CONST,
    VAR,
    ARRAY,
    PTR,
    VOID, // 无返回值的函数
    INT   // 返回类型为int的函数
};

class SymbolInfo
{
public:
    SymbolTag tag;
    std::string symbol;    // koopa IR符号
    std::vector<int> dims; // 数组或数组指针的维数

    SymbolInfo(const SymbolTag tag, const std::string &symbol, const std::vector<int> &dims) : tag(tag), symbol(symbol), dims(dims)
    {
    }
};

/**
 * @brief 一个作用域有一个作用域符号表ScopeSymbolTable
 */
class ScopeSymbolTable
{
public:
    std::unordered_map<std::string, std::shared_ptr<SymbolInfo>> scope_tab;

    /**
     * @brief 向符号表中添加一个符号, 同时记录这个符号的值
     *
     * @param ident     SysY标识符（变量名）
     * @param tag       符号类型
     * @param symbol    Koopa IR符号
     */
    void insert(const std::string &ident,
                const SymbolTag tag,
                const std::string &symbol,
                const std::vector<int> &dims = {})
    {
        assert(!contains(ident));
        scope_tab.insert(std::make_pair(
            ident,
            std::make_shared<SymbolInfo>(tag, symbol, dims)));
    }

    /**
     * @brief 给定一个符号, 查询符号表中是否存在这个符号的定义
     *
     * @param ident SysY标识符
     * @return true
     * @return false
     */
    bool contains(const std::string &ident) const
    {
        return scope_tab.find(ident) != scope_tab.end();
    }

    /**
     * @brief 给定一个符号表中已经存在的符号, 返回这个符号对应的SymbolInfo
     *
     * @param ident SysY标识符
     * @return shared_ptr<SymbolInfo>
     */
    std::shared_ptr<SymbolInfo> operator[](const std::string &ident) const
    {
        assert(contains(ident));
        return scope_tab.at(ident);
    }
};

/**
 * @brief 不同作用域的ScopeSymbolTable栈式进出
 */
class SymbolTable
{
public:
    std::deque<std::unique_ptr<ScopeSymbolTable>> table;

    /**
     * @brief 插入全局符号表
     */
    SymbolTable()
    {
        table.emplace_back(std::make_unique<ScopeSymbolTable>());
        table.back()->insert("getint", SymbolTag::INT, "@getint");
        table.back()->insert("getch", SymbolTag::INT, "@getch");
        table.back()->insert("getarray", SymbolTag::INT, "@getarray");
        table.back()->insert("putint", SymbolTag::VOID, "@putint");
        table.back()->insert("putch", SymbolTag::VOID, "@putch");
        table.back()->insert("putarray", SymbolTag::VOID, "@putarray");
        table.back()->insert("starttime", SymbolTag::VOID, "@starttime");
        table.back()->insert("stoptime", SymbolTag::VOID, "@stoptime");
    }

    /**
     * @brief 插入一个作用域符号表
     */
    void push()
    {
        table.emplace_back(std::make_unique<ScopeSymbolTable>());
    }

    /**
     * @brief 退出一个作用域符号表
     */
    void pop()
    {
        table.pop_back();
    }

    /**
     * @brief 向符号表中添加一个符号, 同时记录这个符号的值
     *
     * @param ident     SysY标识符
     * @param tag       符号类型
     * @param symbol    Koopa IR符号
     */
    void insert(const std::string &ident,
                const SymbolTag tag,
                const std::string &symbol,
                const std::vector<int> &dims = {})
    {
        assert(!(table.back()->contains(ident)));
        table.back()->insert(ident, tag, symbol, dims);
    }

    /**
     * @brief 给定一个符号, 查询符号表中是否存在这个符号的定义
     *
     * @param ident SysY标识符

     * @return true
     * @return false
     */
    bool contains(const std::string &ident) const
    {
        for (auto it = table.rbegin(); it != table.rend(); ++it)
        {
            if ((*it)->contains(ident))
            {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief 给定一个符号表中已经存在的符号, 返回这个符号对应的SymbolInfo
     *
     * @param ident SysY标识符
     * @return std::shared_ptr<SymbolInfo>
     */
    std::shared_ptr<SymbolInfo> operator[](const std::string &ident) const
    {
        for (auto it = table.rbegin(); it != table.rend(); ++it)
        {
            if ((*it)->contains(ident))
            {
                return (**it)[ident];
            }
        }
        assert(false);
    }

    /**
     * @brief 是否在全局作用域
     *
     * @return true
     * @return false
     */
    bool in_global_scope()
    {
        return table.size() == 1;
    }

    /**
     * @brief 在全局作用域中查找符号，用于查找函数符号
     *
     * 因为SysY语法规定局部变量可以与函数同名，使用普通查找时函数可能会被局部变量覆盖。
     * 一个简单的解决方案是，利用全局符号不能同名的规定，在全局作用域中查找函数符号
     *
     * @param ident SysY标识符
     * @return std::shared_ptr<SymbolInfo>
     */
    std::shared_ptr<SymbolInfo> find_in_global_scope(const std::string &ident) const
    {
        assert(table.front()->contains(ident));
        return table.front()->operator[](ident);
    }
};

#endif