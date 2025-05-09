use crate::back::register::{Register, T0};
use crate::between;
use compiler_macro::Inst;
use concat_idents::concat_idents;

pub trait Inst {
    fn dump(&self) -> String;
    fn is_branch(&self) -> bool;
}

macro_rules! eval_inst {
    ($name:ident) => {
        #[derive(Debug, Clone, Inst, Eq, PartialEq)]
        pub struct $name {
            pub rd: Register,
            pub rs1: Register,
            pub rs2: Register,
        }
    };
}

macro_rules! eval_inst_with_imm {
    ($name:ident) => {
        eval_inst!($name);

        concat_idents!(struct_name = $name, i {
            #[derive(Debug, Clone, Inst, Eq, PartialEq)]
            pub struct struct_name {
                pub rd: Register,
                pub rs: Register,
                pub imm: i32,
            }
        });
    };
}

/// Jump if equal zero
#[derive(Debug, Clone, Inst, Eq, PartialEq)]
#[is_branch]
pub struct Beqz {
    pub rs: Register,
    pub label: String,
}

/// Jump if not equal zero
#[derive(Debug, Clone, Inst, Eq, PartialEq)]
#[is_branch]
pub struct Bnez {
    pub rs: Register,
    pub label: String,
}

#[derive(Debug, Clone, Inst, Eq, PartialEq)]
#[asm_name = "j"]
#[is_branch]
pub struct Jmp {
    pub label: String,
}

#[derive(Debug, Clone, Inst, Eq, PartialEq)]
pub struct Call {
    pub label: String,
}

#[derive(Debug, Clone, Inst, Eq, PartialEq)]
pub struct Ret;

/// Load word
#[derive(Debug, Clone, Eq, PartialEq)]
pub struct Lw {
    pub rd: Register,
    pub offset: i32,
    pub rs: Register,
}

impl Inst for Lw {
    fn dump(&self) -> String {
        if between!(-2048, self.offset, 2047) {
            format!("lw {}, {}({})", self.rd, self.offset, self.rs)
        } else {
            let li = LoadImm {
                rd: self.rd.clone(),
                imm: self.offset,
            };
            let add = Add {
                rd: self.rd.clone(),
                rs1: self.rs.clone(),
                rs2: self.rd.clone(),
            };
            let lw = Lw {
                rd: self.rd.clone(),
                offset: 0,
                rs: self.rd.clone(),
            };
            format!("{}\n{}\n{}", li.dump(), add.dump(), lw.dump())
        }
    }

    fn is_branch(&self) -> bool {
        false
    }
}

/// Store word
///
/// rs1 -> offset(rs2)
#[derive(Debug, Clone, Eq, PartialEq)]
pub struct Sw {
    pub rs1: Register,
    pub offset: i32,
    pub rs2: Register,
}

impl Inst for Sw {
    fn dump(&self) -> String {
        if between!(-2048, self.offset, 2047) {
            format!("sw {}, {}({})", self.rs1, self.offset, self.rs2)
        } else {
            let li = LoadImm {
                rd: T0,
                imm: self.offset,
            };
            let add = Add {
                rd: T0,
                rs1: self.rs2.clone(),
                rs2: T0,
            };
            let sw = Sw {
                rs1: self.rs1.clone(),
                offset: 0,
                rs2: T0,
            };
            format!("{}\n{}\n{}", li.dump(), add.dump(), sw.dump())
        }
    }

    fn is_branch(&self) -> bool {
        false
    }
}

eval_inst_with_imm!(Add);

eval_inst!(Sub);

/// If rs1 < rs2, rd = 1; else rd = 0
#[derive(Debug, Clone, Inst, Eq, PartialEq)]
#[asm_name = "slt"]
pub struct SetLt {
    pub rd: Register,
    pub rs1: Register,
    pub rs2: Register,
}

/// If rs1 > rs2, rd = 1; else rd = 0
#[derive(Debug, Clone, Inst, Eq, PartialEq)]
#[asm_name = "sgt"]
pub struct SetGt {
    pub rd: Register,
    pub rs1: Register,
    pub rs2: Register,
}

/// If rs == 0, rd = 1; else rd = 0
#[derive(Debug, Clone, Inst, Eq, PartialEq)]
#[asm_name = "seqz"]
pub struct SetZero {
    pub rd: Register,
    pub rs: Register,
}

/// If rs != 0, rd = 1; else rd = 0
#[derive(Debug, Clone, Inst, Eq, PartialEq)]
#[asm_name = "snez"]
pub struct SetNonZero {
    pub rd: Register,
    pub rs: Register,
}

eval_inst_with_imm!(Or);
eval_inst_with_imm!(Xor);
eval_inst_with_imm!(And);

eval_inst!(Sll);
eval_inst!(Srl);
eval_inst!(Sra);

eval_inst!(Mul);
eval_inst!(Div);
eval_inst!(Rem);

#[derive(Debug, Clone, Inst, Eq, PartialEq)]
#[asm_name = "li"]
pub struct LoadImm {
    pub rd: Register,
    pub imm: i32,
}

#[derive(Debug, Clone, Inst, Eq, PartialEq)]
#[asm_name = "la"]
pub struct LoadLabel {
    pub rd: Register,
    pub label: String,
}

#[derive(Debug, Clone, Inst, Eq, PartialEq)]
#[asm_name = "mv"]
pub struct Move {
    pub rd: Register,
    pub rs: Register,
}
