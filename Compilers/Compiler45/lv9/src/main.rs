use crate::front::generate_ir;
use crate::front::opt::opt;
use front::parser;
use front::parser_context::ParserContext;
use koopa::back::KoopaGenerator;
use std::fs;
use util::args::Params;
use util::logger::{show_error, show_parse_error};

mod back;
mod front;
pub mod macros;
mod util;

fn main() {
    let params = Params::parse();
    let input = fs::read_to_string(&params.input).unwrap_or_else(|e| {
        show_error(&format!("Failed to read input file: {}", e), 1);
    });
    let parser = parser::CompUnitParser::new();
    let mut context = ParserContext::new(&params.input, &input);
    let comp_unit = parser
        .parse(&mut context, &input)
        .unwrap_or_else(|e| show_parse_error(e, &input, &params.input));
    let mut program = generate_ir(comp_unit);
    if params.koopa {
        KoopaGenerator::from_path(&params.output)
            .unwrap()
            .generate_on(&program)
            .unwrap();
    }
    if params.perf {
        opt(&mut program);
    }
    if params.riscv || params.perf {
        let asm = back::generate_asm(program);
        fs::write(&params.output, asm).unwrap();
    }
}
