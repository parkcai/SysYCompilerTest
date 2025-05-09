#ifndef AST_H
#define AST_H

#include <memory>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <stack>
#include <unordered_map>
#include <cstdlib>

#include "symtab.hpp"

// #define DEBUG
#ifdef DEBUG
#define dbg_printf(...) fprintf(stderr, __VA_ARGS__)
#else
#define dbg_printf(...)
#endif

// TODO 考虑把std::string symbol改成std::variant<int, std::string> symbol

/**
 * 特点：
 * 1. 实现简洁漂亮
 * 2. C++17标准，使用智能指针等保证内存安全
 */

/**
 * Koopa IR 符号(symbol)命名规则：
 * 维护递增计数器sym_cnt，符号命名为[@|%][变量符号]_[序号]
 * main函数和库函数除外，因为Koopa规范规定main函数的Koopa IR必须是main
 * 容易证明所有符号都不会重复
 */

/**
 * 目前采用如下策略：
 * 1. 在变量分类上，is_const只对常量成立
 * 2. 在表达式计算上，自下而上判断是否为常量，若是则计算，否则不计算。
 * 因为测试程序没有语义错误，所以一定能在常量定义时计算出值，
 * 在变量定义时，若右边表达式不包含变量，也能计算出值。
 * 3. 这是一种妥协，将SSA形式和稀疏条件常量传播留到最后再做。
 */

void decl_IR();

/**
 * @brief 按照官方文档的写法，所有成员变量均为public，不提供get和set方法
 */
class BaseAST
{
public:
    virtual ~BaseAST() = default;

    /**
     * @brief 按照官方文档的写法，输出AST的结构，基本没用，没有实现
     */
    virtual void Dump() const = 0;

    /**
     * @brief   语义分析，生成Koopa IR，维护符号表
     *
     * 1. 生成Koopa IR，即先以递归形式后序遍历AST树，调用成员AST的IR方法，
     * 然后生成本节点的Koopa IR。
     * 2. 把对应的Koopa IR符号插入符号表
     */
    virtual void IR() = 0;
};

class ExpBaseAST : public BaseAST
{
public:
    // AST对应的Koopa IR符号，若能求值，则为其值，若不能，则为符号
    std::string symbol;
    // 是否为常量
    bool is_const;
};

/**
 * @brief LVal          ::= IDENT {"[" Exp "]"};
 */
class LValAST : public ExpBaseAST
{
public:
    std::string ident;
    std::vector<std::unique_ptr<ExpBaseAST>> exps;
    std::string loc_sym;

    void Dump() const override;

    void IR() override;
};

/**
 * @brief CompUnit ::= [CompUnit] (Decl | FuncDef);
 */
class CompUnitAST : public BaseAST
{
public:
    std::vector<std::unique_ptr<BaseAST>> comp_units;

    void Dump() const override;

    void IR() override;
};

/**
 * @brief Decl          ::= ConstDecl | VarDecl;
 */
class DeclAST : public BaseAST
{
public:
    enum class Tag
    {
        CONST,
        VAR
    } tag;
    std::unique_ptr<BaseAST> const_decl;
    std::unique_ptr<BaseAST> var_decl;

    void Dump() const override;

    void IR() override;
};

/**
 * @brief ConstDecl     ::= "const" BType ConstDef {"," ConstDef} ";";
 *
 * BType         ::= "int";
 *
 */
class ConstDeclAST : public BaseAST
{
public:
    std::vector<std::unique_ptr<BaseAST>> const_defs;

    void Dump() const override;

    void IR() override;
};

/**
 * @brief ConstInitVal  ::= ConstExp | "{" [ConstInitVal {"," ConstInitVal}] "}";
 */
class ConstInitValAST : public ExpBaseAST
{
public:
    enum class Tag
    {
        EXP,
        VAL
    } tag;
    std::unique_ptr<ExpBaseAST> const_exp;
    std::vector<std::unique_ptr<ConstInitValAST>> const_init_vals;

    void Dump() const override;

    void IR() override;
};

// TODO parser填好const_exps和const_init_val树，要
// 1. 生成填好0的初始化列表
// 2. 由初始化列表生成Koopa IR
// 1.1. 确定对齐位置
// 1.2. 递归地填充

/**
 * @brief ConstDef      ::= IDENT {"[" ConstExp "]"} "=" ConstInitVal;
 */
