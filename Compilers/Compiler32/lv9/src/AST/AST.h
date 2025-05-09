#ifndef AST_H
#define AST_H

#include <cassert>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
#include <memory>
#include <string>
#include <vector>

#include "utils.h"


// 类型别名定义
using VoidPtr = void*;
using ConstVoidPtr = const void*;
using VoidPtrVector = std::vector<const void*>;
using SizeVector = std::vector<size_t>;

// 全局管理器实例
static SymbolList symbol_list;
static BlockManager block_manager;
static LoopManager loop_manager;

// AST基类
class BaseAST {
public:
    // 虚析构函数
    virtual ~BaseAST() = default;

    // 转换为左值
    virtual auto to_left_value() const -> VoidPtr { return nullptr; }
    
    // 基础的Koopa IR转换
    virtual auto to_koopa() const -> VoidPtr { return nullptr; }
    
    // 带索引的Koopa IR转换
    virtual auto to_koopa(int index) const -> VoidPtr { return nullptr; }
    
    // 带全局变量的Koopa IR转换
    virtual auto to_koopa(VoidPtrVector& global_var) const -> VoidPtr { 
        return nullptr; 
    }
    
    // 带结束基本块的Koopa IR转换
    virtual auto to_koopa(koopa_raw_basic_block_t end_block) const -> VoidPtr {
        return nullptr;
    }
    
    // 带类型的Koopa IR转换
    virtual auto to_koopa(koopa_raw_type_t type) const -> VoidPtr {
        return nullptr;
    }
    
    // 带函数和值的Koopa IR转换
    virtual auto to_koopa(VoidPtrVector& func,
                         VoidPtrVector& value) const -> VoidPtr {
        return nullptr;
    }
    
    // 带全局变量和类型的Koopa IR转换
    virtual auto to_koopa(VoidPtrVector& global_var,
                         koopa_raw_type_t type) const -> VoidPtr {
        return nullptr;
    }
    
    // 带初始化列表的Koopa IR转换
    virtual auto to_koopa(VoidPtrVector& init_list, 
                         SizeVector size_vec,
                         int level) const -> VoidPtr {
        return nullptr;
    }
    
    // 计算值
    virtual auto cal_value() const -> int { assert(false); }
};

////////////////////////////////////////////////////////////////////////////////


// 类型别名定义
using ASTPtr = std::unique_ptr<BaseAST>;
using ASTPtrVector = std::vector<ASTPtr>;
using ASTPtrVectorPtr = std::unique_ptr<ASTPtrVector>;
using VoidPtr = void*;
using VoidPtrVector = std::vector<const void*>;

// 编译单元AST节点
class CompUnitAST : public BaseAST {
public:
    // 定义向量
    ASTPtrVectorPtr def_vec;

    // 构造函数
    explicit CompUnitAST(ASTPtrVectorPtr& def_vec);

    // 加载库函数
    auto load_lib_func(VoidPtrVector& lib_func_vec) const -> void;

    // 转换为Koopa IR
    auto to_koopa() const -> VoidPtr override;
};

// 定义AST节点
class DefAST : public BaseAST {
public:
    // 定义类型枚举
    enum DefType { 
        FuncDef,    // 函数定义
        ConstDef,   // 常量定义
        VarDef      // 变量定义
    };

    DefType type;   // 定义类型
    ASTPtr def;     // 具体定义

    // 构造函数
    DefAST(ASTPtr& def, DefType type);

    // 转换为Koopa IR
    auto to_koopa(VoidPtrVector& func,
                 VoidPtrVector& value) const -> VoidPtr override;
};

// 函数定义AST节点
class FuncDefAST : public BaseAST {
public:
    ASTPtr func_type;           // 函数类型
    std::string ident;          // 函数标识符
    ASTPtr block;               // 函数体
    ASTPtrVectorPtr param_vec;  // 参数向量

    // 构造函数
    FuncDefAST(ASTPtr& func_type, 
               const char* ident,
               ASTPtrVectorPtr& param_vec,
               ASTPtr& block);

    // 转换为Koopa IR
    auto to_koopa() const -> VoidPtr override;
};


////////////////////////////////////////////////////////////////////////////////

// 类型别名定义
using ASTPtr = std::unique_ptr<BaseAST>;
using ASTPtrVector = std::vector<ASTPtr>;
using ASTPtrVectorPtr = std::unique_ptr<ASTPtrVector>;
using VoidPtr = void*;
using VoidPtrVector = std::vector<const void*>;

