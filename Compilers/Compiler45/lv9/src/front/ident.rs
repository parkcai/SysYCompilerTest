use crate::front::ast::FuncDef;
use crate::front::ir::initial_list::InitializeList;
use koopa::ir::Value;

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub struct Variable {
    pub koopa_def: Value,
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub struct Constant {
    pub value: i32,
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub struct ConstArray {
    pub koopa_def: Value,
    pub values: InitializeList<i32>,
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub struct FunctionParam {
    pub koopa_def: Value,
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub struct Function {
    pub def: FuncDef,
}

/// The type of identifier and its information.
#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub enum Identifier {
    Variable(Variable),
    Constant(Constant),
    ConstArray(ConstArray),
}

impl Identifier {
    pub fn from_variable(koopa_def: Value) -> Self {
        Identifier::Variable(Variable { koopa_def })
    }

    pub fn from_constant(value: i32) -> Self {
        Identifier::Constant(Constant { value })
    }

    pub fn from_const_array(koopa_def: Value, values: InitializeList<i32>) -> Self {
        Identifier::ConstArray(ConstArray { koopa_def, values })
    }

    pub fn koopa_def(&self) -> Option<Value> {
        match self {
            Identifier::Variable(var) => Some(var.koopa_def),
            Identifier::ConstArray(arr) => Some(arr.koopa_def),
            _ => None,
        }
    }
}
