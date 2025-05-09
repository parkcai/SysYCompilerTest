#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include <map>
#include "ast.h"
#include "koopa.h"

using namespace std::string_literals;

extern FILE *yyin;
extern int yyparse(std::unique_ptr<BaseAST> &ast);

std::stringstream ass;

void visit(const koopa_raw_program_t &program, auto &&action);
void visit(const koopa_raw_slice_t &slice, auto &&action);
void visit(const koopa_raw_function_t &func, auto &&action);
void visit(const koopa_raw_basic_block_t &bb, auto &&action);
void visit(const koopa_raw_value_t &value, auto &&action);

void visit(const auto &item, auto &&action) {
    action(item);
}

void Beqz(std::string rs, std::string label) {
    ass << "    beqz " << rs << ", " << label << "\n";
}
void Bnez(std::string rs, std::string label) {
    ass << "    bnez " << rs << ", " << label << "\n";
}
void J(std::string label) {
    ass << "    j " << label << "\n";
}
void Call(std::string label) {
    ass << "    call " << label << "\n";
}
void Ret() {
    ass << "    ret\n";
}
void Lw(std::string rs, int imm12, std::string rd) {
    ass << "    lw " << rs << ", " << imm12 << "(" << rd << ")\n";
}
void Sw(std::string rs2, int imm12, std::string rs1) {
    ass << "    sw " << rs2 << ", " << imm12 << "(" << rs1 << ")\n";
}
void Add(std::string rd, std::string rs1, std::string rs2) {
    ass << "    add " << rd << ", " << rs1 << ", " << rs2 << "\n";
}
void Addi(std::string rd, std::string rs1, int imm12) {
    ass << "    addi " << rd << ", " << rs1 << ", " << imm12 << "\n";
}
void Sub(std::string rd, std::string rs1, std::string rs2) {
    ass << "    sub " << rd << ", " << rs1 << ", " << rs2 << "\n";
}
void Slt(std::string rd, std::string rs1, std::string rs2) {
    ass << "    slt " << rd << ", " << rs1 << ", " << rs2 << "\n";
}
void Sgt(std::string rd, std::string rs1, std::string rs2) {
    ass << "    sgt " << rd << ", " << rs1 << ", " << rs2 << "\n";
}
void Seqz(std::string rd, std::string rs) {
    ass << "    seqz " << rd << ", " << rs << "\n";
}
void Snez(std::string rd, std::string rs) {
    ass << "    snez " << rd << ", " << rs << "\n";
}
void Xor(std::string rd, std::string rs1, std::string rs2) {
    ass << "    xor " << rd << ", " << rs1 << ", " << rs2 << "\n";
}
void Xori(std::string rd, std::string rs1, int imm12) {
    ass << "    xori " << rd << ", " << rs1 << ", " << imm12 << "\n";
}
void Or(std::string rd, std::string rs1, std::string rs2) {
    ass << "    or " << rd << ", " << rs1 << ", " << rs2 << "\n";
}
void Ori(std::string rd, std::string rs1, int imm12) {
    ass << "    ori " << rd << ", " << rs1 << ", " << imm12 << "\n";
}
void And(std::string rd, std::string rs1, std::string rs2) {
    ass << "    and " << rd << ", " << rs1 << ", " << rs2 << "\n";
}
void Andi(std::string rd, std::string rs1, int imm12) {
    ass << "    andi " << rd << ", " << rs1 << ", " << imm12 << "\n";
}
void Sll(std::string rd, std::string rs1, std::string rs2) {
    ass << "    sll " << rd << ", " << rs1 << ", " << rs2 << "\n";
}
void Srl(std::string rd, std::string rs1, std::string rs2) {
    ass << "    srl " << rd << ", " << rs1 << ", " << rs2 << "\n";
}
void Sra(std::string rd, std::string rs1, std::string rs2) {
    ass << "    sra " << rd << ", " << rs1 << ", " << rs2 << "\n";
}
void Mul(std::string rd, std::string rs1, std::string rs2) {
    ass << "    mul " << rd << ", " << rs1 << ", " << rs2 << "\n";
}
void Div(std::string rd, std::string rs1, std::string rs2) {
    ass << "    div " << rd << ", " << rs1 << ", " << rs2 << "\n";
}
void Rem(std::string rd, std::string rs1, std::string rs2) {
    ass << "    rem " << rd << ", " << rs1 << ", " << rs2 << "\n";
}
void Li(std::string rd, int imm) {
    ass << "    li " << rd << ", " << imm << "\n";
}
void La(std::string rd, std::string label) {
    ass << "    la " << rd << ", " << label << "\n";
}