class ConstDefAST : public BaseAST
{
public:
    std::string ident;
    std::vector<std::unique_ptr<ExpBaseAST>> const_exps;
    std::unique_ptr<ConstInitValAST> const_init_val;

    void Dump() const override;

    void IR() override;

    /**
     * @brief 进到待编译程序给出的初始化列表的一个大括号里，填充初始化列表
     *
     * @param init_vals         待编译程序给出的初始化列表的一个大括号对应的AST数组
     * @param full_init_vals    待填充的初始化列表
     * @param is_first          是否是第一个大括号
     * @return int              该大括号负责初始化的长度
     */
    int fill_init_vals(const std::vector<std::unique_ptr<ConstInitValAST>>
                           &init_vals,
                       std::vector<std::string> &full_init_vals, bool is_first = false);

    /**
     * @brief 当前已填充长度的对齐值，即当前大括号负责初始化的长度
     *
     * @param len       当前已填充长度
     * @return int      对齐值
     */
    int aligned_len(int len);

    /**
     * @brief 生成取数组指针和存初始值的Koopa IR
     *
     * @param full_init_vals    初始化列表
     * @param symbol            指针的Koopa IR符号
     * @param dim               当前维度
     */
    void get_ptr_store_val(const std::vector<std::string> &full_init_vals,
                           const std::string &symbol, int dim);

    /**
     * @brief 输出当前大括号的初始化列表
     *
     * @param full_init_vals    初始化列表
     * @param dim               当前维度
     */
    void print_aggr(const std::vector<std::string> &full_init_vals, int dim);
};

/**
 * @brief VarDecl       ::= BType VarDef {"," VarDef} ";";
 * BType                ::= "int";
 */
class VarDeclAST : public BaseAST
{
public:
    std::vector<std::unique_ptr<BaseAST>> var_defs;

    void Dump() const override;

    void IR() override;
};

/**
 * @brief InitVal       ::= Exp | "{" [InitVal {"," InitVal}] "}";
 */
class InitValAST : public ExpBaseAST
{
public:
    enum class Tag
    {
        EXP,
        VAL
    } tag;
    std::unique_ptr<ExpBaseAST> exp;
    std::vector<std::unique_ptr<InitValAST>> init_vals;

    void Dump() const override;

    void IR() override;
};

/**
 * @brief VarDef        ::= IDENT {"[" ConstExp "]"}
 *                      | IDENT {"[" ConstExp "]"} "=" InitVal;
 */
class VarDefAST : public BaseAST
{
public:
    std::string ident;
    std::vector<std::unique_ptr<ExpBaseAST>> const_exps;
    std::unique_ptr<InitValAST> init_val;

    void Dump() const override;

    void IR() override;

    /**
     * @brief 进到待编译程序给出的初始化列表的一个大括号里，填充初始化列表
     *
     * @param init_vals         待编译程序给出的初始化列表的一个大括号对应的AST数组
     * @param full_init_vals    待填充的初始化列表
     * @param is_first          是否是第一个大括号
     * @return int              该大括号负责初始化的长度
     */
    int fill_init_vals(const std::vector<std::unique_ptr<InitValAST>>
                           &init_vals,
                       std::vector<std::string> &full_init_vals, bool is_first = false);

    /**
     * @brief 当前已填充长度的对齐值，即当前大括号负责初始化的长度
     *
     * @param len       当前已填充长度
     * @return int      对齐值
     */
    int aligned_len(int len);

    /**
     * @brief 生成取数组指针和存初始值的Koopa IR
     *
     * @param full_init_vals    初始化列表
     * @param symbol            指针的Koopa IR符号
     * @param dim               当前维度
     */
    void get_ptr_store_val(const std::vector<std::string> &full_init_vals,
                           const std::string &symbol, int dim);

    /**
     * @brief 输出当前大括号的初始化列表
     *
     * @param full_init_vals    初始化列表
     * @param dim               当前维度
     */
    void print_aggr(const std::vector<std::string> &full_init_vals, int dim);
};

/**
 * @brief FuncType    ::= "void" | "int";
 */
class FuncTypeAST : public BaseAST
{
public:
    inline static const std::vector<std::string> type_ir{"", ": i32"};
    enum Type
    {
        VOID,
        INT
    } type;

    void Dump() const override;

    void IR() override;
};

