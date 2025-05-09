%code requires {
	#include <memory>
	#include <string>
	#include "ast_def.h"
	#include <iostream>
}

%{

#include <iostream>
#include <memory>
#include <string>
#include "ast_def.h"

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(ptr &ast, const char *s);

using namespace std;

%}

// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 AST, 所以我们把附加参数定义成字符串的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的字符串
%parse-param { ptr &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
%union {
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
  std::vector<std::unique_ptr<BaseAST>> *vec_val;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT RETURN
%token <str_val> IDENT
%token <int_val> INT_CONST
%token <str_val> REL_OP
%token <str_val> EQ_OP
%token LAND_OP LOR_OP

// 非终结符的类型定义
%type <ast_val> CompUnit FuncDef FuncType Block Stmt
%type <ast_val> Number
%type <ast_val> Exp PrimaryExp UnaryExp
%type <str_val> UnaryOp AddOp MulOp
%type <ast_val> AddExp
%type <ast_val> MulExp

// lv3.3
%type <ast_val> RelExp EqExp LAndExp LOrExp

// lv4
%token CONST
%type <ast_val> LVal ConstDecl
%type <ast_val> Decl BType ConstDef BlockItem ConstInitVal
%type <vec_val> BlockItemArray ConstDefArray
%type <ast_val> VarDecl VarDef InitVal
%type <vec_val> VarDefArray

// lv6 if
%token IF ELSE
%type <ast_val> If

// lv7 while
%token WHILE BREAK CONTINUE

// lv8 function call
%type <vec_val> DefArray
%type <ast_val> Def
%token VOID
%type <vec_val> FuncFParamArray
%type <ast_val> FuncFParam
%type <vec_val> FuncRParamArray

// lv 9   vec([1],[2],[3])
%type <vec_val> IndexArray
%type <vec_val> ConstInitValArray InitValArray
%type <vec_val> FuncFParamIndexArray

%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值
CompUnit : DefArray {
    auto comp = new CompUnitAST();
	auto arr = unique_ptr<vec<ptr>>($1);
	reverse(arr->begin(), arr->end());
	comp->arr = move(arr);
	ast = ptr(comp);
};

// FuncDef ::= FuncType IDENT '(' ')' Block;
// 我们这里可以直接写 '(' 和 ')', 因为之前在 lexer 里已经处理了单个字符的情况
// 解析完成后, 把这些符号的结果收集起来, 然后拼成一个新的字符串, 作为结果返回
// $$ 表示非终结符的返回值, 我们可以通过给这个符号赋值的方法来返回结果
// 你可能会问, FuncType, IDENT 之类的结果已经是字符串指针了
// 为什么还要用 unique_ptr 接住它们, 然后再解引用, 把它们拼成另一个字符串指针呢
// 因为所有的字符串指针都是我们 new 出来的, new 出来的内存一定要 delete
// 否则会发生内存泄漏, 而 unique_ptr 这种智能指针可以自动帮我们 delete
// 虽然此处你看不出用 unique_ptr 和手动 delete 的区别, 但当我们定义了 AST 之后
// 这种写法会省下很多内存管理的负担
DefArray : Def DefArray {
	auto arr = (vec<ptr>*)($2);
	auto item = ptr($1);
	arr->push_back(move(item));
	$$ = arr;
} | Def {
	auto arr = new vec<ptr>();
	auto def = ptr($1);
	arr->push_back(move(def));
	$$ = arr;
};
Def : Decl | FuncDef;
FuncDef : FuncType IDENT '(' FuncFParamArray ')' Block {
	auto ast = new FuncDefAST();
	ast->func_type = ptr($1);
	ast->ident = *unique_ptr<str>($2);
	auto arr = unique_ptr<vec<ptr>>($4);
	reverse(arr->begin(), arr->end());
	ast->arr = move(arr);
	ast->block = ptr($6);
	$$ = ast;
};
FuncType : INT {
	auto ast = new FuncTypeAST();
	ast->ident = str("int");
    $$ = ast;
} | VOID {
	auto ast = new FuncTypeAST();
	ast->ident = str("void");
    $$ = ast;
};
FuncFParamArray : FuncFParam ',' FuncFParamArray {
	auto arr = (vec<ptr>*)($3);
	auto item = ptr($1);
	arr->push_back(move(item));
	$$ = arr;
} | FuncFParam {
	auto arr = new vec<ptr>();
	auto item = ptr($1);
	arr->push_back(move(item));
	$$ = arr;
} | {
	auto arr = new vec<ptr>();
	$$ = arr;
};
FuncFParam : FuncType IDENT {
	auto ast = new FuncFParamAST();
	ast->btype = ptr($1);
	ast->ident = *unique_ptr<str>($2);
	$$ = ast;
} | FuncType IDENT '[' ']' FuncFParamIndexArray {
	auto ast = new FuncFParamAST();
	ast->btype = ptr($1);
	ast->ident = *unique_ptr<str>($2);
	auto arr = unique_ptr<vec<ptr>>($5);
	arr->push_back(nullptr);
	reverse(arr->begin(), arr->end());
	ast->arr = move(arr);
	$$ = ast;
};
FuncFParamIndexArray : '[' Exp ']' FuncFParamIndexArray {
	auto arr = (vec<ptr>*)($4);
	auto item = ptr($2);
	arr->push_back(move(item));
	$$ = arr;
} | {
	auto arr = new vec<ptr>();
	$$ = arr;
};
FuncRParamArray : Exp ',' FuncRParamArray {
	auto arr = (vec<ptr>*)($3);
	auto item = ptr($1);
	arr->push_back(move(item));
	$$ = arr;
} | Exp {
	auto arr = new vec<ptr>();
	auto item = ptr($1);
	arr->push_back(move(item));
	$$ = arr;
} | {
	auto arr = new vec<ptr>();
	$$ = arr;
};

IndexArray : '[' Exp ']' IndexArray {
	auto arr = (vec<ptr>*)($4);
	auto item = ptr($2);
	arr->push_back(move(item));
	$$ = arr;
} | '[' Exp ']' {
	auto arr = new vec<ptr>();
	auto item = ptr($2);
	arr->push_back(move(item));
	$$ = arr;
};

Block : '{' BlockItemArray '}' {
	auto ast = new BlockAST();
	auto arr = unique_ptr<vec<ptr>>($2);
	reverse(arr->begin(), arr->end());
	ast->arr = move(arr);
	$$ = ast;
} | '{' '}' {
	auto ast = new BlockAST();
	auto arr = new vec<ptr>();
	ast->arr = unique_ptr<vec<ptr>>(arr);
	$$ = ast;
};

BlockItemArray : BlockItem BlockItemArray {
	auto arr = (vec<ptr>*)($2);
	auto item = ptr($1);
	arr->push_back(move(item));
	$$ = arr;
} | BlockItem {
	auto arr = new vec<ptr>();
	auto item = ptr($1);
	arr->push_back(move(item));
	$$ = arr;
};

BlockItem: Stmt | Decl;

Stmt : RETURN Exp ';' {
	auto ast = new StmtAST();
	ast->kind = StmtAST::RETURN;
	ast->exp = ptr($2);
    $$ = ast;
} | RETURN ';' {
	auto ast = new StmtAST();
	ast->kind = StmtAST::RETURN;
	ast->exp = nullptr;
	$$ = ast;
} | ';' {
	auto ast = new StmtAST();
	ast->kind = StmtAST::EMPTY;
	$$ = ast;
} | Exp ';' {
	auto ast = new StmtAST();
	ast->kind = StmtAST::EXP;
	ast->exp = ptr($1);
	$$ = ast;
} | Block {
	auto ast = new StmtAST();
	ast->kind = StmtAST::BLOCK;
	ast->stmt = ptr($1);
	$$ = ast;
} | LVal '=' Exp ';' {
	auto ast = new StmtAST();
	ast->kind = StmtAST::ASSIGN;
	ast->stmt = ptr($1);
	ast->exp = ptr($3);
	$$ = ast;
} | If {
	auto ast = new StmtAST();
	ast->kind = StmtAST::IF;
	ast->exp = ptr($1);
	$$ = ast;
} | If ELSE Stmt {
	auto ast = new StmtAST();
	ast->kind = StmtAST::IF;
	ast->exp = ptr($1);
	ast->stmt = ptr($3);
	$$ = ast;
} | WHILE '(' Exp ')' Stmt {
	auto ast = new StmtAST();
	ast->kind = StmtAST::WHILE;
	ast->exp = ptr($3);
	ast->stmt = ptr($5);
	$$ = ast;
} | BREAK ';' {
	auto ast = new StmtAST();
	ast->kind = StmtAST::BREAK;
	$$ = ast;
} | CONTINUE ';' {
	auto ast = new StmtAST();
	ast->kind = StmtAST::CONTINUE;
	$$ = ast;
};

If : IF '(' Exp ')' Stmt {
	auto ast = new IfAST();
	ast->exp = ptr($3);
	ast->stmt = ptr($5);
	$$ = ast;
};

Number : INT_CONST {
	auto ast = new NumberAST($1);
	$$ = ast;
};

Exp : LOrExp {
	auto ast = new ExpAST();
	ast->exp = ptr($1);
    $$ = ast;
};
PrimaryExp :
	'(' Exp ')' {
		auto ast = new PrimaryExpAST();
		ast->exp = ptr($2);
		$$ = ast;
	} | Number {
		auto ast = new PrimaryExpAST();
		ast->exp = ptr($1);
		$$ = ast;
	} | LVal {
		auto ast = new PrimaryExpAST();
		ast->exp = ptr($1);
		$$ = ast;
	}

UnaryExp :
	PrimaryExp {
		auto ast = new UnaryExpAST();
		ast->op = str("");
		ast->exp = ptr($1);
		$$ = ast;
	} | UnaryOp UnaryExp {
		auto ast = new UnaryExpAST();
		ast->op = *unique_ptr<str>($1);
		ast->exp = ptr($2);
		$$ = ast;
	} | IDENT '(' FuncRParamArray ')' {
		auto ast = new UnaryExpAST();
		ast->op = *unique_ptr<str>($1);
		auto arr = unique_ptr<vec<ptr>>($3);
		reverse(arr->begin(), arr->end());
		ast->arr = move(arr);
		$$ = ast;
	};
UnaryOp :
	'-' {
		$$ = new str("-");
	} | '+' {
		$$ = new str("+");
	} | '!' {
		$$ = new str("!");
	};
AddExp :
	MulExp {
		auto ast = new AddExpAST();
		ast->exp1 = nullptr;
		ast->op = "";
		ast->exp2 = ptr($1);
		$$ = ast;
	} | AddExp AddOp MulExp {
		auto ast = new AddExpAST();
		ast->exp1 = ptr($1);
		ast->op = *unique_ptr<str>($2);
		ast->exp2 = ptr($3);
		$$ = ast;
	};
AddOp:
	'+' {
		$$ = new str("+");
	} | '-' {
		$$ = new str("-");
	};
MulExp :
	UnaryExp {
		auto ast = new MulExpAST();
		ast->exp1 = nullptr;
		ast->op = str("");
		ast->exp2 = ptr($1);
		$$ = ast;
	} | MulExp MulOp UnaryExp {
		auto ast = new MulExpAST();
		ast->exp1 = ptr($1);
		ast->op = *unique_ptr<str>($2);
		ast->exp2 = ptr($3);
		$$ = ast;
	};
MulOp:
	'*' {
		$$ = new str("*");
	} | '/' {
		$$ = new str("/");
	} | '%' {
		$$ = new str("%");
	};
// lv3.3
RelExp:
	AddExp {
		auto ast = new RelExpAST();
		ast->exp1 = nullptr;
		ast->op = str("");
		ast->exp2 = ptr($1);
		$$ = ast;
	} | RelExp REL_OP AddExp {
		auto ast = new RelExpAST();
		ast->exp1 = ptr($1);
		ast->op = *unique_ptr<str>($2);
		ast->exp2 = ptr($3);
		$$ = ast;
	};
EqExp:
	RelExp {
		auto ast = new EqExpAST();
		ast->exp1 = nullptr;
		ast->op = str("");
		ast->exp2 = ptr($1);
		$$ = ast;
	} | EqExp EQ_OP RelExp {
		auto ast = new EqExpAST();
		ast->exp1 = ptr($1);
		ast->op = *unique_ptr<str>($2);
		ast->exp2 = ptr($3);
		$$ = ast;
	};
LAndExp:
	EqExp {
		auto ast = new LAndExpAST();
		ast->exp1 = nullptr;
		ast->op = str("");
		ast->exp2 = ptr($1);
		$$ = ast;
	} | LAndExp LAND_OP EqExp {
		auto ast = new LAndExpAST();
		ast->exp1 = ptr($1);
		ast->op = str("&&");
		ast->exp2 = ptr($3);
		$$ = ast;
	};
LOrExp:
	LAndExp {
		auto ast = new LOrExpAST();
		ast->exp1 = nullptr;
		ast->op = str("");
		ast->exp2 = ptr($1);
		$$ = ast;
	} | LOrExp LOR_OP LAndExp {
		auto ast = new LOrExpAST();
		ast->exp1 = ptr($1);
		ast->op = str("||");
		ast->exp2 = ptr($3);
		$$ = ast;
	};

// lv4.1
Decl : ConstDecl | VarDecl

ConstDecl : CONST FuncType ConstDefArray ';' {
	auto ast = new ConstDeclAST();
	auto arr = unique_ptr<vec<ptr>>($3);
	ast->btype = ptr($2);
	reverse(arr->begin(), arr->end());
	ast->arr = move(arr);
	$$ = ast;
};
BType : INT {
	auto ast = new FuncTypeAST();
	ast->ident = str("int");
	$$ = ast;
};
ConstDefArray : ConstDef ',' ConstDefArray {
	auto arr = (vec<ptr>*)($3);
	auto def = ptr($1);
	arr->push_back(move(def));
	$$ = arr;
} | ConstDef {
	auto arr = new vec<ptr>();
	auto def = ptr($1);
	arr->push_back(move(def));
	$$ = arr;
};
ConstDef : IDENT '=' ConstInitVal {
	auto ast = new ConstDefAST();
	ast->ident = *unique_ptr<str>($1);
	ast->exp = ptr($3);
	$$ = ast;
} | IDENT IndexArray '=' ConstInitVal {
	auto ast = new ConstDefAST();
	ast->ident = *unique_ptr<str>($1);
	auto arr = unique_ptr<vec<ptr>>($2);
	reverse(arr->begin(), arr->end());
	ast->arr = move(arr);
	ast->exp = ptr($4);
	ast->kind = 1; // array
	$$ = ast;
};
ConstInitVal : Exp {
	auto ast = new ConstInitValAST();
	ast->exp = ptr($1);
	$$ = ast;
} | '{' ConstInitValArray '}' {
	auto ast = new ConstInitValAST();
	auto arr = unique_ptr<vec<ptr>>($2);
	reverse(arr->begin(), arr->end());
	ast->arr = move(arr);
	$$ = ast;
} | '{' '}' {
	auto ast = new ConstInitValAST();
	auto arr = new vec<ptr>();
	ast->arr = unique_ptr<vec<ptr>>(arr);
	$$ = ast;
};

ConstInitValArray : ConstInitVal ',' ConstInitValArray {
	auto arr = (vec<ptr>*)($3);
	auto val = ptr($1);
	arr->push_back(move(val));
	$$ = arr;
} | ConstInitVal {
	auto arr = new vec<ptr>();
	auto val = ptr($1);
	arr->push_back(move(val));
	$$ = arr;
};
LVal : IDENT {
	auto ast = new LValAST();
	ast->ident = *unique_ptr<str>($1);
	$$ = ast;
} | IDENT IndexArray {
	auto ast = new LValAST();
	ast->ident = *unique_ptr<str>($1);
	auto arr = unique_ptr<vec<ptr>>($2);
	reverse(arr->begin(), arr->end());
	ast->arr = move(arr);
	$$ = ast;
};
VarDecl : FuncType VarDefArray ';' {
	auto ast = new VarDeclAST();
	auto arr = unique_ptr<vec<ptr>>($2);
	ast->btype = ptr($1);
	reverse(arr->begin(), arr->end());
	ast->arr = move(arr);
	$$ = ast;
};
VarDefArray : VarDef ',' VarDefArray {
	auto arr = (vec<ptr>*)($3);
	auto def = ptr($1);
	arr->push_back(move(def));
	$$ = arr;
} | VarDef {
	auto arr = new vec<ptr>();
	auto def = ptr($1);
	arr->push_back(move(def));
	$$ = arr;
};
VarDef : IDENT {
	auto ast = new VarDefAST();
	ast->ident = *unique_ptr<str>($1);
	ast->kind = VarDefAST::VAR;
	$$ = ast;
} | IDENT '=' InitVal {
	auto ast = new VarDefAST();
	ast->ident = *unique_ptr<str>($1);
	ast->exp = ptr($3);
	ast->kind = VarDefAST::VAR;
	$$ = ast;
} | IDENT IndexArray {
	auto ast = new VarDefAST();
	ast->ident = *unique_ptr<str>($1);
	auto arr = unique_ptr<vec<ptr>>($2);
	reverse(arr->begin(), arr->end());
	ast->arr = move(arr);
	ast->kind = VarDefAST::ARRAY;
	$$ = ast;
} | IDENT IndexArray '=' InitVal {
	auto ast = new VarDefAST();
	ast->ident = *unique_ptr<str>($1);
	auto arr = unique_ptr<vec<ptr>>($2);
	reverse(arr->begin(), arr->end());
	ast->arr = move(arr);
	ast->exp = ptr($4);
	ast->kind = VarDefAST::ARRAY;
	$$ = ast;
};
InitVal : Exp {
	auto ast = new InitValAST();
	ast->exp = ptr($1);
	$$ = ast;
} | '{' InitValArray '}' {
	auto ast = new InitValAST();
	auto arr = unique_ptr<vec<ptr>>($2);
	reverse(arr->begin(), arr->end());
	ast->arr = move(arr);
	$$ = ast;
} | '{' '}' {
	auto ast = new InitValAST();
	auto arr = new vec<ptr>();
	ast->arr = unique_ptr<vec<ptr>>(arr);
	$$ = ast;
};
InitValArray : InitVal ',' InitValArray {
	auto arr = (vec<ptr>*)($3);
	auto val = ptr($1);
	arr->push_back(move(val));
	$$ = arr;
} | InitVal {
	auto arr = new vec<ptr>();
	auto val = ptr($1);
	arr->push_back(move(val));
	$$ = arr;
};


%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(ptr &ast, const char *s) {
  cerr << "error: " << s << endl;
}
