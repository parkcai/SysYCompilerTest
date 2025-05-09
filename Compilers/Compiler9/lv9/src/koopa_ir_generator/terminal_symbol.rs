use crate::ast_def::terminal_symbol::*;

impl std::fmt::Display for IDENT {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        match self {
            IDENT::IDENT(ident) => write!(f, "{}", ident)?,
        };
        Ok(())
    }
}

impl std::fmt::Display for BType {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        match self {
            BType::int => write!(f, "i32")?,
            BType::void => write!(f, "")?,
        };
        Ok(())
    }
}

impl std::fmt::Display for INT_CONST {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        match self {
            INT_CONST::Value(int_const) => write!(f, "{}", int_const)?,
        };
        Ok(())
    }
}