// 全局常量定义AST节点
class GlobalConstDefAST : public BaseAST {
public:
    ASTPtr const_type;              // 常量类型
    ASTPtrVectorPtr ConstDef_vec;   // 常量定义向量

    // 构造函数
    GlobalConstDefAST(ASTPtr& const_type,
                     ASTPtrVectorPtr& ConstDef_vec);

    // 转换为Koopa IR
    auto to_koopa() const -> VoidPtr override;
};

// 全局变量定义AST节点
class GlobalVarDefAST : public BaseAST {
public:
    ASTPtr var_type;            // 变量类型
    ASTPtrVectorPtr VarDef_vec; // 变量定义向量

    // 构造函数
    GlobalVarDefAST(ASTPtr& var_type,
                   ASTPtrVectorPtr& VarDef_vec);

    // 转换为Koopa IR
    auto to_koopa(VoidPtrVector& global_var) const -> VoidPtr override;
};

// 函数形参AST节点
class FuncFParamAST : public BaseAST {
public:
    // 函数参数类型枚举
    enum FuncFParamType { 
        Array,  // 数组参数
        Var     // 变量参数
    };

    ASTPtr param_type;          // 参数类型
    std::string ident;          // 参数标识符
    FuncFParamType type;        // 参数种类
    ASTPtrVectorPtr index_array;// 数组索引

    // 构造函数 - 普通变量参数
    FuncFParamAST(ASTPtr& param_type, 
                  const char* ident, 
                  FuncFParamType type);

    // 构造函数 - 数组参数
    FuncFParamAST(ASTPtr& param_type,
                  ASTPtrVectorPtr& index_array,
                  const char* ident, 
                  FuncFParamType type);

    // 转换为Koopa IR
    auto to_koopa() const -> VoidPtr override;
    auto to_koopa(int index) const -> VoidPtr override;
};

// 代码块AST节点
class BlockAST : public BaseAST {
public:
    // 代码块类型枚举
    enum BlockType { 
        Item,   // 包含语句
        Empty   // 空块
    } type;

    ASTPtrVectorPtr blockitem_vec;  // 语句向量

    // 构造函数 - 空块
    BlockAST();

    // 构造函数 - 含语句块
    explicit BlockAST(ASTPtrVectorPtr& blockitem_vec);

    // 转换为Koopa IR
    auto to_koopa() const -> VoidPtr override;
};

////////////////////////////////////////////////////////////////////////////////

// 类型别名定义
using ASTPtr = std::unique_ptr<BaseAST>;
using ASTPtrVector = std::vector<ASTPtr>;
using ASTPtrVectorPtr = std::unique_ptr<ASTPtrVector>;
using VoidPtr = void*;
using VoidPtrVector = std::vector<const void*>;

// 语句AST节点
class StmtAST : public BaseAST {
public:
    // 语句类型枚举
    enum StmtType {
        Exp,        // 表达式语句
        Assign,     // 赋值语句
        Block,      // 代码块
        Return,     // 返回语句
        Empty,      // 空语句
        If,         // if语句
        While,      // while循环
        Break,      // break语句
        Continue    // continue语句
    };

    StmtType type;     // 语句类型
    ASTPtr exp;        // 表达式部分
    ASTPtr stmt;       // 语句部分

    // 构造函数 - 仅类型
    explicit StmtAST(StmtType type);

    // 构造函数 - 类型和表达式
    StmtAST(ASTPtr& exp, StmtType type);

    // 构造函数 - 类型、语句和表达式
    StmtAST(ASTPtr& stmt, ASTPtr& exp, StmtType type);

    // 转换为Koopa IR
    auto to_koopa() const -> VoidPtr override;
};

// If语句AST节点
class IfAST : public BaseAST {
public:
    ASTPtr exp;    // 条件表达式
    ASTPtr stmt;   // if语句体

    // 构造函数
    IfAST(ASTPtr& exp, ASTPtr& stmt);

    // 转换为Koopa IR
    auto to_koopa() const -> VoidPtr override;
};

// 常量声明AST节点
class ConstDeclAST : public BaseAST {
public:
    ASTPtr const_type;          // 常量类型
    ASTPtrVectorPtr ConstDef_vec; // 常量定义向量

    // 构造函数
    ConstDeclAST(ASTPtr& const_type,
                 ASTPtrVectorPtr& ConstDef_vec);

