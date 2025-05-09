use crate::front::ast::CompUnit;
use crate::front::ir::builtin::generate_builtin_decl;
use crate::front::ir::scope::Scope;
use crate::front::ir::GenerateIR;
use crate::util::logger::show_error;
use ir::context;
use koopa::ir::Program;
use lalrpop_util::lalrpop_mod;

pub mod ast;
pub mod ident;
pub mod ir;
pub mod opt;
pub mod parser_context;

lalrpop_mod!(pub parser);

pub fn generate_ir(comp_unit: CompUnit) -> Program {
    let scope = Scope::new();
    let mut ctx = context::Context::new(scope);
    generate_builtin_decl(&mut ctx.program, &mut ctx.func_table);
    comp_unit.generate_ir(&mut ctx).unwrap_or_else(|e| {
        show_error(&format!("{:?}", e), 2);
    });
    ctx.delete_and_link();
    ctx.program()
}
