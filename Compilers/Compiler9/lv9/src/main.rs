mod ast_def;
mod koopa_ir_generator;
mod riscv_generator;
mod perf_generator;
mod macro_processor;

use koopa::ir::Type;
use koopa_ir_generator::koopa_ir_generator;
use lalrpop_util::lalrpop_mod;
use macro_processor::macro_processor;
use perf_generator::perf_generator;
use riscv_generator::riscv_generator;
use std::env::args;
use std::fs::read_to_string;

#[macro_use]
extern crate lazy_static;

lalrpop_mod!(sysy);

fn main() -> Result<(), Box<dyn std::error::Error>> {
    Type::set_ptr_size(4);

    // 解析命令行参数
    let mut args = args();
    args.next();
    let mode = args.next().expect("mode not found");
    let input = args.next().expect("input file not found");
    args.next();
    let output = args.next().expect("output file not found");

    // 读取输入文件
    let input = read_to_string(input)?;
    let str_tmp = input.lines().next().unwrap().trim().to_string();

    match mode.as_str() {
        "-macro" => {
            let mut macro_str = String::new();
            macro_processor(input.clone(), &mut macro_str)?;
            std::fs::write(output, macro_str)?;
            Ok(())
        },
        _ => {
            // 调用 lalrpop 生成的 parser 解析输入文件
            let ast = sysy::CompUnitParser::new()
                .parse(&input)
                .expect("parse error");

            let mut koopa_ir_str = String::new();
            koopa_ir_generator(ast, &mut koopa_ir_str, (str_tmp.clone(), mode.clone()))?;
            match mode.as_str() {
                "-koopa" => {
                    std::fs::write(output, koopa_ir_str)?;
                    Ok(())
                },
                "-riscv"=> {
                    let mut riscv_str = String::new();
                    riscv_generator(koopa_ir_str.clone(), &mut riscv_str)?;
                    std::fs::write(output, riscv_str)?;
                    Ok(())
                },
                "-perf" => {
                    let mut perf_str = String::new();
                    perf_generator(koopa_ir_str, &mut perf_str, str_tmp.clone())?;
                    std::fs::write(output, perf_str)?;
                    Ok(())
                },
                mode => Err(mode),
            }
        }
    }?;

    Ok(())
}

    // 定义正则表达式来匹配临时符号和具名符号
    // let re = Regex::new(r"[%@][a-zA-Z0-9_]+")?;
    // let mut symbol_count: BTreeMap<String, usize> = BTreeMap::new();

    // // 迭代匹配到的符号并计数
    // for cap in re.captures_iter(&koopa_ir_str) {
    //     let symbol = cap[0].to_string();
    //     let count = symbol_count.entry(symbol.clone()).or_insert(0);
    //     *count += 1;
    // }

    // 输出每个符号的出现次数
    // for (symbol, count) in &symbol_count {
    //     if symbol.starts_with("%") && !(symbol.starts_with("%L_AND") || symbol.starts_with("%L_OR") || symbol.starts_with("%end") || symbol.starts_with("%else") || symbol.starts_with("%while") || symbol.starts_with("%then") || symbol == "%a" || symbol == "%b") && *count > 2 {
    //         panic!("Symbol {} appears {} times", symbol, count);
    //     }
        // let tmp_str = match *count > 2{
        //     true => "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!",
        //     false => "",
        // };
        // println!("Symbol {} appears {} times {}", symbol, count, tmp_str);
    // }