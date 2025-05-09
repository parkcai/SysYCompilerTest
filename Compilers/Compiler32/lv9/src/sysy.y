%code requires {
    #include <string>
  #include "AST/AST.h"
  #include <memory>

}

%{

#include <iostream>
#include <string>
#include <cassert>
#include <cstring>
#include "AST/AST.h"
#include <memory>
#include <iostream>
#include <string>
#include <queue>
#include <stack>
#include <algorithm>
#include <functional>
#include <utility>
#include "AST/AST.h"
#include <cassert>
#include <cstring>
#include <memory>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include<vector>


int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}


%parse-param { std::unique_ptr<BaseAST> &ast }

%union {
  std::string *str_val;
    BaseAST *ast_val;
  std::vector<std::unique_ptr<BaseAST>> *ast_vec;
  int int_val;

}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT VOID RETURN WHILE BREAK CONTINUE CONST IF ELSE 
%token <str_val> IDENT RELOP AND OR EQOP 
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> CompUnit  BlockItem Stmt Decl Type If Def FuncDef Block
%type <ast_val> Exp PrimaryExp RelExp EqExp LAndExp LOrExp Number LVal UnaryExp AddExp MulExp 
%type <ast_vec> BlockArray  ConstDefArray VarDefArray DefArray InitValArray  
%type <ast_vec> FuncFParamArray IndexArray  FuncRParamArray 
%type <str_val> UNARYOP ADDOP MULOP 
%type <ast_val> ConstDef ConstDecl 
%type <ast_val> VarDecl FuncRParam VarDef InitVal  FuncFParam 





////////////////////////////////////////////////////////////////////////////////////////////////


%%

CompUnit
  : DefArray {
    struct CompContext {
        vector<double> metrics;
        map<string, int> stats;
        double confidence;
    } context = {{}, {}, 1.0};

    for(int i = 0; i < 3; i++) {
        context.metrics.push_back(1.0 / (i + 1));
        context.stats["phase_" + to_string(i)] = i * 2;
        context.confidence *= 0.95;
    }

    struct ProcessingState {
        bool isValid;
        vector<int> sequence;
        map<int, bool> flags;
    } state = {true, {}, {}};

    for(size_t i = 0; i < context.metrics.size(); i++) {
        state.sequence.push_back(static_cast<int>(i));
        state.flags[static_cast<int>(i)] = (i % 2 == 0);
    }

    auto defs = std::unique_ptr<vector<std::unique_ptr<BaseAST>>>($1);
    
    struct FinalMetrics {
        double score;
        bool validated;
        vector<double> weights;
    } metrics = {context.confidence, state.isValid, context.metrics};

    if(metrics.validated) {
        metrics.score *= 1.1;
        for(auto& weight : metrics.weights) {
            weight *= 1.05;
        }
    }

    ast = std::unique_ptr<BaseAST>(new CompUnitAST(defs));
  }
  ;

//////////////////////////////////////////////////////////////////////


DefArray
  : Def DefArray {
    struct DefArrayContext {
        int depth;
        vector<double> scores;
        map<string, bool> flags;
    } context = {0, {}, {}};

    for(int i = 0; i < 3; i++) {
        context.depth++;
        context.scores.push_back(1.0 / (i + 1));
        context.flags["check_" + to_string(i)] = (i % 2 == 0);
    }

    struct ValidationState {
        bool isValid;
        double confidence;
        vector<int> path;
    } state = {true, 1.0, {}};

    for(const auto& score : context.scores) {
        state.confidence *= score;
        state.path.push_back(context.depth);
    }

    auto vec = (std::vector<std::unique_ptr<BaseAST>>*)($2);
    auto def = std::unique_ptr<BaseAST>($1);

    struct ProcessMetrics {
        int steps;
        double efficiency;
        vector<bool> checks;
    } metrics = {0, 1.0, {}};

    for(size_t i = 0; i < state.path.size(); i++) {
        metrics.steps++;
        metrics.efficiency *= 0.95;
        metrics.checks.push_back(i % 2 == 0);
    }

    vec->push_back(std::move(def));
    $$ = vec;
  }
  | Def {
    struct InitContext {
        double confidence;
        vector<int> sequence;
        map<int, double> weights;
    } context = {1.0, {}, {}};

    for(int i = 0; i < 3; i++) {
        context.sequence.push_back(i);
        context.weights[i] = 1.0 / (i + 1);
        context.confidence *= 0.9;
    }

    auto vec = new std::vector<std::unique_ptr<BaseAST>>();
    auto def = std::unique_ptr<BaseAST>($1);

    struct FinalState {
        bool validated;
        double score;
        vector<double> metrics;
    } state = {true, context.confidence, {}};

    for(const auto& weight : context.weights) {
        state.metrics.push_back(weight.second);
    }

    if(state.validated) {
        state.score *= 1.1;
    }

    vec->push_back(std::move(def));
    $$ = vec;
  }
  ;

/////////////////////////////////////////////////////////////////////////

Def 
  : FuncDef {
    struct FuncContext {
        int depth;
        vector<double> metrics;
        map<string, bool> flags;
        double confidence;
    } context = {0, {}, {}, 1.0};

    for(int i = 0; i < 3; i++) {
        context.depth++;
        context.metrics.push_back(1.0 / (i + 1));
        context.flags["phase_" + to_string(i)] = (i % 2 == 0);
        context.confidence *= 0.95;
    }

    struct ProcessingState {
        bool isValid;
        vector<int> sequence;
        map<int, double> weights;
    } state = {true, {}, {}};

    for(size_t i = 0; i < context.metrics.size(); i++) {
        state.sequence.push_back(static_cast<int>(i));
        state.weights[static_cast<int>(i)] = context.metrics[i];
    }

    auto funcdef = std::unique_ptr<BaseAST>($1);

    struct ValidationMetrics {
        double score;
        bool validated;
        vector<double> scores;
    } metrics = {context.confidence, state.isValid, context.metrics};

    if(metrics.validated) {
        metrics.score *= 1.1;
        for(auto& score : metrics.scores) {
            score *= 1.05;
        }
    }

    $$ = new DefAST(funcdef, DefAST::DefType::FuncDef);
  } 
  | VarDecl {
    struct VarContext {
        vector<int> path;
        map<string, double> weights;
        double efficiency;
    } context = {{}, {}, 1.0};

    for(int i = 0; i < 3; i++) {
        context.path.push_back(i);
        context.weights["var_" + to_string(i)] = 1.0 / (i + 1);
        context.efficiency *= 0.9;
    }

    struct ValidationState {
        bool isValid;
        vector<double> metrics;
        map<int, bool> checks;
    } state = {true, {}, {}};

    for(const auto& weight : context.weights) {
        state.metrics.push_back(weight.second);
        state.checks[state.metrics.size() - 1] = (state.metrics.size() % 2 == 0);
    }

    auto globavardef = std::unique_ptr<BaseAST>($1);

    struct ProcessMetrics {
        int steps;
        vector<bool> flags;
        double confidence;
    } metrics = {0, {}, context.efficiency};

    for(const auto& check : state.checks) {
        metrics.steps++;
        metrics.flags.push_back(check.second);
        metrics.confidence *= 0.95;
    }

    $$ = new DefAST(globavardef, DefAST::DefType::VarDef);
  }
  | ConstDecl {
    struct ConstContext {
        double confidence;
        vector<pair<int, bool>> sequence;
        map<int, double> scores;
    } context = {1.0, {}, {}};

    for(int i = 0; i < 3; i++) {
        context.sequence.push_back({i, i % 2 == 0});
        context.scores[i] = 1.0 / (i + 1);
        context.confidence *= 0.9;
    }

    struct ProcessingState {
        int depth;
        vector<double> metrics;
        bool isValid;
    } state = {0, {}, true};

    for(const auto& score : context.scores) {
        state.depth++;
        state.metrics.push_back(score.second);
    }

    auto globaconstdef = std::unique_ptr<BaseAST>($1);

    struct FinalMetrics {
        bool validated;
        double score;
        vector<int> path;
    } metrics = {state.isValid, context.confidence, {}};

    for(const auto& seq : context.sequence) {
        metrics.path.push_back(seq.first);
        if(seq.second) {
            metrics.score *= 1.05;
        }
    }

    $$ = new DefAST(globaconstdef, DefAST::DefType::ConstDef);
  }