/**
 * @brief FuncFParam ::= BType IDENT ["[" "]" {"[" ConstExp "]"}];
 * BType              ::= "int";
 */
class FuncFParamAST : public BaseAST
{
public:
    enum class Tag
    {
        INT,
        PTR
    } tag;
    std::string ident;
    std::vector<std::unique_ptr<ExpBaseAST>> const_exps;

    void Dump() const override;

    void IR() override;
};

/**
 * @brief FuncFParams ::= FuncFParam {"," FuncFParam};
 */
class FuncFParamsAST : public BaseAST
{
public:
    std::vector<std::unique_ptr<FuncFParamAST>> func_f_params;

    void Dump() const override;

    void IR() override;
};

/**
 * @brief FuncDef   ::= FuncType IDENT "(" [FuncFParams] ")" Block;
 * FuncType    ::= "void" | "int";
 */
class FuncDefAST : public BaseAST
{
public:
    std::unique_ptr<FuncTypeAST> func_type;
    std::string ident;
    std::unique_ptr<FuncFParamsAST> func_f_params;
    std::unique_ptr<BaseAST> block;

    void Dump() const override;

    void IR() override;
};

/**
 * @brief Block         ::= "{" {BlockItem} "}";
 */
class BlockAST : public BaseAST
{
public:
    std::vector<std::unique_ptr<BaseAST>> block_items;

    void Dump() const override;

    void IR() override;
};

/**
 * @brief BlockItem     ::= Decl | Stmt;
 */
class BlockItemAST : public BaseAST
{
public:
    enum class Tag
    {
        DECL,
        STMT
    } tag;
    std::unique_ptr<BaseAST> decl;
    std::unique_ptr<BaseAST> stmt;

    void Dump() const override;

    void IR() override;
};

/**
 * @brief Stmt ::= LVal "=" Exp ";"
 *              | [Exp] ";"
 *              | Block
 *              | "if" "(" Exp ")" Stmt ["else" Stmt]
 *              | "while" "(" Exp ")" Stmt
 *              | "break" ";"
 *              | "continue" ";"
 *              | "return" [Exp] ";";
 */
class StmtAST : public BaseAST
{
public:
    // TODO 两种设计（初始化和生成IR）方法

    // 上一行Koopa IR是否是br, jump, ret等跳转语句
    // 在Decl, Stmt这两种BlockItem生成IR前检查该值，若为true，则不生成IR
    // 在进入下一个Decl或Stmt前确保该值正确
    inline static bool has_jp = false;
    inline static std::stack<int> while_cnt_stk;
    enum class Tag
    {
        LVAL,
        EXP,
        BLOCK,
        IF,
        WHILE,
        BREAK,
        CONTINUE,
        RETURN
    } tag;
    std::unique_ptr<LValAST> lval;
    std::unique_ptr<ExpBaseAST> exp;
    std::unique_ptr<BaseAST> block;
    std::unique_ptr<BaseAST> if_stmt;
    std::unique_ptr<BaseAST> else_stmt;
    std::unique_ptr<BaseAST> while_stmt;

    void Dump() const override;

    void IR() override;
};

/**
 * @brief Exp         ::= LOrExp;
 */
class ExpAST : public ExpBaseAST
{
public:
    std::unique_ptr<ExpBaseAST> lor_exp;

    void Dump() const override;

    void IR() override;
};

/**
 * @brief PrimaryExp    ::= "(" Exp ")" | LVal | Number;
 */
class PrimaryExpAST : public ExpBaseAST
{
public:
    enum class Tag
    {
        EXP,
        LVAL,
        NUMBER
    } tag;
    std::unique_ptr<ExpBaseAST> exp;
    std::unique_ptr<LValAST> lval;
    int number;

    PrimaryExpAST(Tag tag) : tag(tag) {}

    void Dump() const override;

    void IR() override;
};

/**
 * @brief FuncRParams ::= Exp {"," Exp};
 */
class FuncRParamsAST : public BaseAST
{
public:
    std::vector<std::unique_ptr<ExpBaseAST>> exps;

    void Dump() const override;

    void IR() override;
};

/**
 * @brief UnaryExp    ::= PrimaryExp
 *                      | IDENT "(" [FuncRParams] ")"
 *                      | UnaryOp UnaryExp;
 */
