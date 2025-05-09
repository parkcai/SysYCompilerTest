#[derive(Clone)]
pub struct BlockIdGenerator {
    max_id: i32,
    id_stack: Vec<i32>,
}

impl BlockIdGenerator {
    pub fn new() -> Self {
        BlockIdGenerator {
            max_id: 0,
            id_stack: vec![0],
        }
    }

    pub fn generate(&mut self) -> i32 {
        self.max_id += 1;
        self.id_stack.push(self.max_id);
        self.max_id
    }

    pub fn get_current_id(&self) -> i32 {
        self.id_stack[self.id_stack.len() - 1]
    }

    pub fn pop(&mut self) {
        self.id_stack.pop();
    }
}

#[derive(Clone)]
pub struct ParserContext<'a> {
    pub generator: BlockIdGenerator,
    pub file_path: &'a str,
    pub input: &'a str,
}

impl<'a> ParserContext<'a> {
    pub fn new(file_path: &'a str, input: &'a str) -> Self {
        ParserContext {
            generator: BlockIdGenerator::new(),
            file_path,
            input,
        }
    }
}
