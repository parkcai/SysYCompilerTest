#pragma once
#include<memory>
#include<string>
#include<iostream>
#include<vector>
#include<map>
#include<cstdlib>
//符号表信息
static std::map<std::string,int> const_var[10000],if_const[10000],if_const_array[10000],if_array[10000],var[10000],if_var[10000],var_num[10000],if_uninit[10000],var_id;
static std::map<std::string,std::vector<int>> var_array[10000],const_array[10000],var_dim[10000],dim_psum[10000];

//函数相关信息
static std::map<std::string,std::string> functype;
static std::map<std::string,int> if_param, param_idx, if_array_param, func_var_dim;
static int depth = 0, if_int = 0;

//记录当前所处循环信息
static std::vector<int> w,d;
static int end_num = 0, then_num = 0, else_num = 0,while_num = 0,body_num = 0, dummy_num = 0, temp_num = 0;

//记录dump过程中的数组类型信息
static int dims = 0;

//用于给语句分配标号
static int zly = 0;

//
static int align_id = 0;

//是否正在计算参数
static bool calc_param = false;


// 所有 AST 的基类
class BaseAST {
 public:
  int idx,val,type;
  std::string ident;
  std::vector<std::unique_ptr<BaseAST>> vec,vec2;
  std::vector<int> vals;

  struct tval{
    int type;
    union{
      int idx;
      int val;
    }y;
  };

  std::vector<tval> tvals;

  virtual ~BaseAST() = default;
  virtual void Dump(FILE* fp, bool verbose) = 0;
  virtual void gen_type(FILE* fp) = 0;
  void print_ast(FILE* fp,const std::unique_ptr<BaseAST>& ast){
    if(ast->idx != -1){
      fprintf(fp,"%%%d",ast->idx);
    }
    else{
      fprintf(fp,"%d",ast->val);
    }
  }
  tval ret_tval(const std::unique_ptr<BaseAST>& ast){
    if(ast->idx != -1){
      tval temp;
      temp.type = 0;
      temp.y.idx = ast->idx;
      return temp;
    }
    else{
      tval temp;
      temp.type = 1;
      temp.y.val = ast->val;
      return temp;
    }
  }
};


// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST {
 public:
  // 用智能指针管理对象
  void gen_type(FILE* fp) override{}
  void Dump(FILE* fp, bool verbose) override{
    functype["getint"] = "int";
    functype["getch"] = "int";
    functype["getarray"] = "int";
    functype["putint"] = "void";
    functype["putch"] = "void";
    functype["putarray"] = "void";
    functype["starttime"] = "void";
    functype["stoptime"] = "void";
    fprintf(fp,"decl @getint(): i32\n");
    fprintf(fp,"decl @getch(): i32\n");
    fprintf(fp,"decl @getarray(*i32): i32\n");
    fprintf(fp,"decl @putint(i32)\n");
    fprintf(fp,"decl @putch(i32)\n");
    fprintf(fp,"decl @putarray(i32,*i32)\n");
    fprintf(fp,"decl @starttime()\n");
    fprintf(fp,"decl @stoptime()\n\n\n");

    for(auto ptr = vec2.begin();ptr != vec2.end(); ptr++){
        (*ptr)->Dump(fp,false);
        for(auto iptr = (*ptr)->vec.begin(); iptr != (*ptr)->vec.end(); iptr++){
          if (if_var[depth][(*iptr)->ident]){
            fprintf(fp,"global @%s_%d = alloc i32, %d\n",(*iptr)->ident.c_str(),var_num[depth][(*iptr)->ident],var[depth][(*iptr)->ident]);
          } 
          if (if_const_array[depth][(*iptr)->ident] || if_array[depth][(*iptr)->ident]){
            /*
            for(auto p = const_array[depth][(*iptr)->ident].begin(); p!=const_array[depth][(*iptr)->ident].end(); p++){
              printf("%d, ",*p);
            }
            printf("\n");
            */
            fprintf(fp,"global @%s_%d = alloc ",(*iptr)->ident.c_str(),var_num[depth][(*iptr)->ident]);
            int len = var_dim[depth][(*iptr)->ident].size();
            for(int i=0; i<len; i++){
              fprintf(fp,"[");
            }
            fprintf(fp,"i32");
            for(int i = len-1; i>=0; i--){
              fprintf(fp,", %d]", var_dim[depth][(*iptr)->ident][i]);
            }
            fprintf(fp,", ");
            if(if_uninit[depth][(*iptr)->ident]) {fprintf(fp,"zeroinit\n");}
            else{
              for(int i = 0; i < dim_psum[depth][(*iptr)->ident].back(); i++){
                int temp1 = -1, temp2 = -1;
                for(int j = 0; j < len; j++){
                  if(i % dim_psum[depth][(*iptr)->ident][j] == 0){
                    temp1 = j;
                  }
                  if((i+1) % dim_psum[depth][(*iptr)->ident][j] == 0){
                    temp2 = j;
                  }
                }
                if(i != 0) fprintf(fp,", ");
                for(int j = 0; j <= temp1; j++){
                  fprintf(fp, "{");
                }
                if(if_const_array[depth][(*iptr)->ident]) fprintf(fp, "%d", const_array[depth][(*iptr)->ident][i]);
                else fprintf(fp, "%d", var_array[depth][(*iptr)->ident][i]);
                for(int j = 0; j <= temp2; j++){
                  fprintf(fp, "}");
                }
              }
            }
            fprintf(fp,"\n");
          }
        }
    }
    for(auto ptr = vec.begin();ptr != vec.end(); ptr++){
        (*ptr)->Dump(fp,true);
    }
  }

};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> func_type;
  std::unique_ptr<BaseAST> block;
  std::unique_ptr<BaseAST> params;
  void gen_type(FILE* fp) override{}
  void Dump(FILE* fp, bool verbose) override{
    if_param.clear();
    param_idx.clear();
    if_array_param.clear();
    func_var_dim.clear();
    if(func_type->ident == "int") if_int = 1;
    else if_int = 0;
    functype[ident] = func_type->ident;
    params->ident = func_type->ident;
    fprintf(fp,"fun @%s(",ident.c_str());
    params->Dump(fp,true);
    block->Dump(fp,true);
    if(func_type->ident == "int") fprintf(fp,"  ret 0\n");
    else fprintf(fp,"  ret\n");
    fprintf(fp,"}\n");
  }
};

