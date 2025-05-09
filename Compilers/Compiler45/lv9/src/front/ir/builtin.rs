use koopa::ir::{Function, FunctionData, Program, Type};
use std::collections::HashMap;

#[derive(Copy, Clone)]
enum IRType {
    Void,
    Int,
    IntPointer,
}

impl Into<Type> for IRType {
    fn into(self) -> Type {
        match self {
            IRType::Void => Type::get_unit(),
            IRType::Int => Type::get_i32(),
            IRType::IntPointer => Type::get_pointer(Type::get_i32()),
        }
    }
}

struct FuncDecl {
    name: &'static str,
    params: &'static [IRType],
    ret: IRType,
}

const BUILTIN_FUNCTIONS: [FuncDecl; 8] = [
    FuncDecl {
        name: "getint",
        params: &[],
        ret: IRType::Int,
    },
    FuncDecl {
        name: "getch",
        params: &[],
        ret: IRType::Int,
    },
    FuncDecl {
        name: "getarray",
        params: &[IRType::IntPointer],
        ret: IRType::Int,
    },
    FuncDecl {
        name: "putint",
        params: &[IRType::Int],
        ret: IRType::Void,
    },
    FuncDecl {
        name: "putch",
        params: &[IRType::Int],
        ret: IRType::Void,
    },
    FuncDecl {
        name: "putarray",
        params: &[IRType::Int, IRType::IntPointer],
        ret: IRType::Void,
    },
    FuncDecl {
        name: "starttime",
        params: &[],
        ret: IRType::Void,
    },
    FuncDecl {
        name: "stoptime",
        params: &[],
        ret: IRType::Void,
    },
];

pub fn generate_builtin_decl(program: &mut Program, func_table: &mut HashMap<String, Function>) {
    for builtin_func in BUILTIN_FUNCTIONS {
        let func_data = FunctionData::new_decl(
            "@".to_string() + builtin_func.name,
            builtin_func
                .params
                .iter()
                .copied()
                .map(Into::into)
                .collect(),
            builtin_func.ret.into(),
        );
        let func = program.new_func(func_data);
        func_table.insert(builtin_func.name.to_string(), func);
    }
}