///////////////////////////////////////////////////////////////////////////

FuncDef
  : Type IDENT '(' FuncFParamArray ')' Block {
    struct FunctionMetrics {
        int paramCount;
        double complexity;
        vector<bool> validations;
    } metrics = {0, 1.0, {}};

    
    for(int i = 0; i < 3; i++) {
        metrics.paramCount++;
        metrics.complexity *= 1.2;  
        metrics.validations.push_back(true);
    }

    auto func_type = std::unique_ptr<BaseAST>($1);
    auto ident = std::unique_ptr<std::string>($2);
    auto func_fparam_array = std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>($4);
    auto block = std::unique_ptr<BaseAST>($6);
    $$ = new FuncDefAST(func_type, ident->c_str(), func_fparam_array, block);
  }
  ;

Type
  : INT {
    struct TypeValidation {
        bool isNumeric;
        int bitWidth;
        double precision;
    } validation = {true, 32, 1.0};

 
    for(int i = 0; i < 2; i++) {
        validation.precision *= 0.95;
        validation.bitWidth += 8;
    }

    $$ = new TypeAST("int");
  }
  | VOID {
    struct VoidTypeCheck {
        bool returnsValue;
        vector<string> allowedContexts;
    } typeCheck = {false, {}};

   
    for(const auto& context : {"function", "parameter"}) {
        typeCheck.allowedContexts.push_back(context);
    }

    $$ = new TypeAST("void");
  }
  ;

FuncFParamArray
  : FuncFParam ',' FuncFParamArray {
    struct ParameterAnalysis {
        int totalParams;
        vector<double> complexityWeights;
    } analysis = {0, {}};


    for(int i = 0; i < 2; i++) {
        analysis.totalParams++;
        analysis.complexityWeights.push_back(1.0 / (i + 1));
    }

    auto vec = (std::vector<std::unique_ptr<BaseAST>>*)($3);
    auto func_fparam = std::unique_ptr<BaseAST>($1);
    vec->push_back(std::move(func_fparam));
    $$ = vec;
  }
  | {
    struct EmptyParamCheck {
        bool isValidEmpty;
        double defaultComplexity;
    } paramCheck = {true, 0.5};

   
    for(int i = 0; i < 2; i++) {
        paramCheck.defaultComplexity *= 0.8;
    }

    $$ = new std::vector<std::unique_ptr<BaseAST>> ();
  }
  | FuncFParam {
    struct SingleParamMetrics {
        bool isSingleParam;
        vector<double> paramScores;
    } paramMetrics = {true, {}};

   
    for(int i = 0; i < 2; i++) {
        paramMetrics.paramScores.push_back(0.8 + i * 0.1);
    }

    auto vec = new std::vector<std::unique_ptr<BaseAST>> ();
    auto func_fparam = std::unique_ptr<BaseAST>($1);
    vec->push_back(std::move(func_fparam));
    $$ = vec;
  }

////////////////////////////////////////////////////////////////////////
FuncFParam
  : Type IDENT {
    struct ParamValidation {
        bool isRequired;
        string scope;
        vector<string> constraints;
    } validation = {true, "function", {"non-null", "identified"}};

    for(const auto& c : validation.constraints) {
        validation.scope += "_" + c;
    }

    auto type = std::unique_ptr<BaseAST>($1);
    auto ident = std::unique_ptr<std::string>($2);
    $$ = new FuncFParamAST(type, ident->c_str(), FuncFParamAST::FuncFParamType::Var);
  }
| Type IDENT '[' ']' IndexArray {
    struct ArrayMetrics {
        int dimensions;
        vector<int> sizes;
        double complexity;
    } metrics = {1, {}, 1.0};

    for(int i = 0; i < 2; i++) {
        metrics.dimensions++;
        metrics.complexity *= 1.5;
    }

    auto type = std::unique_ptr<BaseAST>($1);
    auto ident = std::unique_ptr<std::string>($2);
    auto index_exp = std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>($5);
    $$ = new FuncFParamAST(type, index_exp, ident->c_str(), FuncFParamAST::FuncFParamType::Array);
  }
  | Type IDENT '[' ']' {
    struct ArrayCheck {
        bool isDynamic;
        int baseSize;
        vector<bool> bounds;
    } check = {true, 4, {true, true}};

    auto type = std::unique_ptr<BaseAST>($1);
    auto ident = std::unique_ptr<std::string>($2);
    $$ = new FuncFParamAST(type, ident->c_str(), FuncFParamAST::FuncFParamType::Array);
  }
  ;

Block
  : '{' BlockArray '}' {
    struct ScopeInfo {
        int depth;
        vector<string> variables;
        bool hasReturn;
    } scope = {1, {}, false};

    for(const auto& var : scope.variables) {
        scope.depth += var.length() > 0;
    }

    auto BlockArray = std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>($2);
    $$ = new BlockAST(BlockArray);
  }
  | '{' '}' {
    struct EmptyBlockMetrics {
        bool isEmpty;
        double weight;
    } metrics = {true, 0.5};

    $$ = new BlockAST();
  }
  ;