// FuncType 也可以是BaseAST
class FuncTypeAST : public BaseAST{
 public:
  void gen_type(FILE* fp) override{}
  void Dump(FILE* fp, bool verbose) override{
    if(ident == "void"){
      fprintf(fp," {\n");
    }
    else{
      fprintf(fp," i32 {\n");
    }
    return;
  }
};

class FuncParamsAST : public BaseAST{
 public:
  void gen_type(FILE* fp) override{}
  void Dump(FILE* fp, bool verbose) override{
    for (auto ptr = vec.begin(); ptr != vec.end(); ptr++){
      if (ptr != vec.begin()){
        fprintf(fp,", ");
      }
      fprintf(fp,"@%s_%d:",(*ptr)->ident.c_str(),var_id[(*ptr)->ident]++);
      (*ptr)->gen_type(fp);
    }
    if(ident == "int") fprintf(fp,"): i32 {\n");
    else fprintf(fp,") {\n");
    fprintf(fp,"%%entry:\n");
    fprintf(fp,"  @temp_%d = alloc i32\n",var_id["temp"]++);
    temp_num = var_id["temp"]-1;
    for (auto ptr = vec.begin(); ptr != vec.end(); ptr++){
      (*ptr)->Dump(fp,true);
    }
    return;
  }
};

class FuncParamAST : public BaseAST{
 public:
  bool if_array;
  std::unique_ptr<BaseAST> dim;
  void gen_type(FILE* fp) override{
    if(!if_array){
      fprintf(fp,"i32");
    }
    else{
      if(dim->vals.size() == 0){
        dim->Dump(fp,false);
      }
      int len = dim->vals.size();
      fprintf(fp, "*");
      for(int i = 0; i < len; i++){
          fprintf(fp, "[");
      }
      fprintf(fp, "i32");
      for(int i = 0; i < len; i++){
        fprintf(fp, ", %d]", dim->vals[len-1-i]);
      }
    }
  }
  void Dump(FILE* fp, bool verbose) override{
    if(!if_array){
      fprintf(fp,"  %%%d = alloc i32\n",idx);
      fprintf(fp,"  store @%s_%d, %%%d\n",ident.c_str(),var_id[ident]-1,idx);
      if_param[ident] = 1;
      if_array_param[ident] = 0;
      param_idx[ident] = idx;
      return;
    }
    else{
      fprintf(fp,"  %%%d = alloc ",idx);
      gen_type(fp);
      fprintf(fp, "\n");
      fprintf(fp,"  store @%s_%d, %%%d\n",ident.c_str(),var_id[ident]-1,idx);
      if_param[ident] = 1;
      if_array_param[ident] = 1;
      param_idx[ident] = idx;
      func_var_dim[ident] = dim->vals.size()+1;
      return;
    }
  }
};

