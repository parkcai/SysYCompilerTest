mod ast;
mod back;
mod cmdline;
mod front;

use crate::back::codegen::GenerateAsm;
use crate::back::o1::peephole_opt;
use crate::front::ir::generate_ir;
use koopa::back::KoopaGenerator;
use koopa::ir::Type;
use lalrpop_util::lalrpop_mod;

// 引用 lalrpop 生成的解析器
// 因为我们刚刚创建了 sysy.lalrpop, 所以模块名是 sysy
lalrpop_mod!(sysy);

fn main() {
    Type::set_ptr_size(4);
    let parser = sysy::CompUnitParser::new();
    let params = cmdline::Params::parse();
    let input = std::fs::read_to_string(params.input).unwrap();
    let result = parser.parse(&input).unwrap();
    // println!("{:#?}", result);
    if params.koopa {
        let ir = generate_ir(&result);
        KoopaGenerator::from_path(params.output)
            .unwrap()
            .generate_on(&ir)
            .unwrap();
        return;
    }
    if params.riscv || params.perf {
        let ir = generate_ir(&result);
        let mut asm = ir.generate(&mut None, &ir);
        peephole_opt(&mut asm);
        std::fs::write(params.output, asm.dump()).unwrap();
        return;
    }
}