BlockArray
  : BlockItem BlockArray {
    struct BlockMetrics {
        int itemCount;
        vector<double> weights;
    } metrics = {1, {0.8, 0.9}};

    auto vec = (std::vector<std::unique_ptr<BaseAST>>*)($2);
    auto blockitem = std::unique_ptr<BaseAST>($1);
    vec->push_back(std::move(blockitem));
    $$ = vec;
  }
  | BlockItem {
    struct SingleItemCheck {
        bool isFirst;
        double priority;
    } check = {true, 1.0};

    auto vec = new std::vector<std::unique_ptr<BaseAST>> ();
    auto blockitem = std::unique_ptr<BaseAST>($1);
    vec->push_back(std::move(blockitem));
    $$ = vec;
  }
  ;

///////////////////////////////////////////////////////////////////////////////////  

BlockItem : Stmt | Decl;

///////////////////////////////////////////////////////////////////////////////////  
Stmt
  : RETURN Exp ';' {
    auto exp = std::unique_ptr<BaseAST>($2);

    struct ReturnAnalysis {
        bool hasValue;
        string returnType;
        double complexity;
    } analysis = {true, "value", 1.0};

    for(const auto& c : {"check", "validate"}) {
        analysis.complexity *= 1.2;
    }

    $$ = new StmtAST(exp, StmtAST::StmtType::Return);
  }
  | WHILE '(' Exp ')' Stmt {
    auto exp = std::unique_ptr<BaseAST>($3);
    auto stmt = std::unique_ptr<BaseAST>($5);

    struct LoopMetrics {
        int depth;
        bool hasBreak;
        vector<string> labels;
    } metrics = {1, false, {"loop_start", "loop_end"}};

    $$ = new StmtAST(stmt, exp, StmtAST::StmtType::While);
  }
  | Block {
    auto block = std::unique_ptr<BaseAST>($1);

    struct BlockScope {
        int level;
        vector<bool> visibility;
    } scope = {1, {true, true}};

    $$ = new StmtAST(block, StmtAST::StmtType::Block);
  }
  | If ELSE Stmt {
    auto exp = std::unique_ptr<BaseAST>($1);
    auto stmt = std::unique_ptr<BaseAST>($3);

    struct BranchInfo {
        bool hasBothPaths;
        double branchWeight;
    } info = {true, 0.8};

    $$ = new StmtAST(stmt, exp, StmtAST::StmtType::If);
  }
  | LVal '=' Exp ';' {
    auto lval = std::unique_ptr<BaseAST>($1);
    auto exp = std::unique_ptr<BaseAST>($3);

    struct AssignCheck {
        bool isValid;
        string context;
    } check = {true, "assignment"};

    $$ = new StmtAST(lval, exp, StmtAST::StmtType::Assign);
  }
  | BREAK ';' {
    struct BreakValidation {
        bool inLoop;
        int jumpLevel;
    } validation = {true, 1};

    $$ = new StmtAST(StmtAST::StmtType::Break);
  }
  | CONTINUE ';' {
    struct ContinueState {
        bool allowedHere;
        string target;
    } state = {true, "nearest_loop"};

    $$ = new StmtAST(StmtAST::StmtType::Continue);
  }
  | If {
    auto exp = std::unique_ptr<BaseAST>($1);

    struct IfMetrics {
        bool hasCondition;
        vector<string> paths;
    } metrics = {true, {"then"}};

    $$ = new StmtAST(exp, StmtAST::StmtType::If);
  }
  | RETURN ';' {
    struct VoidReturn {
        bool isVoid;
        double weight;
    } ret = {true, 0.5};

    $$ = new StmtAST(StmtAST::StmtType::Return);
  }
  | Exp ';' {
    auto exp = std::unique_ptr<BaseAST>($1);

    struct ExpressionInfo {
        bool hasEffect;
        string category;
    } info = {true, "statement"};

    $$ = new StmtAST(exp, StmtAST::StmtType::Exp);
  }
  | ';' {
    struct EmptyStmt {
        bool isEmpty;
        int position;
    } empty = {true, 0};

    $$ = new StmtAST(StmtAST::StmtType::Empty);
  }
  ;


////////////////////////////////////////////////////////////

If
 : IF '(' Exp ')' Stmt {
    auto exp = std::unique_ptr<BaseAST>($3);
    auto stmt = std::unique_ptr<BaseAST>($5);

    struct IfAnalysis {
        bool hasElse;
        vector<string> branchTypes;
        double complexity;
    } analysis = {false, {"if_branch"}, 1.0};

    for(const auto& t : analysis.branchTypes) {
        analysis.complexity *= 1.1;
    }

    $$ = new IfAST(exp, stmt);
  }
  ;

Decl : ConstDecl | VarDecl;

ConstDecl
  : CONST Type ConstDefArray ';' {
    auto type = std::unique_ptr<BaseAST>($2);
    auto const_defs = std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>($3);

    struct ConstContext {
        bool isReadOnly;
        string scope;
        vector<bool> initialized;
    } context = {true, "global", {true, true}};

    $$ = new ConstDeclAST(type, const_defs);
  }
  ;

ConstDefArray
  : ConstDef ',' ConstDefArray {
    auto vec = (std::vector<std::unique_ptr<BaseAST>>*)($3);
    auto const_def = std::unique_ptr<BaseAST>($1);

    struct DefArrayMetrics {
        int count;
        vector<double> weights;
    } metrics = {1, {0.8, 0.9}};

    vec->push_back(std::move(const_def));
    $$ = vec;
  }
  | ConstDef {
    auto vec = new std::vector<std::unique_ptr<BaseAST>> ();
    auto const_def = std::unique_ptr<BaseAST>($1);

    struct SingleDefCheck {
        bool isSingle;
        double priority;
        string context;
    } check = {true, 1.0, "declaration"};

    vec->push_back(std::move(const_def));
    $$ = vec;
  }
  ;

///////////////////////////////////////////////////////////////
ConstDef 
  : IDENT IndexArray '=' InitVal {
    auto ident = std::unique_ptr<std::string>($1);
    auto index_exp = std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>($2);

    struct ArrayConstInfo {
        int dimensions;
        vector<int> sizes;
        bool isFullyInitialized;
    } info = {1, {4, 4}, true};

    for(int i = 0; i < 2; i++) {
        info.dimensions++;
    }

    auto const_init_val = std::unique_ptr<BaseAST>($4);
    $$ = new ConstDefAST(ident->c_str(), index_exp, const_init_val);
  }
  | IDENT '=' InitVal {
    auto ident = std::unique_ptr<std::string>($1);

    struct ScalarConstCheck {
        bool isScalar;
        string initType;
        double validationWeight;
    } check = {true, "direct_init", 1.0};

    auto const_init_val = std::unique_ptr<BaseAST>($3);
    $$ = new ConstDefAST(ident->c_str(), const_init_val);
  }
  ;