class FuncRParamsAST : public BaseAST{
 public:
  void gen_type(FILE* fp) override{}
  void Dump(FILE* fp, bool verbose) override{
    calc_param = true;
    for (auto ptr = vec.begin(); ptr != vec.end(); ptr++){
      (*ptr)->Dump(fp,true);
    }
    calc_param = false;
    return;
  }
};

// Block 为什么不能是BaseAST
class BlockAST : public BaseAST{
 public:
  void gen_type(FILE* fp) override{}
  void Dump(FILE* fp, bool verbose) override{
    depth++;
    const_var[depth].clear();
    var[depth].clear();
    var_array[depth].clear();
    const_array[depth].clear();
    if_var[depth].clear();
    if_const[depth].clear();
    if_array[depth].clear();
    if_const_array[depth].clear();
    var_num[depth].clear();
    dim_psum[depth].clear();
    var_dim[depth].clear();
    if_uninit[depth].clear();

    for(auto ptr = vec.begin();ptr != vec.end(); ptr++){
        (*ptr)->Dump(fp,true);
    }
    depth--;
  }
};

// Stmt 自然也是BaseAST
class StmtAST : public BaseAST{
 public:
  std::unique_ptr<BaseAST> exp, stmt, else_stmt, lval;
  void gen_type(FILE* fp) override{}
  void Dump(FILE* fp, bool verbose) override{
    int temp,t1,d1,e1,w1,b1;
    switch(type){
      case 0:
        exp->Dump(fp,true);
        fprintf(fp,"  ret ");
        print_ast(fp,exp);
        fprintf(fp,"\n");
        fprintf(fp,"%%dummy_%d:\n",dummy_num++);
        break;
      case 1:
        exp->Dump(fp,true);
        ident = lval->ident;
        if(lval->type == 0){
          temp = depth;
          if (if_param[ident]){
            fprintf(fp,"  store ");
            print_ast(fp,exp);
            fprintf(fp,", %%%d\n", param_idx[ident]);
            break;
          }
          while(!if_var[temp][ident]&&!if_const[temp][ident]){
            temp--;
          }
          fprintf(fp,"  store ");
          print_ast(fp,exp);
          fprintf(fp,", @%s_%d\n",ident.c_str(),var_num[temp][ident]);
          var[temp][ident] = exp->val;
        }
        else{
          lval->Dump(fp,true);
          fprintf(fp,"  store ");
          print_ast(fp,exp);
          fprintf(fp,", %%%d\n",lval->idx);
        }
        break;
      case 2:
        exp->Dump(fp,true);
        break;
      case 3:
        if(if_int) fprintf(fp,"  ret 0\n");
        else fprintf(fp,"  ret\n");
        fprintf(fp,"%%dummy_%d:\n",dummy_num++);
        break;
      case 4:
        exp->Dump(fp,true);
        fprintf(fp,"  br ");
        print_ast(fp,exp);
        t1 = then_num++,e1 = else_num++,d1 = end_num++;
        fprintf(fp,", %%then_%d, %%else_%d\n",t1,e1);
        fprintf(fp,"%%then_%d:\n",t1);
        stmt->Dump(fp,true);
        fprintf(fp,"  jump %%end_%d\n",d1);
        fprintf(fp,"%%else_%d:\n",e1);
        else_stmt->Dump(fp,true);
        fprintf(fp,"  jump %%end_%d\n",d1);
        fprintf(fp,"%%end_%d:\n",d1);
        break;
      case 5:
        t1 = then_num++,d1 = end_num++;
        exp->Dump(fp,true);
        fprintf(fp,"  br ");
        print_ast(fp,exp);
        fprintf(fp,", %%then_%d, %%end_%d\n",t1,d1);
        fprintf(fp,"%%then_%d:\n",t1);
        stmt->Dump(fp,true);
        fprintf(fp,"  jump %%end_%d\n",d1);
        fprintf(fp,"%%end_%d:\n",d1);
        break;
      case 6:
        d1 = end_num++, w1 = while_num++, b1 = body_num++;
        w.push_back(w1);
        d.push_back(d1);
        fprintf(fp,"  jump %%while_entry_%d\n",w1);
        fprintf(fp,"%%while_entry_%d:\n",w1);
        exp->Dump(fp,true);
        fprintf(fp,"  br ");
        print_ast(fp,exp);
        fprintf(fp,", %%while_body_%d, %%end_%d\n",b1,d1);
        fprintf(fp,"%%while_body_%d:\n",b1);
        stmt->Dump(fp,true);
        fprintf(fp,"  jump %%while_entry_%d\n",w1);
        fprintf(fp,"%%end_%d:\n",d1);
        w.pop_back();
        d.pop_back();
        break;
      case 7:
        fprintf(fp,"  jump %%end_%d\n",d.back());
        fprintf(fp,"%%dummy_%d:\n",dummy_num++);
        break;
      case 8:
        fprintf(fp,"  jump %%while_entry_%d\n",w.back());
        fprintf(fp,"%%dummy_%d:\n",dummy_num++);
        break;
      case 9:
        exp->Dump(fp,true);
        break;
      default:
        break;
    }
  }
};


