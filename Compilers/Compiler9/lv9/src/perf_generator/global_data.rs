use koopa::ir::{entities::ValueData, values::Aggregate, ValueKind::{self, GlobalAlloc}};

use super::{RiscvGenerate, RiscvGenerateResult, RiscvGeneratorContext};

impl RiscvGenerate for ValueData {
    fn riscv_generate(&self, context: &mut RiscvGeneratorContext) -> Result<RiscvGenerateResult, std::io::Error> {
        let variable_name = self.name().as_ref().unwrap()[1..].to_string(); // 取掉前缀@

        context.output_str.push_str(format!(
            "    .globl {}\n", 
            variable_name,
        ).as_str());
        context.output_str.push_str(format!(
            "{}:\n", 
            variable_name,
        ).as_str());

        if let GlobalAlloc(global) = self.kind() {
            let init_value_data = context.program.borrow_value(global.init());
            match init_value_data.kind() {
                ValueKind::ZeroInit(_) => {
                    context.output_str.push_str(
                        format!(
                            "    .zero {}\n",
                            init_value_data.ty().size()
                        ).as_str()
                    );
                },
                ValueKind::Integer(int) => {
                    let int_value = int.value();
                    context.output_str.push_str(format!(
                        "    .word {}\n", 
                        int_value,
                    ).as_str());
                },
                ValueKind::Aggregate(aggregate) => {
                    init_global_array(context, aggregate);
                },
                _ => {
                    panic!("Unknown Global variable initialization kind");
                }
            }
        }
        else {
            panic!("Unkown global variable kind");
        }
        Ok(RiscvGenerateResult::Ok)
    }
}

fn init_global_array(context: &mut RiscvGeneratorContext, aggregate: &Aggregate) {
    for ele in aggregate.elems() {
        match context.program.borrow_value(*ele).kind() {
            ValueKind::Integer(int) => {
                let int_value = int.value();
                context.output_str.push_str(
                    format!(
                        "    .word {}\n", 
                        int_value,
                    ).as_str()
                );
            },
            ValueKind::Aggregate(aggregate) => {
                init_global_array(context, aggregate);
            },
            _ => {
                panic!("Array initialization error");
            }
        }
    }
}