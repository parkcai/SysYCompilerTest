#[derive(Debug, Clone, Eq, PartialEq)]
pub struct Register {
    pub name: &'static str,
}

pub const ZERO: Register = Register { name: "zero" };
pub const RA: Register = Register { name: "ra" };
pub const SP: Register = Register { name: "sp" };
pub const GP: Register = Register { name: "gp" };
pub const TP: Register = Register { name: "tp" };
pub const T0: Register = Register { name: "t0" };
pub const T1: Register = Register { name: "t1" };
pub const T2: Register = Register { name: "t2" };
pub const FP: Register = Register { name: "fp" };
pub const S1: Register = Register { name: "s1" };
pub const A0: Register = Register { name: "a0" };
pub const A1: Register = Register { name: "a1" };
pub const A2: Register = Register { name: "a2" };
pub const A3: Register = Register { name: "a3" };
pub const A4: Register = Register { name: "a4" };
pub const A5: Register = Register { name: "a5" };
pub const A6: Register = Register { name: "a6" };
pub const A7: Register = Register { name: "a7" };
pub const S2: Register = Register { name: "s2" };
pub const S3: Register = Register { name: "s3" };
pub const S4: Register = Register { name: "s4" };
pub const S5: Register = Register { name: "s5" };
pub const S6: Register = Register { name: "s6" };
pub const S7: Register = Register { name: "s7" };
pub const S8: Register = Register { name: "s8" };
pub const S9: Register = Register { name: "s9" };
pub const S10: Register = Register { name: "s10" };
pub const S11: Register = Register { name: "s11" };
pub const T3: Register = Register { name: "t3" };
pub const T4: Register = Register { name: "t4" };
pub const T5: Register = Register { name: "t5" };
pub const T6: Register = Register { name: "t6" };

pub enum Operand {
    Register(Register),
    Immediate(i32),
}

impl Operand {
    fn to_string(&self) -> String {
        match self {
            Operand::Register(reg) => reg.name.to_string(),
            Operand::Immediate(imm) => imm.to_string(),
        }
    }
}

#[derive(Debug, Clone, Eq, PartialEq)]
pub enum Inst {
    Ret,
    Li {
        rd: Register,
        imm: i32,
    },
    La {
        rd: Register,
        label: String,
    },
    Mv {
        rd: Register,
        rs: Register,
    },
    OP2 {
        op: String,
        rd: Register,
        rs1: Register,
        rs2: Register,
    },
    OP1 {
        op: String,
        rd: Register,
        rs: Register,
    },
    Addi {
        rd: Register,
        rs: Register,
        imm: i32,
        tmp: Register,
    },
    Slli {
        rd: Register,
        rs: Register,
        imm: i32,
    },
    Lw {
        rd: Register,
        rs: Register,
        offset: i32,
        tmp: Register,
    },
    Sw {
        rs: Register,
        rd: Register,
        offset: i32,
        tmp: Register,
    },
    Beqz {
        rs: Register,
        label: String,
    },
    Bnez {
        rs: Register,
        label: String,
    },
    J {
        label: String,
    },
    Call {
        label: String,
    },
    Prologue,
    Epilogue,
}

impl Inst {
    pub fn dump(&self) -> String {
        match self {
            Inst::Ret => "ret\n".to_string(),
            Inst::Li { rd, imm } => format!("li {}, {}\n", rd.name, imm),
            Inst::La { rd, label } => format!("la {}, {}\n", rd.name, label),
            Inst::Mv { rd, rs } => format!("mv {}, {}\n", rd.name, rs.name),
            Inst::OP2 { op, rd, rs1, rs2 } => {
                format!("{} {}, {}, {}\n", op, rd.name, rs1.name, rs2.name)
            }
            Inst::OP1 { op, rd, rs } => format!("{} {}, {}\n", op, rd.name, rs.name),
            Inst::Addi { rd, rs, imm, tmp } => {
                if (-2048..=2047).contains(imm) {
                    format!("addi {}, {}, {}\n", rd.name, rs.name, imm)
                } else {
                    format!(
                        "li {}, {}\nadd {}, {}, {}\n",
                        tmp.name, imm, rd.name, rs.name, tmp.name
                    )
                }
            }
            Inst::Slli { rd, rs, imm } => format!("slli {}, {}, {}\n", rd.name, rs.name, imm),
            Inst::Lw {
                rd,
                rs,
                offset,
                tmp,
            } => {
                if (-2048..=2047).contains(offset) {
                    format!("lw {}, {}({})\n", rd.name, offset, rs.name)
                } else {
                    format!(
                        "li {}, {}\nadd {}, {}, {}\nlw {}, 0({})\n",
                        tmp.name, offset, tmp.name, rs.name, tmp.name, rd.name, tmp.name
                    )
                }
            }
            Inst::Sw {
                rs,
                rd,
                offset,
                tmp,
            } => {
                if (-2048..=2047).contains(offset) {
                    format!("sw {}, {}({})\n", rs.name, offset, rd.name)
                } else {
                    format!(
                        "li {}, {}\nadd {}, {}, {}\nsw {}, 0({})\n",
                        tmp.name, offset, tmp.name, rd.name, tmp.name, rs.name, tmp.name
                    )
                }
            }
            Inst::Beqz { rs, label } => format!("beqz {}, {}\n", rs.name, label),
            Inst::Bnez { rs, label } => format!("bnez {}, {}\n", rs.name, label),
            Inst::J { label } => format!("j {}\n", label),
            Inst::Call { label } => format!("call {}\n", label),
            Inst::Prologue => String::new(),
            Inst::Epilogue => String::new(),
        }
    }
}