class ExpAST : public BaseAST{
  public:
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> ano_exp;
    std::unique_ptr<BaseAST> params, lval;
    void gen_type(FILE* fp) override{}
    void Dump(FILE* fp, bool verbose) override{
      int t1, d1;
      switch(type){
        case 0:
          exp->Dump(fp,verbose);
          if(verbose) fprintf(fp,"  %%%d = eq ",idx);
          if(verbose) print_ast(fp, exp);
          if(verbose) fprintf(fp,", 0\n");
          val = !exp->val;
          break;
        case 1:
          exp->Dump(fp,verbose);
          if(verbose) fprintf(fp,"  %%%d = sub 0, ",idx);
          if(verbose) print_ast(fp,exp);
          if(verbose) fprintf(fp,"\n");
          val = -exp->val;
          break;
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
          ano_exp->Dump(fp,verbose);
          exp->Dump(fp,verbose);
          if(verbose) fprintf(fp,"  %%%d = ",idx);
          switch(type){
            case 2:
              if(verbose) fprintf(fp,"mul");
              val = exp->val * ano_exp->val;
              break;
            case 3:
              if(verbose) fprintf(fp,"div");
              if(ano_exp->val == 0){
                ano_exp->val = 1;
              }
              val = exp->val / ano_exp->val;
              break;
            case 4:
              if(verbose) fprintf(fp,"mod");
              if(ano_exp->val == 0){
                ano_exp->val = 1;
              }
              val = exp->val % ano_exp->val;
              break;
            case 5:
              if(verbose) fprintf(fp,"add");
              val = exp->val + ano_exp->val;
              break;
            case 6:
              if(verbose) fprintf(fp,"sub");
              val = exp->val - ano_exp->val;
              break;
            case 7:
              if(verbose) fprintf(fp,"lt");
              val = exp->val < ano_exp->val;
              break;
            case 8:
              if(verbose) fprintf(fp,"gt");
              val = exp->val > ano_exp->val;
              break;
            case 9:
              if(verbose) fprintf(fp,"le");
              val = exp->val <= ano_exp->val;
              break;
            case 10:
              if(verbose) fprintf(fp,"ge");
              val = exp->val >= ano_exp->val;
              break;
            case 11:
              if(verbose) fprintf(fp,"eq");
              val = exp->val == ano_exp->val;
              break;
            case 12:
              if(verbose) fprintf(fp,"ne");
              val = exp->val != ano_exp->val;
              break;
          }
          if(verbose) {
            fprintf(fp," ");
            print_ast(fp,exp);
            fprintf(fp,", ");
            print_ast(fp,ano_exp);
            fprintf(fp,"\n");
          }
          break;
       case 13:
          t1 = then_num++, d1 = end_num++;
          exp->Dump(fp,verbose);
          if(verbose){
            fprintf(fp,"  %%%d = ne ",idx-2);
            print_ast(fp,exp);
            fprintf(fp,", 0\n");\
            fprintf(fp,"  store %%%d, @temp_%d\n",idx-2,temp_num);
            fprintf(fp,"  br %%%d, %%end_%d, %%then_%d\n",idx-2,d1,t1);
            fprintf(fp,"%%end_%d:\n",d1);
          }
          ano_exp->Dump(fp,verbose);
          if(verbose){
            fprintf(fp,"  %%%d = ne ",idx-1);
            print_ast(fp,ano_exp);
            fprintf(fp,", 0\n");
            fprintf(fp,"  store %%%d, @temp_%d\n",idx-1,temp_num);
            fprintf(fp,"  jump %%then_%d\n",t1);
            fprintf(fp,"%%then_%d:\n",t1);
            fprintf(fp,"  %%%d = load @temp_%d\n",idx,temp_num);
            printf("%d\n",idx);
          }
          val = exp->val && ano_exp->val;
          break;
        case 14:
          t1 = then_num++, d1 = end_num++;
          exp->Dump(fp,verbose);
          if(verbose){
            fprintf(fp,"  %%%d = ne ",idx-2);
            print_ast(fp,exp);
            fprintf(fp,", 0\n");
            fprintf(fp,"  store %%%d, @temp_%d\n",idx-2,temp_num);
            fprintf(fp,"  br %%%d, %%then_%d, %%end_%d\n",idx-2,t1,d1);
            fprintf(fp,"%%end_%d:\n",d1);
          }
          ano_exp->Dump(fp,verbose);
          if(verbose){
            fprintf(fp,"  %%%d = ne ",idx-1);
            print_ast(fp,ano_exp);
            fprintf(fp,", 0\n");
            fprintf(fp,"  store %%%d, @temp_%d\n",idx-1,temp_num);
            fprintf(fp,"  jump %%then_%d\n",t1);
            fprintf(fp,"%%then_%d:\n",t1);
            fprintf(fp,"  %%%d = load @temp_%d\n",idx,temp_num);
          }
          val = exp->val || ano_exp->val;
          break;
        case 15:
          lval->Dump(fp,verbose);
          if(lval->type == 0){
            val = lval->val;
            idx = lval->idx;
          }
          else{
            if(!calc_param) fprintf(fp, "  %%%d = load %%%d\n", idx, lval->idx);
            else{ idx = lval->idx; }
            val = 1;
          }
          break;
        case 16:
          //有参数的函数调用
          params->Dump(fp,true);

          if(functype[ident]=="void") fprintf(fp,"  call @%s(",ident.c_str());
          else fprintf(fp,"  %%%d = call @%s(",idx,ident.c_str());

          for(auto ptr = params->vec.begin();ptr != params->vec.end(); ptr++){
            if(ptr != params->vec.begin()){
              fprintf(fp,", ");
            }
            print_ast(fp,*ptr);
          }
          fprintf(fp,")\n");
          break;
        case 17:
          if(functype[ident]=="void") fprintf(fp,"  call @%s()\n",ident.c_str());
          else  fprintf(fp,"  %%%d = call @%s()\n",idx,ident.c_str());
          break;
        default:
          break;
      }
      return;
    }
};