    // 转换为Koopa IR
    auto to_koopa() const -> VoidPtr override;
    
    // 带全局变量的Koopa IR转换
    auto to_koopa(VoidPtrVector& global_var) const -> VoidPtr override;
};

////////////////////////////////////////////////////////////////////////////////


// 类型别名定义
using ASTPtr = std::unique_ptr<BaseAST>;
using ASTPtrVector = std::vector<ASTPtr>;
using ASTPtrVectorPtr = std::unique_ptr<ASTPtrVector>;
using VoidPtr = void*;
using VoidPtrVector = std::vector<const void*>;

// 类型AST节点
class TypeAST : public BaseAST {
public:
    std::string type;   // 类型名称

    // 构造函数
    explicit TypeAST(const char* type);

    // 转换为Koopa IR
    auto to_koopa() const -> VoidPtr override;
};

// 常量定义AST节点
class ConstDefAST : public BaseAST {
public:
    // 常量定义类型枚举
    enum ConstDefType { 
        Var,    // 变量
        Array   // 数组
    } type;

    std::string ident;          // 标识符
    ASTPtr exp;                 // 初始化表达式
    ASTPtrVectorPtr index_array;// 数组索引

    // 构造函数 - 变量形式
    ConstDefAST(const char* ident, 
                ASTPtr& init_array);

    // 构造函数 - 数组形式
    ConstDefAST(const char* ident,
                ASTPtrVectorPtr& index_array,
                ASTPtr& init_array);

    // 转换为Koopa IR（基本形式）
    auto to_koopa(koopa_raw_type_t type) const -> VoidPtr override;

    // 转换为Koopa IR（全局变量形式）
    auto to_koopa(VoidPtrVector& global_var,
                  koopa_raw_type_t type) const -> VoidPtr override;
};

////////////////////////////////////////////////////////////////////////////////

// 类型别名定义
using ASTPtr = std::unique_ptr<BaseAST>;
using ASTPtrVector = std::vector<ASTPtr>;
using ASTPtrVectorPtr = std::unique_ptr<ASTPtrVector>;
using VoidPtr = void*;
using VoidPtrVector = std::vector<const void*>;

// 变量声明AST节点
class VarDeclAST : public BaseAST {
public:
    ASTPtr var_type;          // 变量类型
    ASTPtrVectorPtr VarDef_vec; // 变量定义向量

    // 构造函数
    VarDeclAST(ASTPtr& var_type,
               ASTPtrVectorPtr& VarDef_vec);

    // 转换为Koopa IR（基本形式）
    auto to_koopa() const -> VoidPtr override;

    // 转换为Koopa IR（全局变量形式）
    auto to_koopa(VoidPtrVector& global_var) const -> VoidPtr override;
};

// 变量定义AST节点
class VarDefAST : public BaseAST {
public:
    // 变量定义类型枚举
    enum VarDefType { 
        Exp,    // 表达式初始化
        Array   // 数组定义
    };

    VarDefType type;          // 定义类型
    std::string ident;        // 变量标识符
    ASTPtr exp;               // 初始化表达式
    ASTPtrVectorPtr index_array; // 数组索引

    // 构造函数 - 带表达式的变量定义
    VarDefAST(const char* ident, 
              ASTPtr& exp, 
              VarDefType type);

    // 构造函数 - 数组定义（无初始化）
    VarDefAST(const char* ident,
              ASTPtrVectorPtr& index_array,
              VarDefType type);

    // 构造函数 - 简单变量定义（无初始化）
    explicit VarDefAST(const char* ident, 
                      VarDefType type);

    // 构造函数 - 带初始化的数组定义
    VarDefAST(const char* ident,
              ASTPtrVectorPtr& index_array,
              ASTPtr& exp,
              VarDefType type);

    // 转换为Koopa IR（基本形式）
    auto to_koopa(koopa_raw_type_t type) const -> VoidPtr override;

    // 转换为Koopa IR（全局变量形式）
    auto to_koopa(VoidPtrVector& global_var,
                  koopa_raw_type_t type) const -> VoidPtr override;
};

////////////////////////////////////////////////////////////////////////////////

// 类型别名定义
using ASTPtr = std::unique_ptr<BaseAST>;
using ASTPtrVector = std::vector<ASTPtr>;
using ASTPtrVectorPtr = std::unique_ptr<ASTPtrVector>;
using VoidPtr = void*;
using VoidPtrVector = std::vector<const void*>;
using SizeVector = std::vector<size_t>;

