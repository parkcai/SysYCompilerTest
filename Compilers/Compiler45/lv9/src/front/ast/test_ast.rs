use super::*;
use crate::front::parser_context::ParserContext;
use crate::parser;

#[test]
fn test_number() {
    let number_parser = parser::NumberParser::new();

    let context = ParserContext::new("", "");

    assert_eq!(number_parser.parse(&mut context.clone(), "123"), Ok(123));
    assert_eq!(number_parser.parse(&mut context.clone(), "0"), Ok(0));
    assert_eq!(number_parser.parse(&mut context.clone(), "010"), Ok(8));
    assert_eq!(number_parser.parse(&mut context.clone(), "0x10"), Ok(16));

    assert!(number_parser.parse(&mut context.clone(), "123a").is_err());
}

#[test]
fn test_identifier() {
    let identifier_parser = parser::IdentParser::new();

    let context = ParserContext::new("", "");

    assert_eq!(
        identifier_parser.parse(&mut context.clone(), "a"),
        Ok("a".to_string())
    );

    assert_eq!(
        identifier_parser.parse(&mut context.clone(), "a0"),
        Ok("a0".to_string())
    );

    assert!(identifier_parser.parse(&mut context.clone(), "0a").is_err());
    assert!(identifier_parser.parse(&mut context.clone(), "_a").is_ok());
    assert!(identifier_parser.parse(&mut context.clone(), "a_").is_ok());
}

#[test]
fn test_expr() {
    let expr_parser = parser::ExprParser::new();
    let mut context = ParserContext::new("", "");

    let input = "1 + 2 * -a";
    let result = expr_parser.parse(&mut context, input);
    assert_eq!(
        result,
        Ok(Expr(Rc::new(LOrExpr::LAndExpr(LAndExpr::EqExpr(
            EqExpr::RelExpr(RelExpr::AddExpr(AddExpr::Add(
                Rc::new(AddExpr::MulExpr(MulExpr::UnaryExpr(
                    UnaryExpr::PrimaryExpr(PrimaryExpr::Number(1))
                ))),
                AddOp::Add,
                Rc::new(MulExpr::Mul(
                    Rc::new(MulExpr::UnaryExpr(UnaryExpr::PrimaryExpr(
                        PrimaryExpr::Number(2)
                    ))),
                    MulOp::Mul,
                    Rc::new(UnaryExpr::Unary(
                        UnaryOp::Neg,
                        Rc::new(UnaryExpr::PrimaryExpr(PrimaryExpr::LVal(LVal::Var(
                            "a".to_string()
                        ))))
                    ))
                ))
            )))
        )))))
    );
}

fn build_number(n: i32) -> LOrExpr {
    LOrExpr::LAndExpr(LAndExpr::EqExpr(EqExpr::RelExpr(RelExpr::AddExpr(
        AddExpr::MulExpr(MulExpr::UnaryExpr(UnaryExpr::PrimaryExpr(
            PrimaryExpr::Number(n),
        ))),
    ))))
}

#[test]
fn test_comp_unit() {
    let comp_unit_parser = parser::CompUnitParser::new();
    let mut context = ParserContext::new("", "");

    let input = r#"
    // This is a comment.
    int a = 10;
    const int b[10][15] = {1};
    /*
    This is a block comment.
    */
    void f() {
        int c = 30;
    }

    int g() {
        return 0;
    }
    "#;

    let result = comp_unit_parser.parse(&mut context, input);

    assert!(result.is_ok());
    let result = result.unwrap();
    assert_eq!(result.items.len(), 4);
    let items = result.items;
    assert_eq!(
        items[0],
        GlobalItem::Decl(Decl::VarDecl(vec![Rc::new(VarDef::NormalVarDef(
            NormalVarDef {
                name: "a".to_string(),
                value: Some(Expr(Rc::new(build_number(10)))),
            }
        ))]))
    );
    assert_eq!(
        items[1],
        GlobalItem::Decl(Decl::ConstDecl(vec![Rc::new(ConstDef::ArrayConstDef(
            ArrayConstDef {
                name: "b".to_string(),
                shape: vec![
                    ConstExpr(Rc::new(build_number(10))),
                    ConstExpr(Rc::new(build_number(15)))
                ],
                values: ConstArray::Array(vec![ConstArray::Val(ConstExpr(Rc::new(build_number(
                    1
                ))))]),
            }
        ))]))
    );
}