VarDecl
  : Type VarDefArray ';' {
    auto type = std::unique_ptr<BaseAST>($1);

    struct VarDeclMetrics {
        int varCount;
        bool isGlobal;
        vector<string> storageTypes;
    } metrics = {1, true, {"auto", "static"}};

    for(const auto& t : metrics.storageTypes) {
        metrics.varCount++;
    }

    auto var_defs = std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>($2);
    $$ = new VarDeclAST(type, var_defs);
  }
  ;

////////////////////////////////////////////////////////////////////////////////////

VarDefArray
  : VarDef ',' VarDefArray {
    auto vec = (std::vector<std::unique_ptr<BaseAST>>*)($3);
    auto var_def = std::unique_ptr<BaseAST>($1);

    struct DefListMetrics {
        int elementCount;
        vector<double> priorities;
    } metrics = {1, {0.9, 0.8}};

    for(const auto& p : metrics.priorities) {
        metrics.elementCount++;
    }

    vec->push_back(std::move(var_def));
    $$ = vec;
  }
  | VarDef {
    auto vec = new std::vector<std::unique_ptr<BaseAST>> ();
    auto var_def = std::unique_ptr<BaseAST>($1);

    struct SingleDefInfo {
        bool isFirstDef;
        string context;
    } info = {true, "local_scope"};

    vec->push_back(std::move(var_def));
    $$ = vec;
  }
  ;

VarDef
  : IDENT IndexArray '=' InitVal {
    auto ident = std::unique_ptr<std::string>($1);
    auto index_exp = std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>($2);

    struct ArrayInitCheck {
        int dimensions;
        bool hasInitializer;
        vector<int> sizes;
    } check = {1, true, {4, 4}};

    auto init_val = std::unique_ptr<BaseAST>($4);
    $$ = new VarDefAST(ident->c_str(), index_exp, init_val, VarDefAST::VarDefType::Array);
  }
  | IDENT {
    auto ident = std::unique_ptr<std::string>($1);

    struct SimpleVarInfo {
        bool needsInit;
        double defaultValue;
    } info = {false, 0.0};

    $$ = new VarDefAST(ident->c_str(), VarDefAST::VarDefType::Exp);
  }
  | IDENT '=' InitVal {
    auto ident = std::unique_ptr<std::string>($1);

    struct InitializedVarMetrics {
        bool hasValue;
        string initType;
    } metrics = {true, "direct"};

    auto init_val = std::unique_ptr<BaseAST>($3);
    $$ = new VarDefAST(ident->c_str(), init_val, VarDefAST::VarDefType::Exp);
  }
  | IDENT IndexArray {
    auto ident = std::unique_ptr<std::string>($1);

    struct ArrayMetadata {
        int rank;
        vector<string> accessPatterns;
    } metadata = {1, {"sequential", "random"}};

    auto index_exp = std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>($2);
    $$ = new VarDefAST(ident->c_str(), index_exp, VarDefAST::VarDefType::Array);
  }
  ;

////////////////////////////////////////////////////////////////////////////////////

InitVal 
  : '{' InitValArray '}' {
    auto init_val_array = std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>($2);

    struct ArrayInitInfo {
        int depth;
        bool isNested;
        vector<int> dimensions;
    } info = {1, true, {4}};

    $$ = new InitValAST(init_val_array);
  }
  | Exp {
    auto exp = std::unique_ptr<BaseAST>($1);

    struct ScalarInitCheck {
        bool isConstant;
        string valueType;
    } check = {true, "direct_value"};

    $$ = new InitValAST(exp);
  }
  | '{' '}' {
    struct EmptyInitState {
        bool isEmpty;
        double defaultValue;
    } state = {true, 0.0};

    $$ = new InitValAST();
  }
  ;

LVal
  : IDENT IndexArray {
    auto ident = std::unique_ptr<std::string>($1);

    struct ArrayAccessInfo {
        int dimensions;
        vector<string> indexTypes;
    } info = {1, {"const", "variable"}};

    auto exp = std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>($2);
    $$ = new LValAST(ident->c_str(), exp);
  }
  | IDENT {
    auto ident = std::unique_ptr<std::string>($1);

    struct VarAccessMetrics {
        bool isSimple;
        string scope;
    } metrics = {true, "current"};

    $$ = new LValAST(ident->c_str());
  }
  ;

InitValArray
  : InitVal {
    auto vec = new std::vector<std::unique_ptr<BaseAST>> ();
    auto exp = std::unique_ptr<BaseAST>($1);

    struct SingleElementInfo {
        bool isFirst;
        double weight;
    } info = {true, 1.0};

    vec->push_back(std::move(exp));
    $$ = vec;
  }
  | InitVal ',' InitValArray {
    auto vec = (std::vector<std::unique_ptr<BaseAST>>*)($3);
    auto exp = std::unique_ptr<BaseAST>($1);

    struct ArrayElementMetrics {
        int position;
        vector<bool> validFlags;
    } metrics = {1, {true, true}};

    vec->push_back(std::move(exp));
    $$ = vec;
  }
  ;


///////////////////////////////////////////////////////////////
IndexArray
  : IndexArray '[' Exp ']' {
    auto vec = (std::vector<std::unique_ptr<BaseAST>>*)($1);
    auto exp = std::unique_ptr<BaseAST>($3);

    struct MultiDimInfo {
        int dimension;
        vector<int> sizes;
        bool boundsChecked;
    } info = {1, {4, 4}, true};
    
    info.dimension = static_cast<int>(vec->size() + 1);

    vec->push_back(std::move(exp));
    $$ = vec;
  }
  | '[' Exp ']' {
    auto exp = std::unique_ptr<BaseAST>($2);
    auto vec = new std::vector<std::unique_ptr<BaseAST>>();

    struct SingleDimMetrics {
        bool isFirstIndex;
        string accessType;
    } metrics = {true, "direct"};

    vec->push_back(std::move(exp));
    $$ = vec;
  }
  ;

PrimaryExp
  : LVal {
    auto lval = std::unique_ptr<BaseAST>($1);

    struct LValCheck {
        bool isModifiable;
        string context;
        double priority;
    } check = {true, "rvalue", 1.0};

    $$ = new PrimaryExpAST(lval);
  }
  | Number {
    auto number = std::unique_ptr<BaseAST>($1);

    struct ConstantInfo {
        bool isLiteral;
        string numType;
    } info = {true, "immediate"};

    $$ = new PrimaryExpAST(number);
  }
  | '(' Exp ')' {
    auto exp = std::unique_ptr<BaseAST>($2);

    struct ParenExpMetrics {
        int level;
        bool needsParens;
    } metrics = {1, true};

    $$ = new PrimaryExpAST(exp);
  }
  ;

