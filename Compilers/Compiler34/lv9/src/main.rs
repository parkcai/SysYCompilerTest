use std::{env, error::Error, process};
use rust_ver::{Config, run};

pub mod ast;

fn main() -> Result<(), Box<dyn Error>> {
    let args: Vec<String> = env::args().collect();
    let config = Config::build(&args).unwrap_or_else(|err|{
        eprintln!("{err}");
        process::exit(1);
    });
    
    run(config)
}