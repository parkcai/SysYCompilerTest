use koopa::ir::Program;
use super::{is_array_related_type, RiscvGenerate, RiscvGenerateResult, RiscvGeneratorContext, RiscvSymbolTableEntry};

impl RiscvGenerate for Program {
    fn riscv_generate(&self, context: &mut RiscvGeneratorContext) -> Result<RiscvGenerateResult, std::io::Error> {
        for &function in self.func_layout() {
            let func_data = self.func(function);
            context.riscv_function_table.insert(function, String::from(func_data.name())[1..].to_string(), func_data.params().len());
        }

        context.output_str.push_str("    .data\n");
        for &global in self.inst_layout() {
            let global_data = self.borrow_value(global);
            let is_global_array = is_array_related_type(global_data.ty()).0;
            let global_variable_name = global_data.name().as_ref().unwrap()[1..].to_string(); // 取掉前缀@
            // println!("name = {}, value = {:?}, is_global_array = {}", global_variable_name, global, is_global_array);
            context.riscv_symbol_table_stack.insert_new_symbol(&mut context.register_manager, &global, Some(RiscvSymbolTableEntry::GlobalVariable(global_variable_name, is_global_array)), false, None);
            
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