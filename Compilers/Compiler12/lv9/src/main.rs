mod ast;
mod ir_gen;
mod riscv_gen;

use lalrpop_util::lalrpop_mod;
use std::env::args;
use std::fs::{read_to_string, write};
use std::io::Result;

// 引用 lalrpop 生成的解析器
lalrpop_mod! {
    sysy
}

fn main() -> Result<()> {
    // 解析命令行参数
    let mut args = args();
    args.next();
    let mode = args.next().unwrap();
    let input = args.next().unwrap();
    args.next();
    let output = args.next().unwrap();

    // 读取输入文件
    let input = read_to_string(input)?;
    
    // 调用 lalrpop 生成的 parser 解析输入文件
    let ast = sysy::CompUnitParser::new().parse(&input).unwrap();

    // 输出解析得到的 AST
    println!("{:#?}", ast);

    match mode.as_str() {
        "-koopa" => {
            // 生成 Koopa IR
            let koopa_ir = ir_gen::generate_ir(&ast);
            write(output, koopa_ir)?;
        }
        "-riscv" => {
            // 生成 Koopa IR
            let koopa_ir = ir_gen::generate_ir(&ast);
            // 使用 Koopa IR 生成 RISC-V 汇编代码
            let riscv_code = riscv_gen::generate_riscv(&koopa_ir);
            write(output, riscv_code)?;
        }
        "-perf" => {
            // 生成 Koopa IR
            let koopa_ir = ir_gen::generate_ir(&ast);
            // 使用 Koopa IR 生成 RISC-V 汇编代码
            let riscv_code = riscv_gen::generate_riscv(&koopa_ir);
            write(output, riscv_code)?;
        }
        _ => {
            eprintln!("Unsupported mode: {}", mode);
        }
    }

    Ok(())
}
