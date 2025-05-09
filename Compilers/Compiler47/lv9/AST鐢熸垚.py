# python "D:\compiler\AST生成.py"
mylist = ["VarDecl","VarDef","InitVal"]
for name in mylist:
    #name = input()
    print("struct",name+"AST",': public BaseAST')
    print('{')
    print("std::unique_ptr<BaseAST> ;")
    print(" string dump() override")
    print(" {")
    print("  return \"\";")
    print(" }")
    print('};')

'''
struct PrimaryExpAST : public BaseAST
{
  string type;
  exp
}
'''