void Addi_(std::string rd, std::string rs, int imm) {
    if (-(1 << 11) <= imm && imm < (1 << 11)) {
        Addi(rd, rs, imm);
    } else {
        Li("t2", imm);
        Add(rd, rs, "t2");
    }
}
void Lw_(std::string rs, int imm, std::string rd) {
    if (-(1 << 11) <= imm && imm < (1 << 11)) {
        Lw(rs, imm, rd);
    } else {
        Li("t2", imm);
        Add("t2", rd, "t2");
        Lw(rs, 0, "t2");
    }
}
void Sw_(std::string rs2, int imm, std::string rs1) {
    if (-(1 << 11) <= imm && imm < (1 << 11)) {
        Sw(rs2, imm, rs1);
    } else {
        Li("t2", imm);
        Add("t2", rs1, "t2");
        Sw(rs2, 0, "t2");
    }
}

int getSize(const koopa_raw_type_t &type) {
    switch (type->tag) {
    case KOOPA_RTT_INT32:
        return 4;
    case KOOPA_RTT_UNIT:
        return 0;
    case KOOPA_RTT_ARRAY:
        return type->data.array.len * getSize(type->data.array.base);
    case KOOPA_RTT_POINTER:
        return 4;
    case KOOPA_RTT_FUNCTION:
        return 4;
    };
}

int nearLabel = 0;
struct AssembleFunc {
    int S = 0;
    int R = 0;
    int A = 0;
    std::map<koopa_raw_value_t, int> offset;
    koopa_raw_value_t thisValue;
    
    void Load(std::string rd, const koopa_raw_value_t &value) {
        if (value->kind.tag == KOOPA_RVT_INTEGER) {
            Li(rd, value->kind.data.integer.value);
        } else if (value->kind.tag == KOOPA_RVT_FUNC_ARG_REF) {
            int index = value->kind.data.func_arg_ref.index;
            if (index < 8) {
                Addi(rd, "a" + std::to_string(index), 0);
            } else {
                Lw_(rd, S + 4 * (index - 8), "sp");
            }
        } else if (value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
            La(rd, value->name + 1);
        } else if (value->kind.tag != KOOPA_RVT_ALLOC) {
            assert(offset.contains(value));
            Lw_(rd, offset[value], "sp");
        } else {
            assert(offset.contains(value));
            Addi_(rd, "sp", offset[value]);
        }
    }
    void LoadPtr(std::string rd, const koopa_raw_value_t &value) {
        Load("t3", value);
        Lw_(rd, 0, "t3");
    }
    void Save(std::string rd, const koopa_raw_value_t &value) {
        assert(offset.contains(value));
        Sw_(rd, offset[value], "sp");
    }
    void SavePtr(std::string rd, const koopa_raw_value_t &value) {
        Load("t3", value);
        Sw_(rd, 0, "t3");
    }
    