// 初始化值AST节点
class InitValAST : public BaseAST {
public:
    // 初始化类型枚举
    enum InitType { 
        Exp,        // 表达式初始化
        InitList,   // 列表初始化
        Empty       // 空初始化
    } type;

    ASTPtr exp;                 // 表达式
    ASTPtrVectorPtr initlist_vec; // 初始化列表

    // 构造函数
    InitValAST();  // 空初始化
    explicit InitValAST(ASTPtr& exp);  // 表达式初始化
    explicit InitValAST(ASTPtrVectorPtr& initlist_vec);  // 列表初始化

    // 转换为Koopa IR
    auto to_koopa(VoidPtrVector& init_vec, 
                  SizeVector size_vec, 
                  int level) const -> VoidPtr override;
    auto to_koopa() const -> VoidPtr override;
    
    // 计算值
    auto cal_value() const -> int override;
    
    // 预处理初始化列表
    auto preprocess(VoidPtrVector& init_vec,
                    SizeVector size_vec) -> void;
};

// 左值AST节点
class LValAST : public BaseAST {
public:
    std::string ident;          // 标识符
    ASTPtrVectorPtr index_array;  // 数组索引

    // 构造函数
    explicit LValAST(const char* ident);
    LValAST(const char* ident, ASTPtrVectorPtr& index_array);

    // 转换相关函数
    auto to_left_value() const -> VoidPtr override;
    auto to_koopa() const -> VoidPtr override;
    auto cal_value() const -> int override;
};

// 表达式AST节点
class ExpAST : public BaseAST {
public:
    ASTPtr add_exp;  // 加法表达式

    // 构造函数
    explicit ExpAST(ASTPtr& add_exp);

    // 转换和计算函数
    auto to_koopa() const -> VoidPtr override;
    auto cal_value() const -> int override;
};

// 主表达式AST节点
class PrimaryExpAST : public BaseAST {
public:
    ASTPtr exp;  // 表达式

    // 构造函数
    explicit PrimaryExpAST(ASTPtr& exp);

    // 转换和计算函数
    auto to_koopa() const -> VoidPtr override;
    auto cal_value() const -> int override;
};


////////////////////////////////////////////////////////////////////////////////


// 类型别名定义
using ASTPtr = std::unique_ptr<BaseAST>;
using ASTPtrVector = std::vector<ASTPtr>;
using ASTPtrVectorPtr = std::unique_ptr<ASTPtrVector>;
using VoidPtr = void*;

// 一元表达式AST节点
class UnaryExpAST : public BaseAST {
public:
    // 一元表达式类型枚举
    enum UnaryType { 
        Exp,    // 表达式
        Op,     // 运算符
        Call    // 函数调用
    } type;

    std::string op;           // 运算符
    ASTPtr exp;              // 表达式
    ASTPtrVectorPtr args;    // 函数参数列表

    // 构造函数 - 表达式形式
    explicit UnaryExpAST(ASTPtr& exp);
    
    // 构造函数 - 运算符形式
    UnaryExpAST(const char* op, ASTPtr& exp);
    
    // 构造函数 - 函数调用形式
    UnaryExpAST(const char* op, ASTPtrVectorPtr& args);

    // 转换和计算函数
    auto to_koopa() const -> VoidPtr override;
    auto cal_value() const -> int override;
};

// 加法表达式AST节点
class AddExpAST : public BaseAST {
public:
    // 加法表达式类型枚举
    enum AddType { 
        Exp,    // 表达式
        Op      // 运算符
    } type;

    std::string op;        // 运算符
    ASTPtr add_exp;       // 加法表达式
    ASTPtr mul_exp;       // 乘法表达式

    // 构造函数 - 表达式形式
    explicit AddExpAST(ASTPtr& add_exp);
    
    // 构造函数 - 运算符形式
    AddExpAST(const char* op, 
              ASTPtr& add_exp,
              ASTPtr& mul_exp);

    // 转换和计算函数
    auto to_koopa() const -> VoidPtr override;
    auto cal_value() const -> int override;
};

// 乘法表达式AST节点
class MulExpAST : public BaseAST {
public:
    // 乘法表达式类型枚举
    enum MulType { 
        Exp,    // 表达式
        Op      // 运算符
    } type;