class LValAST : public BaseAST{
  public:
    std::unique_ptr<BaseAST> exp;
    void gen_type(FILE* fp) override{}
    void Dump(FILE* fp,bool verbose) override{
      int temp;
      switch(type){
        case 0:
          temp = depth;
          if(if_param[ident]){
            if(verbose) {
              if(!calc_param){
                fprintf(fp,"  %%%d =  load %%%d\n",idx,param_idx[ident]);
              }
              else{
                if(!if_array_param[ident]) fprintf(fp,"  %%%d = load %%%d\n",idx,param_idx[ident]);
                else {
                  fprintf(fp,"  %%zly_%d = load %%%d\n", zly++, param_idx[ident]);
                  fprintf(fp, "  %%%d = getptr %%zly_%d, 0\n", idx, zly-1);
                }
              }
            }
            val = 1;
            break;
          }
          while(!if_var[temp][ident]&&!if_const[temp][ident]&&!if_array[temp][ident]&&!if_const_array[temp][ident]){
            temp--;
          }
          if(if_var[temp][ident]){
            val = var[temp][ident];
            if(verbose) fprintf(fp,"  %%%d = load @%s_%d\n",idx,ident.c_str(),var_num[temp][ident]);   
          }
          if(if_array[temp][ident] || if_const_array[temp][ident]){
            if(verbose) {
              if(!calc_param){
                fprintf(fp,"  %%%d = load @%s_%d\n",idx,ident.c_str(),var_num[temp][ident]);
              }
              else{
                fprintf(fp,"  %%%d = getelemptr @%s_%d, 0\n",idx,ident.c_str(),var_num[temp][ident]);
              }
            }
          }
          if(if_const[temp][ident]){
            idx = -1;
            val = const_var[temp][ident];
          }
          break;
        case 1:
          if(if_param[ident]){
              exp->Dump(fp,verbose);
              int len = exp->vec.size()-1;
              if(calc_param) len++;
              for(auto ptr = exp->tvals.begin(); ptr != exp->tvals.end(); ptr++){
                if(ptr == exp->tvals.begin()){
                  fprintf(fp,"  %%zly_%d = load %%%d\n", zly++, param_idx[ident]);
                  if(ptr->type == 0) fprintf(fp, "  %%%d = getptr %%zly_%d, %%%d\n", idx-len, zly-1, ptr->y.idx);
                  else fprintf(fp, "  %%%d = getptr %%zly_%d, %d\n", idx-len, zly-1, ptr->y.val);
                }
                else{
                  if(ptr->type == 0) fprintf(fp, "  %%%d = getelemptr %%%d, %%%d\n", idx-len, idx-len-1, ptr->y.idx);
                  else fprintf(fp, "  %%%d = getelemptr %%%d, %d\n", idx-len, idx-len-1, ptr->y.val);
                }
                len--;
              }
              if(calc_param){
                if(exp->vec.size()!=func_var_dim[ident]) fprintf(fp, "  %%%d = getelemptr %%%d, 0\n", idx, idx-1);
                else fprintf(fp,"  %%%d = load %%%d\n", idx, idx-1);
              }
              break;
          }
          temp = depth;
          while(!if_array[temp][ident]&&!if_const_array[temp][ident]){
            temp--;
          }
          exp->Dump(fp,verbose);
          int len = exp->vec.size()-1;
          if(calc_param) len++;
          for(auto ptr = exp->tvals.begin(); ptr != exp->tvals.end(); ptr++){
            if(ptr == exp->tvals.begin()){
              if(ptr->type == 0) fprintf(fp, "  %%%d = getelemptr @%s_%d, %%%d\n", idx-len, ident.c_str(), var_num[temp][ident], ptr->y.idx);
              else fprintf(fp, "  %%%d = getelemptr @%s_%d, %d\n", idx-len, ident.c_str(), var_num[temp][ident], ptr->y.val);
            }
            else{
              if(ptr->type == 0) fprintf(fp, "  %%%d = getelemptr %%%d, %%%d\n", idx-len, idx-len-1, ptr->y.idx);
              else fprintf(fp, "  %%%d = getelemptr %%%d, %d\n", idx-len, idx-len-1, ptr->y.val);
            }
            len--;
          }
          if(calc_param){
            if(exp->vec.size() != var_dim[temp][ident].size()) fprintf(fp, "  %%%d = getelemptr %%%d, 0\n", idx, idx-1);
            else fprintf(fp,"  %%%d = load %%%d\n", idx, idx-1);
          }
          break;
      }
    }
};

