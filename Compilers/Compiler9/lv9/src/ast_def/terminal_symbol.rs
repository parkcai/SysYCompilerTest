#[derive(Debug, Clone)]
#[allow(non_camel_case_types)]
pub enum BType {
    int,
    void
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
pub enum IDENT {
    IDENT(String),    
}

#[derive(Debug)]
#[allow(non_camel_case_types)]
pub enum INT_CONST {
    Value(i32),
}