    bool operator()(const koopa_raw_function_t &func) {
        ass << (func->name + 1);
        ass << ":\n";
        visit(func,
            [&](auto &&value) {
                if constexpr (std::is_same_v<std::remove_cvref_t<decltype(value)>, koopa_raw_value_t>) {
                    if (value->kind.tag == KOOPA_RVT_CALL) {
                        R = 4;
                        A = std::max(A, int(value->kind.data.call.args.len) - 8);
                    }
                    int s = getSize(value->ty);
                    if (value->kind.tag == KOOPA_RVT_ALLOC) {
                        s = getSize(value->ty->data.pointer.base);
                    }
                    if (s) {
                        offset[value] = S;
                        S += s;
                    }
                    return true;
                }
                return false;
            });
        A *= 4;
        int nS = S + R + A;
        nS += (16 - nS % 16) % 16;
        for (auto &[_, o] : offset) {
            o += nS - R - S;
        }
        S = nS;
        if (S) {
            Addi_("sp", "sp", -S);
        }
        if (R) {
            Sw_("ra", S - 4, "sp");
        }
        return false;
    }
    bool operator()(const koopa_raw_basic_block_t &bb) {
        assert(bb->name);
        ass << (bb->name + 1) << ":\n";
        return false;
    }
    bool operator()(const koopa_raw_value_t &value) {
        thisValue = value;
        return false;
    }
    bool operator()(const koopa_raw_load_t &load) {
        LoadPtr("t0", load.src);
        Save("t0", thisValue);
        return true;
    }
    bool operator()(const koopa_raw_store_t &store) {
        Load("t0", store.value);
        SavePtr("t0", store.dest);
        return true;
    }
    bool operator()(const koopa_raw_get_elem_ptr_t &get_elem_ptr) {
        int s = getSize(get_elem_ptr.src->ty->data.pointer.base->data.array.base);
        Li("t0", s);
        Load("t1", get_elem_ptr.index);
        Mul("t0", "t0", "t1");
        Load("t1", get_elem_ptr.src);
        Add("t0", "t0", "t1");
        Save("t0", thisValue);
        return true;
    }
    bool operator()(const koopa_raw_get_ptr_t &get_ptr) {
        int s = getSize(get_ptr.src->ty->data.pointer.base);
        Li("t0", s);
        Load("t1", get_ptr.index);
        Mul("t0", "t0", "t1");
        Load("t1", get_ptr.src);
        Add("t0", "t0", "t1");
        Save("t0", thisValue);
        return true;
    }
    bool operator()(const koopa_raw_binary_t &binary) {
        Load("t0", binary.lhs);
        Load("t1", binary.rhs);
        switch (binary.op) {
        case KOOPA_RBO_NOT_EQ:
            Xor("t0", "t0", "t1");
            Snez("t0", "t0");
            break;
        case KOOPA_RBO_EQ:
            Xor("t0", "t0", "t1");
            Seqz("t0", "t0");
            break;
        case KOOPA_RBO_GT:
            Sgt("t0", "t0", "t1");
            break;
        case KOOPA_RBO_LT:
            Slt("t0", "t0", "t1");
            break;
        case KOOPA_RBO_GE:
            Slt("t0", "t0", "t1");
            Seqz("t0", "t0");
            break;
        case KOOPA_RBO_LE:
            Sgt("t0", "t0", "t1");
            Seqz("t0", "t0");
            break;
        case KOOPA_RBO_ADD:
            Add("t0", "t0", "t1");
            break;
        case KOOPA_RBO_SUB:
            Sub("t0", "t0", "t1");
            break;
        case KOOPA_RBO_MUL:
            Mul("t0", "t0", "t1");
            break;
        case KOOPA_RBO_DIV:
            Div("t0", "t0", "t1");
            break;
        case KOOPA_RBO_MOD:
            Rem("t0", "t0", "t1");
            break;
        case KOOPA_RBO_AND:
            And("t0", "t0", "t1");
            break;
        case KOOPA_RBO_OR:
            Or("t0", "t0", "t1");
            break;
        case KOOPA_RBO_XOR:
            Xor("t0", "t0", "t1");
            break;
        case KOOPA_RBO_SHL:
            Sll("t0", "t0", "t1");
            break;
        case KOOPA_RBO_SHR:
            Srl("t0", "t0", "t1");
            break;
        case KOOPA_RBO_SAR:
            Sra("t0", "t0", "t1");
            break;
        };
        Save("t0", thisValue);
        return true;
    }
    bool operator()(const koopa_raw_branch_t &branch) {
        Load("t0", branch.cond);
        Bnez("t0", "near" + std::to_string(nearLabel));
        J(branch.false_bb->name + 1);
        ass << "near" << std::to_string(nearLabel++) << ":\n";
        J(branch.true_bb->name + 1);
        return true;
    }
    bool operator()(const koopa_raw_jump_t &jump) {
        J(jump.target->name + 1);
        return true;
    }
    bool operator()(const koopa_raw_call_t &call) {
        for (int i = 0; i < call.args.len; i++) {
            auto value = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);
            if (i < 8) {
                Load("a" + std::to_string(i), value);
            } else {
                Load("t0", value);
                Sw_("t0", 4 * (i - 8), "sp");
            }
        }
        Call(call.callee->name + 1);
        if (thisValue->ty->tag != KOOPA_RTT_UNIT) {
            Save("a0", thisValue);
        }
        return true;
    }
    bool operator()(const koopa_raw_return_t &ret) {
        if (ret.value) {
            Load("a0", ret.value);
        }
        if (R) {
            Lw_("ra", S - 4, "sp");
        }
        if (S) {
            Addi_("sp", "sp", S);
        }
        Ret();
        return true;
    }
    bool operator()(const auto &item) {
        return false;
    }
};

struct AssembleProgram {
    void printInitVal(const koopa_raw_value_t &value) {
        switch (value->kind.tag) {
        case KOOPA_RVT_INTEGER:
            ass << "    .word " << value->kind.data.integer.value << "\n";
            return;
        case KOOPA_RVT_AGGREGATE:
            {
                auto slice = value->kind.data.aggregate.elems;
                for (int i = 0; i < slice.len; i++) {
                    printInitVal(reinterpret_cast<koopa_raw_value_t>(slice.buffer[i]));
                }
            }
            return;
        default:
            assert(false);
        };
    }

    bool operator()(const koopa_raw_value_t &value) {
        if (value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
            ass << "    .data\n";
            ass << "    .global ";
            ass << (value->name + 1);
            ass << "\n";
            ass << (value->name + 1) << ":\n";
            auto init = value->kind.data.global_alloc.init;
            if (init->kind.tag == KOOPA_RVT_ZERO_INIT) {
                ass << "    .zero " << getSize(value->ty->data.pointer.base) << "\n";
            } else {
                printInitVal(init);
            }
        }
        return false;
    }
    bool operator()(const koopa_raw_function_t &func) {
        if (func->bbs.len == 0) {
            return true;
        }
        ass << "    .text\n";
        ass << "    .global ";
        ass << (func->name + 1);
        ass << "\n";
        visit(func, AssembleFunc());
        return true;
    }
    bool operator()(const auto &item) {
        return false;
    }
};