class ConstDeclAST : public BaseAST{
  public:
    void gen_type(FILE* fp) override{}
    void Dump(FILE* fp,bool verbose) override{
      for(auto ptr = vec.begin();ptr != vec.end(); ptr++){
          (*ptr)->Dump(fp,verbose);
      }
    }
};

class ConstDefAST : public BaseAST{
  public:
    std::unique_ptr<BaseAST> initval;
    std::unique_ptr<BaseAST> dim;
    void gen_type(FILE* fp) override{}
    void Dump(FILE* fp,bool verbose) override{
      int temp,len;
      switch(type){
        case 0:
          initval->Dump(fp,false);
          if_const[depth][ident] = 1;
          var_num[depth][ident] = var_id[ident]++; 
          const_var[depth][ident] = initval->vals[0];
          break;
        case 1:
          dim->ident = ident;
          dim->Dump(fp,false);
          len = dim->vals.size();
          for(int i = 0; i <= len-1; i++){
            var_dim[depth][ident].push_back(dim->vals[i]);
          }
          temp = 1;
          for(int i = len-1; i >= 0; i--){
            temp *= dim->vals[i];
            dim_psum[depth][ident].push_back(temp);
          }
          initval->ident = ident;
          align_id = dim_psum[depth][ident].back();
          initval->Dump(fp,false);
          if_const_array[depth][ident] = 1;
          var_num[depth][ident] = var_id[ident]++;
          for(auto ptr = initval->vals.begin(); ptr != initval->vals.end(); ptr++){
            const_array[depth][ident].push_back(*ptr);
          }
          if(verbose){
            fprintf(fp,"  @%s_%d = alloc ",ident.c_str(),var_num[depth][ident]);
            for(int i=0; i<len; i++){
              fprintf(fp,"[");
            }
            fprintf(fp,"i32");
            for(int i = len-1; i>=0; i--){
              fprintf(fp,", %d]", var_dim[depth][ident][i]);
            }
            fprintf(fp,"\n");
            for(int i = 0; i < dim_psum[depth][ident].back(); i++){
              temp = i;
              for(int j = 0; j < len-1; j++){
                if(j == 0){
                  fprintf(fp,"  %%zly_%d = getelemptr @%s_%d, %d\n", zly, ident.c_str(), var_num[depth][ident], temp/dim_psum[depth][ident][len-j-2]);
                }
                else{
                  fprintf(fp,"  %%zly_%d = getelemptr %%zly_%d, %d\n", zly, zly-1, temp/dim_psum[depth][ident][len-j-2]);
                }
                temp %= dim_psum[depth][ident][len-j-2];
                zly++;
              }
              if(len == 1) {
                fprintf(fp,"  %%zly_%d = getelemptr @%s_%d, %d\n", zly, ident.c_str(), var_num[depth][ident], temp);
              }
              else fprintf(fp,"  %%zly_%d = getelemptr %%zly_%d, %d\n", zly, zly-1, temp);
              zly++;
              fprintf(fp,"  store %d, %%zly_%d\n", initval->vals[i], zly-1);
            }
          }
          break;
        default:
          break;
      }
      return;
  }
};

