use std::env::args;

pub(crate) struct Params {
    pub input: String,
    pub output: String,
    pub koopa: bool,
    pub riscv: bool,
    pub perf: bool,
}

impl Params {
    pub fn parse() -> Params {
        let mut args = args();
        args.next();
        let mut input = String::new();
        let mut output = String::new();
        let mut koopa = false;
        let mut riscv = false;
        let mut perf = false;
        while let Some(arg) = args.next() {
            match arg.as_str() {
                "-o" => {
                    output = args.next().unwrap();
                }
                "-koopa" => {
                    koopa = true;
                }
                "-riscv" => {
                    riscv = true;
                }
                "-perf" => {
                    perf = true;
                }
                _ => {
                    input = arg;
                }
            }
        }
        Params {
            input,
            output,
            koopa,
            riscv,
            perf,
        }
    }
}