class UnaryExpAST : public ExpBaseAST
{
public:
    enum class Tag
    {
        PRIMARY,
        IDENT,
        UNARY
    } tag;
    inline static const std::unordered_map<std::string, std::string> op_ir{
        {"-", "sub"},
        {"!", "eq"}};
    std::unique_ptr<ExpBaseAST> primary_exp;
    std::string ident;
    std::unique_ptr<FuncRParamsAST> func_r_params;
    std::string unary_op;
    std::unique_ptr<ExpBaseAST> unary_exp;

    UnaryExpAST(Tag tag) : tag(tag) {}

    void Dump() const override;

    void IR() override;
};

/**
 * @brief MulExp      ::= UnaryExp | MulExp ("*" | "/" | "%") UnaryExp;
 */
class MulExpAST : public ExpBaseAST
{
public:
    enum class Tag
    {
        UNARY,
        MUL
    } tag;
    inline static const std::unordered_map<std::string, std::string> op_ir{
        {"*", "mul"},
        {"/", "div"},
        {"%", "mod"}};
    std::unique_ptr<ExpBaseAST> unary_exp;
    std::unique_ptr<ExpBaseAST> mul_exp;
    std::string op;

    MulExpAST(Tag tag) : tag(tag) {}

    void Dump() const override;

    void IR() override;
};

/**
 * @brief AddExp      ::= MulExp | AddExp ("+" | "-") MulExp;
 */
class AddExpAST : public ExpBaseAST
{
public:
    enum class Tag
    {
        MUL,
        ADD
    } tag;
    inline static const std::unordered_map<std::string, std::string> op_ir{
        {"+", "add"},
        {"-", "sub"}};
    std::unique_ptr<ExpBaseAST> mul_exp;
    std::unique_ptr<ExpBaseAST> add_exp;
    std::string op;

    AddExpAST(Tag tag) : tag(tag) {}

    void Dump() const override;

    void IR() override;
};

/**
 * @brief RelExp      ::= AddExp | RelExp ("<" | ">" | "<=" | ">=") AddExp;
 */
class RelExpAST : public ExpBaseAST
{
public:
    enum class Tag
    {
        ADD,
        REL
    } tag;
    inline static const std::unordered_map<std::string, std::string> op_ir{
        {"<", "lt"},
        {">", "gt"},
        {"<=", "le"},
        {">=", "ge"}};
    std::unique_ptr<ExpBaseAST> add_exp;
    std::unique_ptr<ExpBaseAST> rel_exp;
    std::string op;

    RelExpAST(Tag tag) : tag(tag) {}

    void Dump() const override;

    void IR() override;
};

/**
 * @brief EqExp       ::= RelExp | EqExp ("==" | "!=") RelExp;
 */
class EqExpAST : public ExpBaseAST
{
public:
    enum class Tag
    {
        REL,
        EQ
    } tag;
    inline static const std::unordered_map<std::string, std::string> op_ir{
        {"==", "eq"},
        {"!=", "ne"}};
    std::unique_ptr<ExpBaseAST> rel_exp;
    std::unique_ptr<ExpBaseAST> eq_exp;
    std::string op;
    EqExpAST(Tag tag) : tag(tag) {}

    void Dump() const override;

    void IR() override;
};

/**
 * @brief LAndExp     ::= EqExp | LAndExp "&&" EqExp;
 */
class LAndExpAST : public ExpBaseAST
{
public:
    enum Tag
    {
        EQ,
        LAND
    } tag;
    std::unique_ptr<ExpBaseAST> eq_exp;
    std::unique_ptr<ExpBaseAST> land_exp;

    LAndExpAST(Tag tag) : tag(tag) {}

    void Dump() const override;

    void IR() override;
};

/**
 * @brief LOrExp      ::= LAndExp | LOrExp "||" LAndExp;
 *
 * @todo 分支语句加速
 */
class LOrExpAST : public ExpBaseAST
{
public:
    enum class Tag
    {
        LAND,
        LOR
    } tag;
    std::unique_ptr<ExpBaseAST> land_exp;
    std::unique_ptr<ExpBaseAST> lor_exp;

    LOrExpAST(Tag tag) : tag(tag) {}

    void Dump() const override;

    void IR() override;
};

/**
 * @brief ConstExp      ::= Exp;
 */
class ConstExpAST : public ExpBaseAST
{
public:
    std::unique_ptr<ExpBaseAST> exp;

    void Dump() const override;

    void IR() override;
};

#endif