void assemble(const koopa_raw_program_t &program) {
    visit(program, AssembleProgram());
}

void visit(const koopa_raw_program_t &program, auto &&action) {
    if (action(program)) {
        return;
    }
    visit(program.values, action);
    visit(program.funcs, action);
}

void visit(const koopa_raw_slice_t &slice, auto &&action) {
    if (action(slice)) {
        return;
    }
    for (int i = 0; i < slice.len; i++) {
        auto ptr = slice.buffer[i];
        switch (slice.kind) {
            case KOOPA_RSIK_FUNCTION:
                visit(reinterpret_cast<koopa_raw_function_t>(ptr), action);
                break;
            case KOOPA_RSIK_BASIC_BLOCK:
                visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr), action);
                break;
            case KOOPA_RSIK_VALUE:
                visit(reinterpret_cast<koopa_raw_value_t>(ptr), action);
                break;
            default:
                break;
        }
    }
}

void visit(const koopa_raw_function_t &func, auto &&action) {
    if (action(func)) {
        return;
    }
    visit(func->bbs, action);
}

void visit(const koopa_raw_basic_block_t &bb, auto &&action) {
    if (action(bb)) {
        return;
    }
    visit(bb->insts, action);
}

void visit(const koopa_raw_value_t &value, auto &&action) {
    if (action(value)) {
        return;
    }
    const auto &kind = value->kind;
    switch (kind.tag) {
        case KOOPA_RVT_INTEGER:
            visit(kind.data.integer, action);
            break;
        case KOOPA_RVT_ZERO_INIT:
            break;
        case KOOPA_RVT_UNDEF:
            break;
        case KOOPA_RVT_AGGREGATE:
            visit(kind.data.aggregate, action);
            break;
        case KOOPA_RVT_FUNC_ARG_REF:
            visit(kind.data.func_arg_ref, action);
            break;
        case KOOPA_RVT_BLOCK_ARG_REF:
            visit(kind.data.block_arg_ref, action);
            break;
        case KOOPA_RVT_ALLOC:
            break;
        case KOOPA_RVT_GLOBAL_ALLOC:
            visit(kind.data.global_alloc, action);
            break;
        case KOOPA_RVT_LOAD:
            visit(kind.data.load, action);
            break;
        case KOOPA_RVT_STORE:
            visit(kind.data.store, action);
            break;
        case KOOPA_RVT_GET_PTR:
            visit(kind.data.get_ptr, action);
            break;
        case KOOPA_RVT_GET_ELEM_PTR:
            visit(kind.data.get_elem_ptr, action);
            break;
        case KOOPA_RVT_BINARY:
            visit(kind.data.binary, action);
            break;
        case KOOPA_RVT_BRANCH:
            visit(kind.data.branch, action);
            break;
        case KOOPA_RVT_JUMP:
            visit(kind.data.jump, action);
            break;
        case KOOPA_RVT_CALL:
            visit(kind.data.call, action);
            break;
        case KOOPA_RVT_RETURN:
            visit(kind.data.ret, action);
            break;
    }
}

int main(int argc, const char *argv[]) {
    // 解析命令行参数. 测试脚本/评测平台要求你的编译器能接收如下参数:
    // compiler 模式 输入文件 -o 输出文件
    
    auto mode = argv[1];
    auto input = argv[2];
    auto output = argv[4];
    
    yyin = std::fopen(input, "r");
    assert(yyin != nullptr);
    
    std::unique_ptr<BaseAST> ast;
    auto ret = yyparse(ast);
    if (ret) {
        return ret;
    }
    
    std::stringstream irs;
    
    if (argc > 5) {
        ast->dump(std::cerr);
        std::cerr << std::endl;
    }
    ast->koopa(irs);
    std::string ir = irs.str();
    
    if (mode == "-koopa"s) {
        std::ofstream os(output);
        os << ir;
    } else if (mode == "-riscv"s || mode == "-perf"s) {
        koopa_program_t program;
        koopa_error_code_t ret = koopa_parse_from_string(ir.c_str(), &program);
        assert(ret == KOOPA_EC_SUCCESS);  // 确保解析时没有出错
        koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
        koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
        koopa_delete_program(program);
        
        assemble(raw);
        std::ofstream os(output);
        
        os << ass.str();
        
        koopa_delete_raw_program_builder(builder);
    }

    return 0;
}
