use crate::util::logger::show_error;
use std::env::args;

#[derive(Debug, Clone)]
pub struct Params {
    pub output: String,
    pub input: String,

    pub koopa: bool,
    pub riscv: bool,
    pub perf: bool,
}

impl Params {
    pub fn parse() -> Self {
        let mut args = args();
        let (mut input, mut output, mut koopa, mut riscv, mut perf) =
            (String::new(), String::new(), false, false, false);
        args.next();
        while let Some(arg) = args.next() {
            match arg.as_str() {
                "-o" => {
                    if let Some(output_path) = args.next() {
                        output = output_path;
                    } else {
                        show_error("missing output file", 1);
                    }
                }
                "-koopa" => koopa = true,
                "-riscv" => riscv = true,
                "-perf" => perf = true,
                _ => {
                    if input.is_empty() {
                        input = arg;
                    } else {
                        show_error("multiple input files", 1);
                    }
                }
            }
        }
        if input.is_empty() {
            show_error("missing input file", 1);
        }
        if output.is_empty() {
            show_error("missing output file", 1);
        }
        if !koopa && !riscv && !perf {
            show_error("no output format specified", 1);
        }
        if (koopa && riscv) || (koopa && perf) || (riscv && perf) {
            show_error("multiple output formats specified", 1);
        }
        Params {
            output,
            input,
            koopa,
            riscv,
            perf,
        }
    }
}
