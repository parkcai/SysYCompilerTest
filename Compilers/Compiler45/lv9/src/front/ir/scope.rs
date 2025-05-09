use crate::front::ident::Identifier;
use std::collections::HashMap;

pub type Result = std::result::Result<(), String>;

type IdentifierMap = HashMap<String, Identifier>;

#[derive(Debug)]
pub struct Scope {
    stack: Vec<(i32, IdentifierMap)>,
}

impl Scope {
    pub fn new() -> Scope {
        let mut scope = Scope { stack: Vec::new() };
        scope.go_into_scoop(0);
        scope
    }

    pub fn go_into_scoop(&mut self, scope_id: i32) {
        self.stack.push((scope_id, HashMap::new()));
    }

    pub fn go_out_scoop(&mut self) {
        self.stack.pop();
    }

    pub fn current_scope_id(&self) -> i32 {
        self.stack.last().unwrap().0
    }

    pub fn get_identifier(&self, name: &str) -> Option<&Identifier> {
        self.stack
            .iter()
            .rev()
            .find_map(|(_, identifiers)| identifiers.get(name))
    }

    pub fn add_identifier(&mut self, name: String, identifier: Identifier) -> Result {
        let scope_id = self.current_scope_id();
        let (_, identifiers) = self.stack.last_mut().unwrap();
        if let Some(_) = identifiers.get(&name) {
            return Err(format!(
                "Identifier {} is already defined in scope {}",
                name, scope_id
            ));
        }
        identifiers.insert(name, identifier);
        Ok(())
    }
}
