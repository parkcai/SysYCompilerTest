use crate::back::inst::*;

pub struct AsmProgram {
    pub(crate) func: Vec<AsmFunc>,
    data: Vec<AsmData>,
}

pub struct AsmData {
    pub name: String,
    pub size: i32,
    pub init: Vec<i32>,
}

pub struct AsmFunc {
    pub(crate) name: String,
    pub(crate) args: Vec<String>,
    pub(crate) block: Vec<AsmBlock>,
}

pub struct AsmBlock {
    pub name: String,
    pub(crate) inst: Vec<Inst>,
}

impl AsmBlock {
    pub(crate) fn new(name: String) -> Self {
        Self {
            inst: Vec::new(),
            name,
        }
    }
}

impl AsmProgram {
    pub(crate) fn new() -> Self {
        Self {
            func: Vec::new(),
            data: Vec::new(),
        }
    }

    pub(crate) fn add_func(&mut self, func: AsmFunc) -> &AsmFunc {
        self.func.push(func);
        self.func.last().unwrap()
    }

    pub(crate) fn add_data(&mut self, data: AsmData) {
        self.data.push(data);
    }

    pub fn dump(&self) -> String {
        let mut s = String::new();
        // .data
        if !self.data.is_empty() {
            s = s + &".data\n".to_string();
            for data in &self.data {
                s = s + &format!(".globl {}\n", data.name);
                s = s + &format!("{}:\n", data.name);
                if data.init.is_empty() {
                    s = s + &format!(".zero {}\n", data.size);
                } else {
                    for data in &data.init {
                        s = s + &format!(".word {}\n", data);
                    }
                }
            }
        }

        // .text
        s = s + &".text\n".to_string();
        for func in &self.func {
            let name = &func.name[1..];
            s = s + &format!(".globl {}\n", name);
            for block in &func.block {
                s = s + &format!("{}:\n", block.name);
                for inst in &block.inst {
                    s = s + &inst.dump();
                }
            }
        }
        s
    }
}

impl AsmFunc {
    pub(crate) fn add_block(&mut self, block: AsmBlock) -> &AsmBlock {
        self.block.push(block);
        self.block.last().unwrap()
    }
}

impl AsmBlock {
    pub(crate) fn add_inst(&mut self, inst: Vec<Inst>) {
        for i in inst {
            self.inst.push(i);
        }
    }

    pub fn add_prologue(&mut self) {
        self.inst.insert(0, Inst::Prologue);
    }

    pub fn set_prologue(&mut self, prologue: i32, if_call: bool) {
        for i in 0..self.inst.len() {
            if let Inst::Prologue = self.inst[i] {
                if if_call {
                    self.inst.splice(
                        i..=i,
                        vec![
                            Inst::Addi {
                                rd: SP,
                                rs: SP,
                                imm: -prologue,
                                tmp: T0,
                            },
                            Inst::Sw {
                                rs: RA,
                                rd: SP,
                                offset: prologue - 4,
                                tmp: T0,
                            },
                        ],
                    );
                } else {
                    self.inst.splice(
                        i..=i,
                        vec![Inst::Addi {
                            rd: SP,
                            rs: SP,
                            imm: -prologue,
                            tmp: T0,
                        }],
                    );
                }
                break;
            }
        }
    }

    pub fn set_epilogue(&mut self, epilogue: i32, if_call: bool) {
        for i in 0..self.inst.len() {
            if let Inst::Epilogue = self.inst[i] {
                if if_call {
                    self.inst.splice(
                        i..=i,
                        vec![
                            Inst::Lw {
                                rs: SP,
                                rd: RA,
                                offset: epilogue - 4,
                                tmp: T0,
                            },
                            Inst::Addi {
                                rd: SP,
                                rs: SP,
                                imm: epilogue,
                                tmp: T0,
                            },
                        ],
                    );
                } else {
                    self.inst.splice(
                        i..=i,
                        vec![Inst::Addi {
                            rd: SP,
                            rs: SP,
                            imm: epilogue,
                            tmp: T0,
                        }],
                    );
                }
            }
        }
    }
}
