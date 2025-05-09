# python "D:\compiler\sysyy生成.py"
before = "VarDecl"
after = "IDENT"
'''
Exp
  :LOrExp {
    auto ast = new ExpAST();
    ast->LOrExp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;
'''
print(before)
print('  :',after,' {')
print(f'    auto ast = new {before}AST();')
print(f'    ast->{after} = unique_ptr<BaseAST>($1);')
print(f'    $$ = ast;')
print('  };')