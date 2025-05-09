pub enum Operator{
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    Eq,
    Ne,
    Gt,
    Lt,
    Ge,
    Le,
    And,
    Or,
    Xor,
    Shl,
    Shr,
    Sar,
}
pub enum Parameter{
    ConstInt(i32),
    ConstStr(String),
}

pub fn parse_parameter(parameter:&Parameter) -> String{
    match parameter {
        Parameter::ConstInt(int) => int.to_string(),
        Parameter::ConstStr(str) => str.clone(),
    }
}

pub fn parse_binary(name:&str, parameters:&[Parameter]) -> String{
    format!("{} {}, {}", name, parse_parameter(&parameters[0]), parse_parameter(&parameters[1]))
}

//get the name of operator and the Parameters of the operator, return a String they combine as KoopaIR
pub fn parse_operator(operator:&Operator, parameters:&[Parameter]) -> String {
    match operator {
        Operator::Sub => parse_binary("sub", parameters),
        Operator::Eq => parse_binary("eq", parameters),
        Operator::Add => parse_binary("add", parameters),
        Operator::Mul => parse_binary("mul", parameters),
        Operator::Div => parse_binary("div", parameters),
        Operator::Mod => parse_binary("mod", parameters),
        Operator::Ne => parse_binary("ne", parameters),
        Operator::Ge => parse_binary("ge", parameters),
        Operator::Gt => parse_binary("gt", parameters),
        Operator::Le => parse_binary("le", parameters),
        Operator::Lt => parse_binary("lt", parameters),
        Operator::And => parse_binary("and", parameters),
        Operator::Or => parse_binary("or", parameters),
        Operator::Xor => parse_binary("xor", parameters),
        _ => unreachable!()
        
    }

}