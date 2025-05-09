use koopa::ir::Program;
use super::{RiscvGenerate, RiscvGenerateResult, RiscvGeneratorContext, RiscvSymbolTableEntry};

impl RiscvGenerate for Program {
    fn riscv_generate(&self, context: &mut RiscvGeneratorContext) -> Result<RiscvGenerateResult, std::io::Error> {

        // for &global in self.inst_layout() {
        //     let global_data = self.borrow_value(global);
        //     println!("global: {:?}\n", global_data);
        //     println!("value = {:?}", global);
        // }
        // println!("global variable over");

        for &function in self.func_layout() {
            let func_data = self.func(function);
            context.riscv_function_table.insert(function, String::from(func_data.name())[1..].to_string(), func_data.params().len());

            // 下面的代码是调试用的，最终要删除。
            // for param in func_data.params() {
            //     let param_data = func_data.dfg().value(*param);
            //     println!("param value = {:?}, data: {:?}\n", param, param_data);
            // }
            // println!("func: {:?}\n", func_data.name());
            // for (&_bb, node) in func_data.layout().bbs() {
            //     println!("basic block: {:?}\n", _bb);
            //     for inst in node.insts().keys() {
            //         let inst_data = func_data.dfg().value(*inst);
            //         println!("inst_data: {:?}\n    id of this instruction = {:?}\n", inst_data, inst);
            //     }
            // }
        }
        // println!("function over");

        context.output_str.push_str("    .data\n");
        for &global in self.inst_layout() {
            let global_data = self.borrow_value(global);

            let global_variable_name = global_data.name().as_ref().unwrap()[1..].to_string(); // 取掉前缀@
            context.riscv_symbol_table_stack.insert_new_symbol(&global, RiscvSymbolTableEntry::GlobalVariable(global_variable_name));
            
            global_data.riscv_generate(context)?;
        }

        context.output_str.push_str("    .text\n");
        for &function in self.func_layout() {
            let func_data = self.func(function);
            context.riscv_symbol_table_stack.push();
            func_data.riscv_generate(context)?;
            context.riscv_symbol_table_stack.pop();
        }
        Ok(RiscvGenerateResult::Ok)
    }
}