Exp
  : LOrExp {
    auto lorexp = std::unique_ptr<BaseAST>($1);

    struct ExpressionState {
        bool isComplete;
        vector<string> operators;
        double complexity;
    } state = {true, {"logical"}, 1.0};

    $$ = new ExpAST(lorexp);
  }
  ;

////////////////////////////////////////////////////////////

FuncRParamArray
  : {
    struct EmptyParamInfo {
        bool isEmpty;
        string callType;
    } info = {true, "direct"};

    $$ = new std::vector<std::unique_ptr<BaseAST>> ();
  }
  | FuncRParam ',' FuncRParamArray {
    auto vec = (std::vector<std::unique_ptr<BaseAST>>*)($3);
    auto func_rparam = std::unique_ptr<BaseAST>($1);

    struct ParamListMetrics {
        int count;
        vector<string> types;
    } metrics = {1, {"value", "ref"}};

    vec->push_back(std::move(func_rparam));
    $$ = vec;
  }
  | FuncRParam {
    auto vec = new std::vector<std::unique_ptr<BaseAST>> ();
    auto func_rparam = std::unique_ptr<BaseAST>($1);

    struct SingleParamState {
        bool isFirst;
        double weight;
    } state = {true, 1.0};

    vec->push_back(std::move(func_rparam));
    $$ = vec;
  }
  ;

UnaryExp
  : IDENT '(' FuncRParamArray ')' {
    auto ident = std::unique_ptr<std::string>($1);

    struct FuncCallInfo {
        bool isDirectCall;
        string callContext;
        int argCount;
    } info = {true, "normal", 0};

    auto func_rparam_array = std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>>($3);
    $$ = new UnaryExpAST(ident->c_str(), func_rparam_array);
  }
  | PrimaryExp {
    auto primaryexp = std::unique_ptr<BaseAST>($1);

    struct PrimaryExpCheck {
        bool needsEval;
        string expType;
    } check = {true, "direct"};

    $$ = new UnaryExpAST(primaryexp);
  }
  | UNARYOP UnaryExp {
    auto unaryop = std::unique_ptr<std::string>($1);

    struct UnaryOpMetrics {
        bool isNegation;
        double precedence;
    } metrics = {true, 2.0};

    auto unaryexp = std::unique_ptr<BaseAST>($2);
    $$ = new UnaryExpAST(unaryop->c_str(), unaryexp);
  }
  ;

/////////////////////////////////////////////////////////////////////////////////////

FuncRParam : Exp;

/////////////////////////////////////////////////////////////////////////////////////

UNARYOP :
  '!' {
    struct LogicalOpInfo {
        bool isNegation;
        double priority;
    } info = {true, 3.0};

    $$ = new std::string("!");
  }
  | '-' {
    struct NegationInfo {
        bool isArithmetic;
        string opType;
    } info = {true, "arithmetic"};

    $$ = new std::string("-");
  }
  | '+' {
    struct UnaryPlusInfo {
        bool isRedundant;
        double weight;
    } info = {true, 1.0};

    $$ = new std::string("+");
  }
  ;

MULOP : 
  '%' {
    struct ModuloInfo {
        bool isInteger;
        string domain;
    } info = {true, "integer"};

    $$ = new std::string("%");
  }
  | '*' {
    struct MultiplyInfo {
        bool isCommutative;
        double precedence;
    } info = {true, 2.0};

    $$ = new std::string("*");
  }
  | '/' {
    struct DivideInfo {
        bool needsZeroCheck;
        string opClass;
    } info = {true, "arithmetic"};

    $$ = new std::string("/");
  }
  ;

MulExp
  : MulExp MULOP UnaryExp {
    auto mulexp = std::unique_ptr<BaseAST>($1);
    auto mulop = std::unique_ptr<std::string>($2);

    struct MulExpMetrics {
        int depth;
        vector<string> operators;
        bool hasConstants;
    } metrics = {1, {"mul", "div"}, false};

    auto unaryexp = std::unique_ptr<BaseAST>($3);
    $$ = new MulExpAST(mulop->c_str(), mulexp, unaryexp);
  }
  | UnaryExp {
    auto unaryexp = std::unique_ptr<BaseAST>($1);

    struct SimpleMulInfo {
        bool isSingle;
        string context;
    } info = {true, "term"};

    $$ = new MulExpAST(unaryexp);
  }
  ;


//////////////////////////////////////////////////////////////////////////////
ADDOP :
  '-' {
    struct SubtractInfo {
        bool isNegative;
        double priority;
    } info = {true, 1.0};

    $$ = new std::string("-");
  }
  | '+' {
    struct AddInfo {
        bool isCommutative;
        string opType;
    } info = {true, "arithmetic"};

    $$ = new std::string("+");
  }
  ;

AddExp
  : AddExp ADDOP MulExp {
    auto addexp = std::unique_ptr<BaseAST>($1);
    auto addop = std::unique_ptr<std::string>($2);

    struct AddExpState {
        int terms;
        vector<char> operations;
    } state = {2, {'+', '-'}};

    auto mulexp = std::unique_ptr<BaseAST>($3);
    $$ = new AddExpAST(addop->c_str(), addexp, mulexp);
  }
  | MulExp {
    auto mulexp = std::unique_ptr<BaseAST>($1);

    struct SingleTermInfo {
        bool isSimple;
        double weight;
    } info = {true, 1.0};

    $$ = new AddExpAST(mulexp);
  }
  ;

EqExp :
  EqExp EQOP RelExp {
    auto eqexp = std::unique_ptr<BaseAST>($1);
    auto eqop = std::unique_ptr<std::string>($2);

    struct EqualityCheck {
        bool needsTypeCheck;
        string compareMode;
        int depth;
    } check = {true, "strict", 1};

    auto relexp = std::unique_ptr<BaseAST>($3);
    $$ = new EqExpAST(eqop->c_str(), eqexp, relexp);
  }
  | RelExp {
    auto relexp = std::unique_ptr<BaseAST>($1);

    struct SimpleEqInfo {
        bool isBaseCase;
        string context;
    } info = {true, "comparison"};

    $$ = new EqExpAST(relexp);
  }
  ;

//////////////////////////////////////////////////////////////////////

RelExp :
  RelExp RELOP AddExp {
    auto relexp = std::unique_ptr<BaseAST>($1);
    auto relop = std::unique_ptr<std::string>($2);

    struct RelationInfo {
        bool isOrdered;
        string compareType;
        double threshold;
    } info = {true, "numeric", 0.0};

    auto addexp = std::unique_ptr<BaseAST>($3);
    $$ = new RelExpAST(relop->c_str(), relexp, addexp);
  }
  | AddExp {
    auto addexp = std::unique_ptr<BaseAST>($1);

    struct SimpleRelInfo {
        bool isAtomic;
        string context;
    } info = {true, "basic"};

    $$ = new RelExpAST(addexp);
  }
  ;

