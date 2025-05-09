use koopa::ir::{Type, TypeKind};

pub mod args;
pub mod logger;

pub fn remove_pointer(ty: Type) -> Type {
    match ty.kind() {
        TypeKind::Pointer(t) => t.clone(),
        _ => ty,
    }
}
