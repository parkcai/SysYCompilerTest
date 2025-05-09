use ast::{KoopaFunctionCounter, KoopaFunctionData, ToKoopa};
use lalrpop_util::lalrpop_mod;
use risc_v::parse_ir;
use std::{cell::RefCell, collections::HashMap, error::Error, fs, rc::Rc};
lalrpop_mod!(pub sysy); // synthesized by LALRPOP
pub mod ast;
pub mod risc_v;

pub struct Config<'a> {
    mode: &'a str,
    input_file_path: &'a str,
    output_file_path: &'a str,
}

impl<'a> Config<'a> {
    pub fn build(args: &[String]) -> Result<Config, &'static str> {
        if args.len() < 5 {
            return Err("Need more arguments.");
        }
        Ok(Config {
            mode: &args[1],
            input_file_path: &args[2],
            output_file_path: &args[4],
        })
    }
}

pub fn run(config: Config) -> Result<(), Box<dyn Error>> {
    let input_file = fs::read_to_string(config.input_file_path)?;
    let ast = sysy::CompUnitParser::new().parse(&input_file).unwrap();
    //println!("{:#?}", ast);
    let mut koopa_function = KoopaFunctionData::new(HashMap::new(), Option::None, &KoopaFunctionCounter::new());
    koopa_function.insert_symbol(format!("getint"), ast::TypedValue::IntFunc(format!("@getint")));
    koopa_function.insert_symbol(format!("getch"), ast::TypedValue::IntFunc(format!("@getch")));
    koopa_function.insert_symbol(format!("getarray"), ast::TypedValue::IntFunc(format!("@getarray")));
    koopa_function.insert_symbol(format!("putint"), ast::TypedValue::VoidFunc(format!("@putint")));
    koopa_function.insert_symbol(format!("putch"), ast::TypedValue::VoidFunc(format!("@putch")));
    koopa_function.insert_symbol(format!("putarray"), ast::TypedValue::VoidFunc(format!("@putarray")));
    koopa_function.insert_symbol(format!("starttime"), ast::TypedValue::VoidFunc(format!("@starttime")));
    koopa_function.insert_symbol(format!("stoptime"), ast::TypedValue::VoidFunc(format!("@stoptime")));
    let res = ast.to_koopa(Rc::new(RefCell::new(koopa_function)))?;
    //println!("{}", &res);
    match config.mode {
        "-koopa" => fs::write(config.output_file_path, res.clone()).unwrap(),
        _ => ()
    }
    //fs::write(config.output_file_path, res).unwrap();
    let driver = koopa::front::Driver::from(res);
    let program = driver.generate_program().unwrap();
    let risc_v = parse_ir(&program)?;
    //println!("{}", &risc_v);
    match config.mode {
        "-riscv" => fs::write(config.output_file_path, risc_v).unwrap(),
        "-perf" => fs::write(config.output_file_path, risc_v).unwrap(),
        _ => ()
    }
    Ok(())
}
