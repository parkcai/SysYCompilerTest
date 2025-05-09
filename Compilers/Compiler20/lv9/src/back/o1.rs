use crate::back::asm::AsmProgram;
use crate::back::inst::{Inst, T2};

pub fn peephole_opt(asm_program: &mut AsmProgram) {
    for func in &mut asm_program.func {
        for block in &mut func.block {
            let mut to_delete = Vec::new();
            for (i, inst) in block.inst.iter().enumerate() {
                match inst {
                    Inst::Sw {
                        rs,
                        rd,
                        offset,
                        tmp,
                    } => {
                        if let Inst::Lw {
                            rs: rs2,
                            rd: rd2,
                            offset: offset2,
                            tmp: tmp2,
                        } = &block.inst[i + 1]
                        {
                            if rs.clone() == rd2.clone()
                                && rd.clone() == rs2.clone()
                                && offset.clone() == offset2.clone()
                                && tmp.clone() == tmp2.clone()
                            {
                                to_delete.push(i + 1);
                            }
                        }
                    }
                    _ => {}
                }
            }
            for i in to_delete.iter().rev() {
                block.inst.remove(*i);
            }
            let mut change_to_i = Vec::new();
            for (i, inst) in block.inst.iter().enumerate() {
                match inst {
                    Inst::Li { rd, imm } => {
                        if let Inst::OP2 {
                            op,
                            rd: rd2,
                            rs1,
                            rs2,
                        } = &block.inst[i + 1]
                        {
                            if op == "add" && rs2.clone() == rd.clone() {
                                change_to_i.push(i);
                            }
                        }
                    }
                    _ => {}
                }
            }
            for i in change_to_i.iter().rev() {
                let imm = match &block.inst[*i] {
                    Inst::Li { rd, imm } => imm.clone(),
                    _ => unreachable!(),
                };
                let rs1 = match &block.inst[*i + 1] {
                    Inst::OP2 { op, rd, rs1, rs2 } => rs1.clone(),
                    _ => unreachable!(),
                };
                let rd = match &block.inst[*i + 1] {
                    Inst::OP2 { op, rd, rs1, rs2 } => rd.clone(),
                    _ => unreachable!(),
                };
                block.inst[*i] = Inst::Addi {
                    rd: rd.clone(),
                    rs: rs1.clone(),
                    imm,
                    tmp: T2,
                };
                block.inst.remove(*i + 1);
            }
        }
    }
}

pub fn is_pow_of_two(x: i32) -> Option<i32> {
    if x == 0 {
        return None;
    }
    let mut y = x;
    while y % 2 == 0 {
        y /= 2;
    }
    if y == 1 {
        return Some(x.trailing_zeros() as i32);
    }
    None
}