    std::string op;         // 运算符
    ASTPtr mul_exp;        // 乘法表达式
    ASTPtr unary_exp;      // 一元表达式

    // 构造函数 - 表达式形式
    explicit MulExpAST(ASTPtr& unary_exp);
    
    // 构造函数 - 运算符形式
    MulExpAST(const char* op, 
              ASTPtr& mul_exp,
              ASTPtr& unary_exp);

    // 转换和计算函数
    auto to_koopa() const -> VoidPtr override;
    auto cal_value() const -> int override;
};

////////////////////////////////////////////////////////////////////////////////

// 类型别名定义
using ASTPtr = std::unique_ptr<BaseAST>;
using VoidPtr = void*;

// 关系表达式AST节点
class RelExpAST : public BaseAST {
public:
    // 关系表达式类型枚举
    enum RelType { 
        Exp,    // 表达式
        Op      // 运算符
    } type;

    std::string op;        // 关系运算符
    ASTPtr rel_exp;       // 关系表达式
    ASTPtr add_exp;       // 加法表达式

    // 构造函数 - 表达式形式
    explicit RelExpAST(ASTPtr& add_exp);
    
    // 构造函数 - 运算符形式
    RelExpAST(const char* op, 
              ASTPtr& rel_exp,
              ASTPtr& add_exp);

    // 转换和计算函数
    auto to_koopa() const -> VoidPtr override;
    auto cal_value() const -> int override;
};

// 相等性表达式AST节点
class EqExpAST : public BaseAST {
public:
    // 相等性表达式类型枚举
    enum EqType { 
        Exp,    // 表达式
        Op      // 运算符
    } type;

    std::string op;        // 相等性运算符
    ASTPtr eq_exp;        // 相等性表达式
    ASTPtr rel_exp;       // 关系表达式

    // 构造函数 - 表达式形式
    explicit EqExpAST(ASTPtr& rel_exp);
    
    // 构造函数 - 运算符形式
    EqExpAST(const char* op, 
             ASTPtr& eq_exp,
             ASTPtr& rel_exp);

    // 转换和计算函数
    auto to_koopa() const -> VoidPtr override;
    auto cal_value() const -> int override;
};


////////////////////////////////////////////////////////////////////////////////


// 类型别名定义
using ASTPtr = std::unique_ptr<BaseAST>;
using VoidPtr = void*;

// 逻辑与表达式AST节点
class LAndExpAST : public BaseAST {
public:
    // 逻辑与表达式类型枚举
    enum AndType { 
        Exp,    // 表达式
        Op      // 运算符
    } type;

    std::string op;        // 逻辑运算符
    ASTPtr and_exp;       // 与表达式
    ASTPtr eq_exp;        // 相等性表达式

    // 构造函数 - 表达式形式
    explicit LAndExpAST(ASTPtr& eq_exp);
    
    // 构造函数 - 运算符形式
    LAndExpAST(const char* op, 
               ASTPtr& and_exp,
               ASTPtr& eq_exp);

    // 布尔值转换函数
    auto make_bool(const ASTPtr& exp) const -> VoidPtr;
    
    // 转换和计算函数
    auto to_koopa() const -> VoidPtr override;
    auto cal_value() const -> int override;
};

// 逻辑或表达式AST节点
class LOrExpAST : public BaseAST {
public:
    // 逻辑或表达式类型枚举
    enum OrType { 
        Exp,    // 表达式
        Op      // 运算符
    } type;

    std::string op;        // 逻辑运算符
    ASTPtr or_exp;        // 或表达式
    ASTPtr and_exp;       // 与表达式

    // 构造函数 - 表达式形式
    explicit LOrExpAST(ASTPtr& and_exp);
    
    // 构造函数 - 运算符形式
    LOrExpAST(const char* op, 
              ASTPtr& or_exp,
              ASTPtr& and_exp);

    // 布尔值转换函数
    auto make_bool(const ASTPtr& exp) const -> VoidPtr;
    
    // 转换和计算函数
    auto to_koopa() const -> VoidPtr override;
    auto cal_value() const -> int override;
};

// 数值AST节点
class NumberAST : public BaseAST {
public:
    int val;  // 数值

    // 构造函数
    explicit NumberAST(int val);

    // 转换和计算函数
    auto to_koopa() const -> VoidPtr override;
    auto cal_value() const -> int override;
};


#endif // AST_H