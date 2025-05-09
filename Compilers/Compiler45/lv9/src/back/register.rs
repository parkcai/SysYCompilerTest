#![allow(unused)]

use std::fmt::Display;

#[derive(Debug, Clone, Copy, Hash, Eq, PartialEq)]
pub struct Register {
    name: &'static str,
}

impl Display for Register {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.name.to_string())
    }
}

macro_rules! reg {
    ($name:ident, $display_name:ident) => {
        pub const $name: Register = Register {
            name: stringify!($display_name),
        };
    };
}

macro_rules! regs {
    ($($name:ident, $display_name:ident);* $(;)?) => {
        $(reg!($name, $display_name);)*
    };
}

regs!(
    ZERO, zero;
    RA, ra;
    SP, sp;
    GP, gp;
    TP, tp;
    T0, t0;
    T1, t1;
    T2, t2;
    FP, fp;
    S1, s1;
    A0, a0;
    A1, a1;
    A2, a2;
    A3, a3;
    A4, a4;
    A5, a5;
    A6, a6;
    A7, a7;
    S2, s2;
    S3, s3;
    S4, s4;
    S5, s5;
    S6, s6;
    S7, s7;
    S8, s8;
    S9, s9;
    S10, s10;
    S11, s11;
    T3, t3;
    T4, t4;
    T5, t5;
    T6, t6;
);