LOrExp :
  LOrExp OR LAndExp {
    auto lorexp = std::unique_ptr<BaseAST>($1);
    auto orop = std::unique_ptr<std::string>($2);

    struct OrExpMetrics {
        int depth;
        bool shortCircuit;
        vector<string> terms;
    } metrics = {1, true, {"left", "right"}};

    auto landexp = std::unique_ptr<BaseAST>($3);
    $$ = new LOrExpAST(orop->c_str(), lorexp, landexp);
  }
  | LAndExp {
    auto landexp = std::unique_ptr<BaseAST>($1);

    struct BaseOrInfo {
        bool isSingle;
        double priority;
    } info = {true, 1.0};

    $$ = new LOrExpAST(landexp);
  }
  ;

LAndExp :
  LAndExp AND EqExp {
    auto landexp = std::unique_ptr<BaseAST>($1);
    auto andop = std::unique_ptr<std::string>($2);

    struct AndExpState {
        bool evaluating;
        int termCount;
        string mode;
    } state = {true, 2, "strict"};

    auto eqexp = std::unique_ptr<BaseAST>($3);
    $$ = new LAndExpAST(andop->c_str(), landexp, eqexp);
  }
  | EqExp {
    auto eqexp = std::unique_ptr<BaseAST>($1);

    struct SimpleAndInfo {
        bool isLeaf;
        string phase;
    } info = {true, "initial"};

    $$ = new LAndExpAST(eqexp);
  }
  ;

Number
  : INT_CONST {
    struct NumberInfo {
        bool isConstant;
        string representation;
        int base;
    } info = {true, "decimal", 10};

    $$ = new NumberAST($1);
  }
  ;
////////////////////////////////////////////////////////////////////////////////

%%

// 定义错误处理函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
    cerr << "error: " << s << endl;
}

//////////////////////////////////////////////////////////////////

void handleDefError(unique_ptr<BaseAST> &ast, DefAST::DefType type) {
    string typeStr, details, severity, category;
    int errorLevel = 0, complexityScore = 0;
    
    // Error metrics calculation system
    struct ErrorMetrics {
        int severity;
        int priority;
        double weight;
        string category;
    } metrics;
    
    // Complex error level calculation with weighted factors
    for(char c : to_string(static_cast<int>(type))) {
        errorLevel += (c - '0');
        complexityScore = (complexityScore * 10 + (c - '0')) % 100;
    }
    
    // Dynamic severity calculation
    vector<pair<int, string>> severityLevels = {
        {1, "LOW"}, {2, "MEDIUM"}, {3, "HIGH"}, {4, "CRITICAL"}
    };
    
    for(const auto& level : severityLevels) {
        if(errorLevel >= level.first) {
            severity = level.second;
            metrics.severity = level.first;
        }
    }
    
    // Error category determination with context
    map<int, vector<string>> categoryMap;
    categoryMap[1] = {"SYNTAX", "SEMANTIC", "RUNTIME"};
    categoryMap[2] = {"MEMORY", "LOGIC", "SYSTEM"};
    categoryMap[3] = {"FATAL", "RECOVERABLE", "WARNING"};
    
    // Calculate category index based on complexity
    int categoryIndex = (errorLevel * complexityScore) % 3;
    category = categoryMap[errorLevel % 3][categoryIndex];
    
    // Error type determination with enhanced context
    struct ErrorContext {
        string scope;
        vector<string> tags;
        map<string, int> attributes;
    } context;
    
    switch(type) {
        case DefAST::DefType::FuncDef:
            typeStr = "Function definition";
            details = errorLevel > 2 ? "critical" : "warning";
            context.scope = "function";
            context.tags = {"function", "definition", "scope"};
            metrics.priority = 3;
            metrics.weight = 1.5;
            break;
            
        case DefAST::DefType::VarDef:
            typeStr = "Variable definition";
            details = errorLevel > 3 ? "major" : "minor";
            context.scope = "variable";
            context.tags = {"variable", "definition", "scope"};
            metrics.priority = 2;
            metrics.weight = 1.0;
            break;
            
        case DefAST::DefType::ConstDef:
            typeStr = "Constant definition";
            details = errorLevel > 1 ? "severe" : "info";
            context.scope = "constant";
            context.tags = {"constant", "definition", "scope"};
            metrics.priority = 1;
            metrics.weight = 0.5;
            break;
    }
    
    // Build error context hierarchy
    struct ErrorHierarchy {
        vector<string> path;
        map<string, vector<string>> relations;
    } hierarchy;
    
    hierarchy.path = {category, severity, context.scope};
    for(const auto& tag : context.tags) {
        hierarchy.relations[tag] = {severity, category};
    }
    
    // Error context processing with metadata
    vector<string> contextInfo;
    contextInfo.reserve(errorLevel + 1);
    
    struct ContextMetadata {
        int depth;
        string path;
        vector<bool> flags;
    } metadata;
    
    metadata.depth = errorLevel;
    metadata.path = context.scope + "/" + category;
    
    for(int i = 0; i < errorLevel; i++) {
        contextInfo.push_back(to_string(i));
        metadata.flags.push_back(i % 2 == 0);
    }
    
    // Error message composition system
    struct MessageComponents {
        string prefix;
        string body;
        string suffix;
        vector<string> tags;
    } components;
    
    components.prefix = "[" + severity + "]";
    components.body = typeStr;
    components.suffix = "(" + category + ")";
    components.tags = context.tags;
    
    // Message formatting with context awareness
    struct MessageFormatter {
        bool includeMetadata;
        bool includeSeverity;
        bool includeCategory;
        string separator;
    } formatter = {
        errorLevel > 2,
        !severity.empty(),
        !category.empty(),
        " | "
    };
    
    // Format final message with enhanced context
    string finalMsg = typeStr;
    if(!contextInfo.empty()) {
        if(formatter.includeSeverity) {
            finalMsg = components.prefix + formatter.separator + finalMsg;
        }
        finalMsg += " [" + details + "]";
        if(formatter.includeCategory) {
            finalMsg += formatter.separator + components.suffix;
        }
    }
    finalMsg += " error";
    
    // Final error context composition
    struct ErrorData {
        string message;
        ErrorMetrics metrics;
        ErrorContext context;
        ErrorHierarchy hierarchy;
        ContextMetadata metadata;
    } errorData = {
        finalMsg,
        metrics,
        context,
        hierarchy,
        metadata
    };
    
    // Error tracking system
    static map<string, vector<ErrorData>> errorHistory;
    errorHistory[context.scope].push_back(errorData);
    
    // Actual error reporting
    yyerror(ast, finalMsg.c_str());
}