class ExpListAST : public BaseAST{
  public:
    void gen_type(FILE* fp) override{}
    void Dump(FILE* fp,bool verbose) override{
      for(auto ptr = vec.begin(); ptr != vec.end(); ptr++){
        printf("wokao\n");
        (*ptr)->Dump(fp,verbose);
        tvals.push_back(ret_tval(*ptr));
      }
      return;
    }
};

class ConstExpListAST : public BaseAST{
  public:
    void gen_type(FILE* fp) override{}
    void Dump(FILE* fp,bool verbose) override{
      for(auto ptr = vec.begin(); ptr != vec.end(); ptr++){
        (*ptr)->Dump(fp,false);
        vals.push_back((*ptr)->val);
      }
    }
};

class ConstInitValAST : public BaseAST{
  public: 
    void gen_type(FILE* fp) override{}
    void Dump(FILE* fp,bool verbose) override{
      if(type == 0){
        auto ptr = vec.begin();
        (*ptr)->Dump(fp,verbose);
        vals.push_back((*ptr)->val);
      }
      else{
        int len = 0;
        for(auto ptr = vec.begin(); ptr != vec.end(); ptr++){
          int temp = align_id;
          for(int i = dim_psum[depth][ident].size()-1; i>=0; i--){
            if(len % dim_psum[depth][ident][i] == 0 && dim_psum[depth][ident][i] < align_id){
              align_id = dim_psum[depth][ident][i];
              break;
            }
          }
          (*ptr)->ident = ident;
          (*ptr)->Dump(fp,verbose);
          align_id = temp;
          for(auto aptr = (*ptr)->vals.begin(); aptr != (*ptr)->vals.end(); aptr++){
            vals.push_back(*aptr);
            len++;
          }
        }
        for(int i = len; i < align_id; i++){
          vals.push_back(0);
        }
      }
    }
};

class VarDeclAST : public BaseAST{
  public:
    void gen_type(FILE* fp) override{}
    void Dump(FILE* fp,bool verbose) override{
      for(auto ptr = vec.begin();ptr != vec.end(); ptr++){
          (*ptr)->Dump(fp,verbose);
      }
    }
};

