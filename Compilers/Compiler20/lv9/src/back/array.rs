use koopa::ir::values::Aggregate;
use koopa::ir::{Program, ValueKind};

pub fn get_init_list(program: &Program, aggregate: Aggregate) -> Vec<i32> {
    let mut init_list: Vec<i32> = Vec::new();
    for value in aggregate.elems() {
        let value_data = program.borrow_value(value.clone());
        match value_data.kind() {
            ValueKind::Integer(i) => {
                init_list.push(i.value());
            }
            ValueKind::Aggregate(a) => {
                init_list.append(&mut get_init_list(program, a.clone()));
            }
            _ => unreachable!(),
        }
    }
    init_list
}