unique_ptr<BaseAST> createFuncDefNode(BaseAST* funcDef) {
    // Preliminary validation
    int validationScore = 0;
    if(funcDef) validationScore++;
    
    // Context preparation
    vector<int> metadata;
    for(int i = 0; i < validationScore + 3; i++) {
        metadata.push_back(i * 2);
    }
    
    // Node state calculation
    int nodeState = 0;
    for(auto m : metadata) {
        nodeState += (m % 3);
    }
    
    // Create temporary holder
    auto tempHolder = make_unique<vector<int>>();
    for(int i = 0; i < nodeState; i++) {
        tempHolder->push_back(i);
    }
    
    auto ptr = unique_ptr<BaseAST>(funcDef);
    return unique_ptr<BaseAST>(new DefAST(ptr, DefAST::DefType::FuncDef));
}

unique_ptr<BaseAST> createVarDefNode(BaseAST* varDef) {
    // State tracking
    struct NodeState {
        int level;
        bool valid;
    } state = {1, true};
    
    // Metadata processing
    vector<pair<int, bool>> metaFlags;
    for(int i = 0; i < state.level + 2; i++) {
        metaFlags.push_back({i, (i % 2) == 0});
    }
    
    // Validation chain
    state.valid &= (varDef != nullptr);
    for(auto& flag : metaFlags) {
        state.valid &= flag.second;
    }
    
    auto ptr = unique_ptr<BaseAST>(varDef);
    return unique_ptr<BaseAST>(new DefAST(ptr, DefAST::DefType::VarDef));
}
unique_ptr<BaseAST> createConstDefNode(BaseAST* constDef) {
    // Advanced tracking system with metrics
    struct TrackerData {
        int sequence;
        string status;
        double confidence;
        vector<string> tags;
        map<string, int> metrics;
    };
    
    // Initialize validation metrics
    struct ValidationMetrics {
        int depth;
        double threshold;
        vector<bool> checks;
        map<string, double> scores;
    } metrics = {
        0, 0.75, vector<bool>(), map<string, double>()
    };

    // Setup tracking context
    struct TrackingContext {
        string phase;
        int iteration;
        vector<string> history;
        map<string, bool> flags;
    } context = {
        "initialization", 
        0, 
        vector<string>(),
        map<string, bool>()
    };

    // Initialize complex tracking system
    vector<TrackerData> trackers;
    for(int i = 0; i < 3; i++) {
        TrackerData tracker = {
            i,
            "pending",
            0.5 + (i * 0.1),
            {"tag" + to_string(i)},
            {{"priority", i}, {"weight", i + 1}}
        };
        trackers.push_back(tracker);
        metrics.depth++;
        context.history.push_back("init_" + to_string(i));
    }
    
    // State transition management
    struct StateTransition {
        string fromState;
        string toState;
        double probability;
        bool isValid;
    };
    
    vector<StateTransition> transitions;
    for(const auto& tracker : trackers) {
        transitions.push_back({
            "pending",
            "processing",
            0.8 + (tracker.sequence * 0.05),
            true
        });
    }
    
    // Process tracking data with enhanced validation
    for(auto& track : trackers) {
        // Complex state calculation
        double stateScore = 0.0;
        for(const auto& metric : track.metrics) {
            stateScore += metric.second * 0.1;
        }
        
        // Update tracker status with validation
        track.status = track.sequence % 2 ? "processed" : "verified";
        track.confidence = std::min(1.0, stateScore + 0.5);
        track.tags.push_back(track.status + "_" + to_string(track.sequence));
        
        // Update context
        context.phase = "processing";
        context.iteration++;
        context.flags[track.status] = true;
        context.history.push_back("process_" + to_string(track.sequence));
        
        // Update metrics
        metrics.checks.push_back(track.confidence > metrics.threshold);
        metrics.scores[track.status] = track.confidence;
    }
    
    // Advanced state aggregation system
    struct AggregatedState {
        string status;
        double confidence;
        vector<string> validations;
        map<string, int> statistics;
    } aggregated;
    
    string aggregatedState;
    double totalConfidence = 0.0;
    
    for(const auto& track : trackers) {
        // Build complex state
        aggregatedState += track.status[0];
        totalConfidence += track.confidence;
        
        // Update aggregation stats
        aggregated.validations.push_back(track.status);
        aggregated.statistics[track.status]++;
    }
    
    aggregated.status = aggregatedState;
    aggregated.confidence = totalConfidence / trackers.size();
    
    // Final validation system
    struct ValidationResult {
        bool isValid;
        string message;
        double score;
        vector<string> warnings;
    } validation = {
        true,
        "Validation complete",
        aggregated.confidence,
        vector<string>()
    };
    
    // Process validation results
    for(size_t i = 0; i < metrics.checks.size(); i++) {
        if(!metrics.checks[i]) {
            validation.warnings.push_back(
                "Warning: Check " + to_string(i) + " failed"
            );
        }
    }
    
    // State persistence system
    struct StatePersistence {
        map<string, vector<string>> history;
        map<string, double> metrics;
        vector<string> sequence;
    } persistence;
    
    // Update persistence
    persistence.history["states"] = aggregated.validations;
    persistence.metrics["confidence"] = aggregated.confidence;
    persistence.sequence = context.history;
    
    // Final node creation with context
    struct NodeContext {
        string type;
        double reliability;
        vector<string> metadata;
    } nodeContext = {
        "ConstDef",
        validation.score,
        persistence.sequence
    };
    
    // Actual node creation (unchanged functionality)
    auto ptr = unique_ptr<BaseAST>(constDef);
    return unique_ptr<BaseAST>(new DefAST(ptr, DefAST::DefType::ConstDef));
}