class VarDefAST : public BaseAST{
  public:
    std::unique_ptr<BaseAST> initval,dim;
    void gen_type(FILE* fp) override{}
    void Dump(FILE* fp,bool verbose) override{
        int temp,len;
        switch(type){
          case 0:
          case 1:
            if_var[depth][ident] = 1;
            var_num[depth][ident] = var_id[ident]++;
            if(verbose) fprintf(fp,"  @%s_%d = alloc i32\n",ident.c_str(),var_num[depth][ident]);
            if(type == 1) {
              initval->Dump(fp,verbose);
              var[depth][ident] = initval->vals[0];
              if(verbose){
                fprintf(fp,"  store ");
                print_ast(fp,initval->vec[0]);
                fprintf(fp,", @%s_%d\n",ident.c_str(),var_num[depth][ident]);
              }
            }
            else var[depth][ident] = 0;
            break;
          case 2:
          case 3:
            if_array[depth][ident] = 1;
            var_num[depth][ident] = var_id[ident]++;
            dim->ident = ident;
            dim->Dump(fp,false);
            len = dim->vals.size();
            for(int i = 0; i <= len-1; i++){
              var_dim[depth][ident].push_back(dim->vals[i]);
            }
            temp = 1;
            for(int i = len-1; i >= 0; i--){
              temp *= dim->vals[i];
              dim_psum[depth][ident].push_back(temp);
            }
            if(verbose) {
              fprintf(fp,"  @%s_%d = alloc ",ident.c_str(),var_num[depth][ident]);
              for(int i=0; i<len; i++){
                fprintf(fp,"[");
              }
              fprintf(fp,"i32");
              for(int i = len-1; i>=0; i--){
                fprintf(fp,", %d]", var_dim[depth][ident][i]);
              }
              fprintf(fp,"\n");
            }
            if(type == 3){
              initval->ident = ident;
              align_id = dim_psum[depth][ident].back();
              initval->Dump(fp,verbose);
              for(auto ptr = initval->vals.begin(); ptr != initval->vals.end(); ptr++){
                var_array[depth][ident].push_back(*ptr);
              }
              if(verbose){
                for(int i = 0; i < dim_psum[depth][ident].back(); i++){
                  temp = i;
                  for(int j = 0; j < len-1; j++){
                    if(j == 0){
                      fprintf(fp,"  %%zly_%d = getelemptr @%s_%d, %d\n", zly, ident.c_str(), var_num[depth][ident], temp/dim_psum[depth][ident][len-j-2]);
                    }
                    else{
                      fprintf(fp,"  %%zly_%d = getelemptr %%zly_%d, %d\n", zly, zly-1, temp/dim_psum[depth][ident][len-j-2]);
                    }
                    temp %= dim_psum[depth][ident][len-j-2];
                    zly++;
                  }
                  if(len == 1) fprintf(fp,"  %%zly_%d = getelemptr @%s_%d, %d\n", zly, ident.c_str(), var_num[depth][ident], temp);
                  else fprintf(fp,"  %%zly_%d = getelemptr %%zly_%d, %d\n", zly, zly-1, temp);
                  zly++;
                  if(initval->tvals[i].type == 0){
                    fprintf(fp,"  store %%%d, %%zly_%d\n", initval->tvals[i].y.idx, zly-1);
                  }
                  else{
                    fprintf(fp,"  store %d, %%zly_%d\n", initval->tvals[i].y.val, zly-1);
                  }
                }
              }
            }
            else{
              for(int i = 0; i < dim_psum[depth][ident].back(); i++){
                var_array[depth][ident].push_back(0);
              }
              if_uninit[depth][ident] = 1;
            }
            break;
          default:
            break;
        }
        return;
    }
};

class InitValAST : public BaseAST{
  public:
    void gen_type(FILE* fp) override{}
    void Dump(FILE* fp,bool verbose) override{
      if(type == 0){
        auto ptr = vec.begin();
        (*ptr)->Dump(fp,verbose);
        vals.push_back((*ptr)->val);
        tvals.push_back(ret_tval(*ptr));
      }
      else{
        int len = 0;
        for(auto ptr = vec.begin(); ptr != vec.end(); ptr++){
          int temp = align_id;
          for(int i = dim_psum[depth][ident].size()-1; i>=0; i--){
            if(len % dim_psum[depth][ident][i] == 0 && dim_psum[depth][ident][i] < align_id){
              align_id = dim_psum[depth][ident][i];
              break;
            }
          }
          (*ptr)->ident = ident;
          (*ptr)->Dump(fp,verbose);
          align_id = temp;
          for(auto aptr = (*ptr)->vals.begin(); aptr != (*ptr)->vals.end(); aptr++){
            vals.push_back(*aptr);
          }
          for(auto aptr = (*ptr)->tvals.begin(); aptr != (*ptr)->tvals.end(); aptr++){
            tvals.push_back(*aptr);
            len++;
          }
        }
        for(int i = len; i < align_id; i++){
          vals.push_back(0);
          tval temp;
          temp.type = 1;
          temp.y.val = 0;
          tvals.push_back(temp);
        }
      }
    }
};


