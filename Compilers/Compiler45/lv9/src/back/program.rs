use crate::back::inst::Inst;

pub trait Assembly {
    fn dump(&self) -> String;
}

enum AsmProgramItem {
    VarDecl(AsmVarDecl),
    FuncDecl(AsmFunc),
}

pub struct AsmVarDecl {
    name: String,
    size: usize,
    init: Option<Vec<i32>>,
}

pub struct AsmFunc {
    name: String,
    body: Vec<AsmBlock>,
}

pub struct AsmBlock {
    name: String,
    items: Vec<Box<dyn Inst>>,
}

pub struct AsmProgram {
    items: Vec<AsmProgramItem>,
}

impl Assembly for AsmProgram {
    fn dump(&self) -> String {
        let mut s = String::new();
        let mut funcs = String::new();
        let mut vars = String::new();
        for item in &self.items {
            match item {
                AsmProgramItem::FuncDecl(func_decl) => {
                    funcs.push_str(&func_decl.dump());
                    funcs.push_str("\n\n");
                }
                AsmProgramItem::VarDecl(var_decl) => {
                    vars.push_str(&var_decl.dump());
                }
            }
        }
        if !vars.is_empty() {
            s.push_str("\t.data\n");
            s.push_str(&vars);
        }
        s.push_str("\n\n\t.text\n");
        s.push_str(&funcs);
        s
    }
}

impl AsmProgram {
    pub fn new() -> Self {
        Self { items: Vec::new() }
    }

    pub fn add_func(&mut self, func_decl: AsmFunc) {
        self.items.push(AsmProgramItem::FuncDecl(func_decl));
    }

    pub fn add_var_decl(&mut self, var_decl: AsmVarDecl) {
        self.items.push(AsmProgramItem::VarDecl(var_decl));
    }
}

impl Assembly for AsmFunc {
    fn dump(&self) -> String {
        let mut s = String::new();
        s.push_str(&format!("\t.globl {}\n", self.name));
        s.push_str(&format!("{}:\n", self.name));
        for block in &self.body {
            s.push_str(&block.dump());
            s.push_str("\n");
        }
        s
    }
}

impl Assembly for AsmVarDecl {
    fn dump(&self) -> String {
        let mut s = String::new();
        s.push_str(&format!("\t.globl {}\n", self.name));
        s.push_str(&format!("{}:\n", self.name));
        if let Some(init) = &self.init {
            if init.iter().all(|x| *x == 0) {
                s.push_str(&format!("\t.zero {}\n", self.size));
                return s;
            }
            for (i, val) in init.iter().enumerate() {
                s.push_str(&format!("\t.word {}\n", val));
                if i == self.size - 1 {
                    break;
                }
            }
        } else {
            s.push_str(&format!("\t.zero {}\n", self.size));
        }
        s
    }
}

impl AsmFunc {
    pub fn new(name: String) -> Self {
        Self {
            name,
            body: Vec::new(),
        }
    }

    pub fn add_block(&mut self, block: AsmBlock) -> &mut AsmBlock {
        self.body.push(block);
        self.body.last_mut().unwrap()
    }

    pub fn add_first_block(&mut self, block: AsmBlock) -> &mut AsmBlock {
        self.body.insert(0, block);
        self.body.first_mut().unwrap()
    }

    pub fn blocks_mut(&mut self) -> &mut [AsmBlock] {
        &mut self.body
    }
}

impl AsmVarDecl {
    pub fn new(name: String, size: usize, init: Option<Vec<i32>>) -> Self {
        Self { name, size, init }
    }
}

impl Assembly for AsmBlock {
    fn dump(&self) -> String {
        let mut s = String::new();
        s.push_str(&format!("{}:\n", self.name));
        for item in &self.items {
            s.push_str(&format!("\t{}\n", item.dump()));
        }
        s
    }
}

impl AsmBlock {
    pub fn new(name: String) -> Self {
        Self {
            name,
            items: Vec::new(),
        }
    }

    pub fn add_inst_before_branch<T: Inst + 'static>(&mut self, inst: T) {
        let mut insert_pos = self.items.len();
        for inst in self.items.iter().rev() {
            if inst.is_branch() {
                insert_pos -= 1
            } else {
                break;
            }
        }
        self.items.insert(insert_pos, Box::new(inst));
    }

    pub fn add_insts(&mut self, insts: Vec<Box<dyn Inst>>) {
        self.items.extend(insts);
    }

    pub fn add_insts_in_pos(&mut self, pos: usize, insts: Vec<Box<dyn Inst>>) {
        self.items.splice(pos..pos, insts);
    }

    pub fn contains<T: Inst>(&self, inst: &T) -> Option<usize> {
        for (i, item) in self.items.iter().enumerate() {
            if item.dump().eq(&inst.dump()) {
                return Some(i);
            }
        }
        None
    }
}
