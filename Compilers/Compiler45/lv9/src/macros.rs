#[macro_export]
macro_rules! new_bb {
    ($func:expr) => {
        $func.dfg_mut().new_bb()
    };
}

#[macro_export]
macro_rules! new_value {
    ($func:expr) => {
        $func.dfg_mut().new_value()
    };
}

#[macro_export]
macro_rules! add_bb {
    ($func:expr, $bb:expr) => {
        $func.layout_mut().bbs_mut().push_key_back($bb).unwrap()
    };
}

#[macro_export]
macro_rules! add_inst {
    ($func:expr, $bb:expr, $inst:expr) => {
        $func
            .layout_mut()
            .bb_mut($bb)
            .insts_mut()
            .push_key_back($inst)
            .unwrap()
    };
}

#[macro_export]
macro_rules! between {
    ($min:expr, $val:expr, $max:expr) => {
        $val >= $min && $val <= $max
    };
}