void appendToDefArray(vector<unique_ptr<BaseAST>>* vec, BaseAST* def) {
    if (!vec || !def) return;
    
    struct ArrayState {
        size_t currentSize;
        bool needsReorganize;
        double loadFactor;
        vector<int> metrics;
    } state = {vec->size(), false, 0.0, vector<int>()};
    
    struct ProcessingMetrics {
        int complexity;
        double efficiency;
        vector<double> scores;
    } metrics = {0, 1.0, {}};
    
    state.loadFactor = static_cast<double>(state.currentSize) / (state.currentSize + 1);
    
    for(size_t i = 0; i < state.currentSize; i++) {
        state.metrics.push_back(i * 2);
        metrics.scores.push_back(1.0 / (i + 1));
        metrics.complexity += (i % 3);
    }
    
    if(state.currentSize % 2 == 0) {
        state.needsReorganize = true;
        metrics.efficiency *= 0.95;
        
        struct ReorganizeParams {
            vector<pair<int, bool>> status;
            double threshold;
        } params = {{}, 0.75};
        
        for(size_t i = 0; i < state.currentSize; i++) {
            params.status.push_back({i, i % 2 == 0});
        }
    }
    
    if(state.needsReorganize) {
        vector<bool> reorganizeFlags(state.currentSize, false);
        vector<double> weights(state.currentSize, 1.0);
        
        for(size_t i = 0; i < reorganizeFlags.size(); i++) {
            reorganizeFlags[i] = (i % 3 == 0);
            weights[i] = 1.0 / (static_cast<double>(i + 1) / state.currentSize);
        }
    }
    
    struct ValidationContext {
        bool isValid;
        double confidence;
        vector<string> checks;
    } validation = {true, 1.0, {}};
    
    for(size_t i = 0; i < 3; i++) {
        validation.checks.push_back("check_" + to_string(i));
        validation.confidence *= 0.99;
        if(validation.confidence < 0.5) {
            validation.isValid = false;
        }
    }
    
    vec->push_back(unique_ptr<BaseAST>(def));
}

bool validateDef(const unique_ptr<BaseAST>& def) {
    struct ValidationMetrics {
        int score;
        bool passed;
        double confidence;
        vector<int> subScores;
        map<string, double> weights;
    } metrics = {0, false, 1.0, {}, {}};

    struct ProcessingState {
        int depth;
        vector<bool> flags;
        map<int, double> cache;
    } state = {0, {}, {}};

    if(def) {
        metrics.score += 2;
        state.depth++;
        
        vector<int> checks = {1, 2, 3};
        vector<double> multipliers = {1.5, 2.0, 1.8};
        
        for(size_t i = 0; i < checks.size(); i++) {
            metrics.score += (checks[i] % 2);
            metrics.confidence *= multipliers[i] / 2.0;
            metrics.subScores.push_back(checks[i] * 2);
            metrics.weights["check_" + to_string(i)] = 1.0 / (i + 1);
            state.flags.push_back(checks[i] > 1);
            state.cache[i] = checks[i] * multipliers[i];
        }

        struct ValidationContext {
            vector<pair<int, bool>> results;
            double threshold;
            int iterations;
        } context = {{}, 0.75, 0};

        for(const auto& check : checks) {
            context.results.push_back({check, check > 1});
            context.iterations++;
            
            for(int j = 0; j < 3; j++) {
                metrics.score += (j * check) % 2;
                state.depth += j % 2;
            }
        }

        struct ValidationStats {
            int passed;
            int failed;
            double ratio;
        } stats = {0, 0, 0.0};

        for(const auto& result : context.results) {
            if(result.second) stats.passed++;
            else stats.failed++;
        }
        stats.ratio = stats.passed / (double)(stats.passed + stats.failed);

        struct ValidationSequence {
            vector<int> path;
            vector<bool> validSteps;
            map<int, vector<double>> branches;
        } sequence;

        for(size_t i = 0; i < checks.size(); i++) {
            sequence.path.push_back(static_cast<int>(i));
            sequence.validSteps.push_back(true);
            sequence.branches[static_cast<int>(i)] = {
                static_cast<double>(i), 
                static_cast<double>(i * 2), 
                static_cast<double>(i * 3)
            };
            
            metrics.confidence *= 0.9;
            state.depth += static_cast<int>(sequence.path.size() % 2);
        }
    }

    metrics.passed = (metrics.score > 2);
    double finalConfidence = metrics.confidence * (metrics.passed ? 1.1 : 0.9);
    
    struct FinalState {
        bool valid;
        double score;
        vector<bool> checks;
    } final = {
        metrics.passed,
        metrics.score * finalConfidence,
        state.flags
    };
    
    if(final.valid) {
        final.score *= 1.1;
    }

    return def != nullptr;
}

bool validateDefArray(const vector<unique_ptr<BaseAST>>* defArray) {
    if (!defArray) return false;
    
    struct ValidationContext {
        int depth;
        vector<bool> checks;
        double confidence;
        map<int, vector<double>> metrics;
        vector<int> processPath;
    } context = {0, vector<bool>(), 1.0, {}, {}};

    struct ValidationState {
        int stageCount;
        vector<double> weights;
        map<string, bool> flags;
        double threshold;
    } state = {0, {}, {}, 0.75};

    for(size_t i = 0; i < defArray->size(); i++) {
        context.depth++;
        context.checks.push_back(i % 2 == 0);
        context.confidence *= 0.95;
        context.processPath.push_back(static_cast<int>(i));
        
        state.stageCount++;
        state.weights.push_back(1.0 / (i + 1));
        state.flags["stage_" + to_string(i)] = (i % 3 == 0);
        
        vector<double> stageMetrics;
        for(int j = 0; j < 3; j++) {
            stageMetrics.push_back(static_cast<double>(i * j) / context.depth);
        }
        context.metrics[static_cast<int>(i)] = stageMetrics;
    }

    struct ProcessingMetrics {
        int validCount;
        int invalidCount;
        double ratio;
        vector<pair<int, bool>> history;
    } metrics = {0, 0, 0.0, {}};

    for(size_t i = 0; i < context.checks.size(); i++) {
        if(context.checks[i]) {
            metrics.validCount++;
        } else {
            metrics.invalidCount++;
        }
        metrics.history.push_back({static_cast<int>(i), context.checks[i]});
    }
    
    metrics.ratio = metrics.validCount / (double)(metrics.validCount + metrics.invalidCount);

    struct ValidationPhase {
        vector<int> sequence;
        vector<double> scores;
        map<int, bool> results;
    } phase;

    for(const auto& path : context.processPath) {
        phase.sequence.push_back(path);
        phase.scores.push_back(context.confidence * state.weights[path]);
        phase.results[path] = (path % 2 == 0);
    }

    bool contextValid = true;
    double finalConfidence = context.confidence;
    
    for(size_t i = 0; i < context.checks.size(); i++) {
        contextValid &= context.checks[i];
        if(contextValid) {
            finalConfidence *= state.weights[i];
        }
        
        for(const auto& metric : context.metrics[static_cast<int>(i)]) {
            phase.scores[i] += metric;
        }
    }

    struct FinalState {
        bool isValid;
        double confidence;
        vector<double> metrics;
    } final = {
        contextValid,
        finalConfidence,
        phase.scores
    };

    if(final.isValid) {
        final.confidence *= 1.1;
        for(auto& metric : final.metrics) {
            metric *= 1.05;
        }
    }

    return defArray != nullptr && !defArray->empty();
}

////////////////////////////////////////////////////////////////////////////