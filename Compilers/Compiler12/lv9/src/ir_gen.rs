use crate::ast::*;
use std::fmt::Write;

pub fn generate_ir(comp_unit: &CompUnit) -> String {
    let mut ir = String::new();
    let mut global_symbol_table = SymbolTable::new(); // 全局符号表
    let mut label_counter = 0; // 初始化 label 计数器

    // 预注册 SysY 库函数
    register_sysy_library(&mut ir, &mut global_symbol_table);
    
    for item in &comp_unit.items {
        match item {
            CompUnitItem::Decl(decl) => generate_global_decl(decl, &mut ir, &mut global_symbol_table,&mut label_counter),
            CompUnitItem::FuncDef(func_def) => {
                generate_func_def(func_def, &mut ir, &mut global_symbol_table, &mut label_counter);
            }
        }
    }

    postprocess_ir(&mut ir, &global_symbol_table);

    ir
}

fn register_sysy_library(ir: &mut String, global_symbol_table: &mut SymbolTable) {
    let sysy_functions = vec![
        ("getint", FuncType::Int, vec![]),
        ("getch", FuncType::Int, vec![]),
        ("getarray", FuncType::Int, vec![FuncFParam {
            btype: BType::Int,
            ident: "*i32".to_string(),
            dims: Some(vec![]),
            is_array: true,
        }]),
        ("putint", FuncType::Void, vec![FuncFParam {
            btype: BType::Int,
            ident: "i32".to_string(),
            dims: None,
            is_array: false,
        }]),
        ("putch", FuncType::Void, vec![FuncFParam {
            btype: BType::Int,
            ident: "i32".to_string(),
            dims: None,
            is_array: false,
        }]),
        ("putarray", FuncType::Void, vec![
            FuncFParam {
                btype: BType::Int,
                ident: "i32".to_string(),
                dims: None,
                is_array: false,
            },
            FuncFParam {
                btype: BType::Int,
                ident: "*i32".to_string(),
                dims: Some(vec![]),
                is_array: true,
            },
        ]),
        ("starttime", FuncType::Void, vec![]),
        ("stoptime", FuncType::Void, vec![]),
    ];

    for (name, func_type, params) in sysy_functions {
        global_symbol_table.insert_function(
            name.to_string(),
            Some(func_type.clone()),
            params.clone(),
        );

        let param_types: Vec<String> = params
            .iter()
            .map(|param| {
                if param.is_array {
                    "*i32".to_string()
                } else {
                    "i32".to_string()
                }
            })
            .collect();

        let param_str = param_types.join(", ");
        match func_type {
            FuncType::Int => writeln!(ir, "decl @{}({}): i32", name, param_str).unwrap(),
            FuncType::Void => writeln!(ir, "decl @{}({})", name, param_str).unwrap(),
        }
    }
    writeln!(ir).unwrap();
}

fn add_global_variable(
    ir: &mut String,
    global_symbol_table: &mut SymbolTable,
    ident: &str,
    size: Option<i32>,
    initval: Option<&InitVal>,
) {
    let array_type = if let Some(size) = size {
        format!("[i32, {}]", size)
    } else {
        "i32".to_string() // 非数组类型
    };

    let init_str = if let Some(initval) = initval {
        match initval {
            InitVal::Single(exp) => format!("{}", exp.eval(global_symbol_table,global_symbol_table)),
            _ => panic!("Unsupported InitVal for global variable initialization"),
        }
    } else {
        "zeroinit".to_string()
    };

    writeln!(ir, "global @{} = alloc {}, {}", ident, array_type, init_str).unwrap();
    global_symbol_table.insert_variable(ident.to_string(), format!("@{}", ident));
}

fn add_global_constant(
    ir: &mut String,
    global_symbol_table: &mut SymbolTable,
    ident: &str,
    value: i32,
) {
    //writeln!(ir, "global @{} = alloc i32, {}", ident, value).unwrap();
    global_symbol_table.insert_constant(ident.to_string(), value);
}

fn generate_global_decl(
    decl: &Decl,
    ir: &mut String,
    global_symbol_table: &mut SymbolTable,
    label_counter: &mut i32,
) {
    match decl {
        Decl::ConstDecl(const_decl) => {
            // 遍历所有常量定义，逐一调用处理函数
            process_global_const_def(&const_decl.def, ir, global_symbol_table);
            if let Some(defs) = &const_decl.defs {
                for def in defs {
                    process_global_const_def(def, ir, global_symbol_table);
                }
            }
        }
        Decl::VarDecl(var_decl) => {
            // 遍历所有变量定义，逐一调用处理函数
            process_global_var_def(&var_decl.def, ir, global_symbol_table, label_counter);
            if let Some(defs) = &var_decl.defs {
                for def in defs {
                    process_global_var_def(def, ir, global_symbol_table, label_counter);
                }
            }
        }
    }
}

/// 处理单个全局常量定义
fn process_global_const_def(
    def: &ConstDef,
    ir: &mut String,
    global_symbol_table: &mut SymbolTable,
) {
    let sizes = if let Some(size_exp) = &def.sizes {
        size_exp.iter().map(|s| s.eval(global_symbol_table,global_symbol_table) as usize).collect()
    } else {
        vec![]
    };

    if sizes.is_empty() {
        // 单个常量
        let mut align_product=vec![];
        align_product.push(1);
        let mut flat_values = vec![]; // 定义一个显式的变量来存储临时值
        let init_value = def.constinitval.eval(&sizes, global_symbol_table, &align_product, &mut flat_values,global_symbol_table);
        if init_value.len() != 1 {
            panic!(
                "Constant '{}' must have exactly one initialization value",
                def.ident
            );
        }
        add_global_constant(ir, global_symbol_table, &def.ident, init_value[0]);
    } else {
        // 数组常量
        let len=sizes.len();
        let mut align_product:Vec<usize>=Vec::new();
        //例如,a[2][3][4],align_product=[4,12,24]
        let mut now_product=1;
        for i in (0..len).rev(){
            now_product*=sizes[i];
            align_product.push(now_product);
        }
        let mut flat_values = vec![]; // 定义一个显式的变量来存储临时值
        let init_values = def.constinitval.eval(&sizes, global_symbol_table, &align_product, &mut flat_values,global_symbol_table);
        let total_size: usize = sizes.iter().product();

        if init_values.len() > total_size {
            panic!(
                "Initializer size ({}) exceeds total array size ({}) for constant '{}'",
                init_values.len(),
                total_size,
                def.ident
            );
        }

        add_global_array(
            ir,
            global_symbol_table,
            &def.ident,
            &sizes,
            Some(&init_values),
            &mut 0,
        );
    }
}

/// 处理单个全局变量定义
fn process_global_var_def(
    def: &VarDef,
    ir: &mut String,
    global_symbol_table: &mut SymbolTable,
    label_counter: &mut i32,
) {
    let sizes = if let Some(size_exp) = &def.sizes {
        size_exp.iter().map(|s| s.eval(global_symbol_table,global_symbol_table) as usize).collect()
    } else {
        vec![]
    };

    if sizes.is_empty() {
        // 单个变量
        if let Some(initval) = &def.initval {
            let mut align_product=vec![];
            align_product.push(1);
            let mut flat_values = vec![]; // 定义一个显式的变量来存储临时值
            let init_value = initval.eval(&sizes, global_symbol_table,&align_product,&mut flat_values,global_symbol_table);
            if init_value.len() != 1 {
                panic!(
                    "Variable '{}' must have exactly one initialization value",
                    def.ident
                );
            }
            add_global_variable(ir, global_symbol_table, &def.ident, None, Some(initval));
        } else {
            add_global_variable(ir, global_symbol_table, &def.ident, None, None);
        }
    } else {
        // 数组变量
        let total_size: usize = sizes.iter().product();
        let len=sizes.len();
        let mut align_product:Vec<usize>=Vec::new();
        //例如,a[2][3][4],align_product=[4,12,24]
        let mut now_product=1;
        for i in (0..len).rev(){
            now_product*=sizes[i];
            align_product.push(now_product);
        }
        let mut flat_values = vec![]; // 定义一个显式的变量来存储临时值
        let init_values = def
            .initval
            .as_ref()
            .map(|initval| initval.eval(&sizes, global_symbol_table,&align_product,&mut flat_values,global_symbol_table))
            .map(|v| &**v); // 转换为 Option<&[i32]>
            
        if let Some(init_values) = &init_values {
            if init_values.len() > total_size {
                panic!(
                    "Initializer size ({}) exceeds total array size ({}) for variable '{}'",
                    init_values.len(),
                    total_size,
                    def.ident
                );
            }
        }

        add_global_array(
            ir,
            global_symbol_table,
            &def.ident,
            &sizes,
            init_values,
            label_counter,
        );
    }
}

fn add_global_array(
    ir: &mut String,
    global_symbol_table: &mut SymbolTable,
    ident: &str,
    sizes: &[usize],
    init_values: Option<&[i32]>,
    label_counter: &mut i32,
) {
    let alloc_name = format!("@{}", ident);

    let total_size: usize = sizes.iter().product();

    // 注册到符号表
    global_symbol_table.insert_array(
        ident.to_string(),
        BType::Int,
        sizes.to_vec(),
        alloc_name.clone(),
    );

    // 初始化值处理
    if let Some(values) = init_values {
        let mut values = values.to_vec();

        // 补零填充
        while values.len() < total_size {
            values.push(0);
        }

        // 生成aggregate结构
        let mut aggregate = String::new();
        generate_aggregate(&mut aggregate, sizes, &values, 0);

        writeln!(
            ir,
            "global {} = alloc {}, {}",
            alloc_name,
            format_dimensions(sizes),
            aggregate
        )
        .unwrap();
    } else {
        writeln!(
            ir,
            "global {} = alloc {}, zeroinit",
            alloc_name,
            format_dimensions(sizes)
        )
        .unwrap();
    }
}

/// 格式化数组维度为层次结构的 Koopa IR 类型
fn format_dimensions(sizes: &[usize]) -> String {
    if sizes.is_empty() {
        "i32".to_string() // 如果没有尺寸（标量），直接返回 "i32"
    } else {
        // 递归生成内层类型
        let inner_type = format_dimensions(&sizes[1..]);
        format!("[{}, {}]", inner_type, sizes[0])
    }
}

fn format_func_fparam_type(is_array: &bool, dims: &Option<Vec<ConstExp>>, symbol_table: &SymbolTable,global_symbol_table: &SymbolTable) -> String {
    if let Some(dims) = dims {
        let mut type_str = "i32".to_string();
        for dim in dims.iter().rev() {
            let dim_value = dim.eval(symbol_table,global_symbol_table) as usize;
            type_str = format!("[{}, {}]", type_str, dim_value);
        }
        if *is_array {
            format!("*{}", type_str)
        } else {
            type_str
        }
    } else {
        if *is_array {
            "*i32".to_string()
        } else {
            "i32".to_string()
        }
    }
}


/// 递归生成多维数组的aggregate
fn generate_aggregate(
    output: &mut String,
    sizes: &[usize],
    values: &[i32],
    offset: usize,
) -> usize {
    if sizes.is_empty() {
        return offset; // 终止递归
    }

    output.push('{');
    let dim_size = sizes[0];
    let mut next_offset = offset;

    for i in 0..dim_size {
        if i > 0 {
            output.push_str(", ");
        }

        if sizes.len() == 1 {
            output.push_str(&values[next_offset].to_string());
            next_offset += 1;
        } else {
            next_offset = generate_aggregate(output, &sizes[1..], values, next_offset);
        }
    }

    output.push('}');
    next_offset
}


fn generate_func_def(
    func_def: &FuncDef,
    ir: &mut String,
    global_symbol_table: &mut SymbolTable,
    label_counter: &mut i32,
) {
    let mut local_symbol_table = SymbolTable::new();
    let func_name = &func_def.ident;

    // 注册函数到全局符号表
    let params = if let Some(params) = &func_def.params {
        let mut all_params = vec![params.func_fparam.clone()];
        if let Some(other_params) = &params.params {
            all_params.extend(other_params.clone());
        }
        all_params
    } else {
        vec![]
    };
    global_symbol_table.insert_function(
        func_name.clone(),
        Some(func_def.func_type.clone()),
        params.clone(),
    );

    // 定义函数头部
    let param_str = if let Some(params) = &func_def.params {
        let mut all_params = vec![format!(
            "@p_{}: {}",
            params.func_fparam.ident,
            format_func_fparam_type(&params.func_fparam.is_array, &params.func_fparam.dims, global_symbol_table,global_symbol_table)
        )];
        if let Some(other_params) = &params.params {
            all_params.extend(other_params.iter().map(|param| {
                format!(
                    "@p_{}: {}",
                    param.ident,
                    format_func_fparam_type(&param.is_array, &param.dims, global_symbol_table,global_symbol_table)
                )
            }));
        }
        all_params.join(", ")
    } else {
        String::new()
    };

    match func_def.func_type {
        FuncType::Int => writeln!(ir, "fun @{}({}): i32 {{", func_name, param_str).unwrap(),
        FuncType::Void => writeln!(ir, "fun @{}({}) {{", func_name, param_str).unwrap(),
    }

    // 函数入口块
    writeln!(ir, "%entry:").unwrap();

    // 注册函数参数到局部符号表，并生成相关指令
    if let Some(params) = &func_def.params {
        // 处理第一个参数
        process_func_fparam(
            &params.func_fparam,
            ir,
            &mut local_symbol_table,
            global_symbol_table,
            label_counter,
        );

        // 处理其他参数
        if let Some(other_params) = &params.params {
            for param in other_params {
                process_func_fparam(param, ir, &mut local_symbol_table, global_symbol_table, label_counter);
            }
        }
    }


    let mut loop_labels = Vec::new();
    let mut has_return = false;
    has_return = generate_block(
        &func_def.block,
        ir,
        &mut local_symbol_table,
        label_counter,
        &mut loop_labels,
        func_def.func_type.clone(),
        global_symbol_table,
    );

    if !has_return {
        match func_def.func_type {
            FuncType::Int => writeln!(ir, "  ret 0").unwrap(),
            FuncType::Void => writeln!(ir, "  ret").unwrap(),
        }
    }

    writeln!(ir, "}}").unwrap();
}

fn process_func_fparam(
    param: &FuncFParam,
    ir: &mut String,
    local_symbol_table: &mut SymbolTable,
    global_symbol_table: &SymbolTable,
    label_counter: &mut i32,
) {
    let param_alloc = format!("@{}_{}", param.ident, label_counter);
    
    let param_pointer = format!("@p_{}", param.ident);
    *label_counter += 1;

    // 为参数分配内存
    writeln!(
        ir,
        "  {} = alloc {}",
        param_alloc,
        format_func_fparam_type(&param.is_array, &param.dims, global_symbol_table,global_symbol_table)
    )
    .unwrap();

    if param.is_array {
        // 如果是数组参数，插入到符号表为数组指针类型
        let array_dims = param
            .dims
            .as_ref()
            .map(|dims| dims.iter().map(|dim| dim.eval(global_symbol_table,global_symbol_table) as usize).collect())
            .unwrap_or_else(|| vec![]);
        local_symbol_table.insert_pointer(param.ident.clone(), param.btype.clone(), array_dims, param_alloc.clone());
    } else {
        // 如果是普通变量参数，插入到符号表为变量
        local_symbol_table.insert_variable(param.ident.clone(), param_alloc.clone());
    }
    // 为参数生成存储指令
    writeln!(ir, "  store {}, {}", param_pointer, param_alloc).unwrap();
}


fn generate_block(
    block: &Block, 
    ir: &mut String, 
    symbol_table: &mut SymbolTable,
    label_counter: &mut i32, // 新增：label 计数器
    loop_labels: &mut Vec<(String, String)>, // 新增：循环标签堆栈
    func_type: FuncType,
    global_symbol_table: &mut SymbolTable,
)->bool {
    symbol_table.enter_scope();
    let mut has_return = false;
    for item in &block.items {
        match item {
            BlockItem::Decl(decl) => generate_decl(decl, ir, symbol_table, &mut has_return,label_counter,global_symbol_table),
            BlockItem::Stmt(stmt) => {
                has_return |= generate_stmt(stmt, ir, symbol_table, &mut has_return, label_counter, loop_labels,func_type.clone(),global_symbol_table);
            }
        }
    }
    symbol_table.exit_scope();
    has_return
}

fn generate_decl(decl: &Decl, ir: &mut String, symbol_table: &mut SymbolTable, has_return: &mut bool,label_counter: &mut i32,global_symbol_table: &mut SymbolTable) {
    if *has_return {
        return; // 如果已经生成 return，停止生成
    }
    match decl {
        Decl::ConstDecl(const_decl) => generate_const_decl(const_decl, ir, symbol_table,label_counter,global_symbol_table),
        Decl::VarDecl(var_decl) => generate_var_decl(var_decl, ir, symbol_table,label_counter,global_symbol_table),
    }
}

fn generate_const_decl(const_decl: &ConstDecl, ir: &mut String, symbol_table: &mut SymbolTable,label_counter: &mut i32,global_symbol_table: &mut SymbolTable) {
    // 处理第一个常量定义 (const_decl.def)
    process_const_def(&const_decl.def, ir, symbol_table,label_counter,global_symbol_table);

    // 如果有其他常量定义 (const_decl.defs)，逐个处理
    if let Some(defs) = &const_decl.defs {
        for def in defs {
            process_const_def(def, ir, symbol_table,label_counter,global_symbol_table);
        }
    }
}

fn process_const_def(
    def: &ConstDef, 
    ir: &mut String, 
    symbol_table: &mut SymbolTable,
    label_counter: &mut i32,
    global_symbol_table: &mut SymbolTable,
) {
    let sizes = if let Some(size_exp) = &def.sizes {
        size_exp.iter().map(|s| s.eval(symbol_table,global_symbol_table) as usize).collect()
    } else {
        vec![]
    };

    let total_size = if sizes.is_empty() {
        1
    } else {
        sizes.iter().product::<usize>()
    };

    if sizes.is_empty() {
        // 单个常量
        let mut align_product=vec![];
        align_product.push(1);
        let mut flat_values = vec![]; // 定义一个显式的变量来存储临时值
        let init_values = def.constinitval.eval(&sizes, symbol_table, &align_product, &mut flat_values,global_symbol_table);
        if init_values.len() != 1 {
            panic!(
                "Constant '{}' must have exactly one initialization value",
                def.ident
            );
        }
        let value = init_values[0];
        symbol_table.insert_constant(def.ident.clone(), value);
    } else {
        // 数组常量
        // 多维数组分配
        let alloc_name = format!("@{}_{}", def.ident, label_counter);
        *label_counter += 1;

        let sizes_usize: Vec<usize> = sizes.iter().map(|&s| s as usize).collect();
        writeln!(ir, "  {} = alloc {}", alloc_name, format_dimensions(&sizes_usize)).unwrap();
        symbol_table.insert_array(
            def.ident.clone(),
            BType::Int,
            sizes_usize.clone(),
            alloc_name.clone(),
        );

        // 初始化数组
        let total_size: usize = sizes_usize.iter().product();
        let init_values = if let constinitval = &def.constinitval {
            // 如果有初始化值，生成初始化数据
            let len=sizes.len();
            let mut align_product:Vec<usize>=Vec::new();
            //例如,a[2][3][4],align_product=[4,12,24]
            let mut now_product:usize=1;
            for i in (0..len).rev(){
                now_product*=sizes_usize[i];
                align_product.push(now_product);
            }
            let mut flat_values = vec![];   
            let flat_valuess = constinitval.eval(&sizes_usize, symbol_table,&align_product,&mut flat_values,global_symbol_table);
            let mut flat_valuess = flat_valuess.clone();

            // 如果初始化值不足 total_size，补零
            while flat_valuess.len() < total_size {
                flat_valuess.push(0);
            }
            flat_valuess
        } else {
            // 如果没有初始化值，默认全为 0
            vec![0; total_size]
        };

        let mut values_iter = init_values.into_iter();
        initialize_array(ir, alloc_name, &sizes_usize, &mut values_iter, label_counter);
    }
}




fn generate_var_decl(
    var_decl: &VarDecl,
    ir: &mut String,
    symbol_table: &mut SymbolTable,
    label_counter: &mut i32,
    global_symbol_table: &mut SymbolTable,
) {
    // 处理第一个变量定义 (var_decl.def)
    process_var_def(&var_decl.def, ir, symbol_table, label_counter, global_symbol_table);

    // 如果有其他变量定义 (var_decl.defs)，逐个处理
    if let Some(defs) = &var_decl.defs {
        for def in defs {
            process_var_def(def, ir, symbol_table, label_counter, global_symbol_table);
        }
    }
}

/// 处理单个变量定义的分配和初始化
fn process_var_def(
    var_def: &VarDef,
    ir: &mut String,
    symbol_table: &mut SymbolTable,
    label_counter: &mut i32,
    global_symbol_table: &mut SymbolTable,
) {
    // 显式条件判断解析数组大小
    let sizes = if let Some(size_exp) = &var_def.sizes {
        size_exp.iter().map(|s| s.eval(symbol_table,global_symbol_table)).collect::<Vec<_>>()
    } else {
        vec![]
    };

    if sizes.is_empty() {
        // 单个变量分配
        let alloc_name = format!("@{}_{}", var_def.ident, label_counter);
        *label_counter += 1;
        writeln!(ir, "  {} = alloc i32", alloc_name).unwrap();
        symbol_table.insert_variable(var_def.ident.clone(), alloc_name.clone());

        if let Some(initval) = &var_def.initval {
            // 如果有初始化值
            if let InitVal::Single(exp) = initval {
                let value = generate_exp(exp, ir, symbol_table, label_counter, global_symbol_table);
                writeln!(ir, "  store {}, {}", value, alloc_name).unwrap();
            }
        } else {
            // 如果没有初始化值，默认初始化为 0
            writeln!(ir, "  store 0, {}", alloc_name).unwrap();
        }
    } else {
        // 多维数组分配
        let alloc_name = format!("@{}_{}", var_def.ident, label_counter);
        *label_counter += 1;

        let sizes_usize: Vec<usize> = sizes.iter().map(|&s| s as usize).collect();
        writeln!(ir, "  {} = alloc {}", alloc_name, format_dimensions(&sizes_usize)).unwrap();
        symbol_table.insert_array(
            var_def.ident.clone(),
            BType::Int,
            sizes_usize.clone(),
            alloc_name.clone(),
        );

        // 初始化数组
        let total_size: usize = sizes_usize.iter().product();
        let init_values = if let Some(initval) = &var_def.initval {
            // 如果有初始化值，生成初始化数据
            let len=sizes.len();
            let mut align_product:Vec<usize>=Vec::new();
            //例如,a[2][3][4],align_product=[4,12,24]
            let mut now_product:usize=1;
            for i in (0..len).rev(){
                now_product*=sizes_usize[i];
                align_product.push(now_product);
            }
            let mut flat_values = vec![];   
            let flat_valuess = initval.eval(&sizes_usize, symbol_table,&align_product,&mut flat_values,global_symbol_table);
            let mut flat_valuess = flat_valuess.clone();

            // 如果初始化值不足 total_size，补零
            while flat_valuess.len() < total_size {
                flat_valuess.push(0);
            }
            flat_valuess
        } else {
            // 如果没有初始化值，默认全为 0
            vec![0; total_size]
        };

        let mut values_iter = init_values.into_iter();
        initialize_array(ir, alloc_name, &sizes_usize, &mut values_iter, label_counter);
    }
}

// 局部变量初始化：通过递归调用 getelemptr
fn initialize_array(
    ir: &mut String,
    ptr: String,
    sizes: &[usize],
    values_iter: &mut impl Iterator<Item = i32>,
    label_counter: &mut i32,
) {
    //let len=sizes.len();
    if sizes.is_empty() {
        let value = values_iter.next().unwrap_or(0);
        writeln!(ir, "  store {}, {}", value, ptr).unwrap();
    } else {
        for i in 0..sizes[0] {
            let new_ptr = format!("%ptr_{}", label_counter);
            *label_counter += 1;
            writeln!(ir, "  {} = getelemptr {}, {}", new_ptr, ptr, i).unwrap();
            initialize_array(ir, new_ptr, &sizes[1..], values_iter, label_counter);
        }
    }
}

fn generate_stmt(
    stmt: &Stmt,
    ir: &mut String,
    symbol_table: &mut SymbolTable,
    has_return: &mut bool,
    label_counter: &mut i32, // 添加 label 计数器
    loop_labels: &mut Vec<(String, String)>, // 新增：循环的开始和结束标签堆栈
    func_type: FuncType,
    global_symbol_table: &mut SymbolTable,
)->bool {
    let mut local_has_return = false;
    match stmt {
        Stmt::Assign { lval, exp } => {
            if *has_return {
                return true; // 如果已经生成 return，跳过后续生成
            }
        
            // 提前获取变量分配地址
            let alloc = symbol_table
                .get(&lval.ident)
                .or_else(|| global_symbol_table.get(&lval.ident));
        
            if let Some(symbol) = alloc {
                let value = generate_exp(exp, ir, symbol_table, label_counter, global_symbol_table);
        
                match symbol {
                    Symbol::Variable(alloc_name) => {
                        // 普通变量赋值
                        writeln!(ir, "  store {}, {}", value, alloc_name).unwrap();
                    }
                    Symbol::Array { alloc, .. } => {
                        // 数组元素赋值
                        if let Some(indices) = &lval.indices {
                            //let len = indices.len();
                            let mut ptr = alloc.clone();
                            for (index,index_exp) in indices.iter().enumerate() {
                                let index_value = generate_exp(index_exp, ir, symbol_table, label_counter, global_symbol_table);
        
                                let new_ptr = format!("%ptr_{}", label_counter);
                                *label_counter += 1;
                                writeln!(ir, "  {} = getelemptr {}, {}", new_ptr, ptr, index_value).unwrap();
                                
                                ptr = new_ptr;
                            }
                            // 最终将值存储到目标地址
                            writeln!(ir, "  store {}, {}", value, ptr).unwrap();
                        } else {
                            panic!("Array '{}' requires indices for assignment", lval.ident);
                        }
                    }
                    // 数组指针, 需要先对第一个[]解引用，也就是load
                    Symbol::Pointer {alloc, .. } => {
                        let ptr = alloc.clone();
                        let new_ptr = format!("%ptr_{}", label_counter);
                        *label_counter += 1;
                        writeln!(ir, "  {} = load {}", new_ptr, ptr).unwrap();
                        if let Some(indices) = &lval.indices {
                            let len = indices.len();
                            let mut ptr = new_ptr;
                            for (index,index_exp) in indices.iter().enumerate() {
                                let index_value = generate_exp(index_exp, ir, symbol_table, label_counter, global_symbol_table);
        
                                let new_ptr = format!("%ptr_{}", label_counter);
                                *label_counter += 1;
                                if index==0 {
                                    writeln!(ir, "  {} = getptr {}, {}", new_ptr, ptr, index_value).unwrap();
                                } else {
                                    writeln!(ir, "  {} = getelemptr {}, {}", new_ptr, ptr, index_value).unwrap();
                                }
                                ptr = new_ptr;
                            }
                            // 最终将值存储到目标地址
                            writeln!(ir, "  store {}, {}", value, ptr).unwrap();
                        } else {
                            panic!("Array '{}' requires indices for assignment", lval.ident);
                        }
                    }
                    _ => {
                        panic!("Cannot assign to constant or unsupported type: {}", lval.ident);
                    }
                }
            } else {
                // 未定义的变量
                panic!("Cannot assign to undefined variable: {}", lval.ident);
            }
        }
        Stmt::Return { exp } => {
            if *has_return {
                return true; // 如果已经生成 return，跳过后续生成
            }
            if let Some(exp) = exp {
                let value = generate_exp(exp, ir, symbol_table,label_counter,global_symbol_table);
                writeln!(ir, "  ret {}", value).unwrap();
            } else {
                // 根据函数返回类型处理
                match func_type {
                    FuncType::Int => writeln!(ir, "  ret 0").unwrap(),
                    FuncType::Void => writeln!(ir, "  ret").unwrap(),
                }
            }
            local_has_return = true;
        }
        Stmt::IfElse { cond, then_branch, else_branch } => {
            let cond_value = generate_exp(cond, ir, symbol_table,label_counter,global_symbol_table);
            let then_label = new_label(label_counter, "then");
            let else_label = new_label(label_counter, "else");
            let end_label = new_label(label_counter, "end");

            // 条件分支
            writeln!(ir, "  br {}, {}, {}", cond_value, then_label, else_label).unwrap();

            // 处理 then 分支
            writeln!(ir, "{}:", then_label).unwrap();
            let mut temp_has_return = false;
            let then_has_return = generate_stmt(then_branch, ir, symbol_table, &mut temp_has_return, label_counter, loop_labels,func_type.clone(),global_symbol_table);
            if !then_has_return {
                writeln!(ir, "  jump {}", end_label).unwrap();
            }

            // 处理 else 分支
            writeln!(ir, "{}:", else_label).unwrap();
            let mut temp_has_return_1 = false;
            let else_has_return = if let Some(else_branch) = else_branch {
                generate_stmt(else_branch, ir, symbol_table, &mut temp_has_return_1, label_counter, loop_labels,func_type.clone(),global_symbol_table)
            } else {
                false
            };
            if !else_has_return {
                writeln!(ir, "  jump {}", end_label).unwrap();
            }

            // 结束块
            writeln!(ir, "{}:", end_label).unwrap();
        }
        Stmt::While { cond, body } => {
            let while_entry_label = new_label(label_counter, "while_entry");
            let while_body_label = new_label(label_counter, "while_body");
            let while_end_label = new_label(label_counter, "end");

            // 将当前循环的开始和结束标签压入堆栈
            loop_labels.push((while_entry_label.clone(), while_end_label.clone()));
            // while 条件检查块
            writeln!(ir, "  jump {}", while_entry_label).unwrap();
            writeln!(ir, "{}:", while_entry_label).unwrap();
            let cond_value = generate_exp(cond, ir, symbol_table,label_counter,global_symbol_table);
            writeln!(ir, "  br {}, {}, {}", cond_value, while_body_label, while_end_label).unwrap();

            // while 循环体块
            writeln!(ir, "{}:", while_body_label).unwrap();
            let mut temp_has_return = false;
            let body_has_return_or_break_or_continue = generate_stmt(body, ir, symbol_table, &mut temp_has_return, label_counter, loop_labels,func_type.clone(),global_symbol_table);
            if !body_has_return_or_break_or_continue {
                writeln!(ir, "  jump {}", while_entry_label).unwrap();
            }

            // while 结束块
            writeln!(ir, "{}:", while_end_label).unwrap();

            // 弹出当前循环的标签
            loop_labels.pop();
        }
        Stmt::Break => {
            if *has_return {
                return true; // 如果已经生成 return，跳过后续生成
            }
            if let Some((_, end_label)) = loop_labels.last() {
                writeln!(ir, "  jump {}", end_label).unwrap();
            } else {
                panic!("`break` used outside of a loop");
            }
            local_has_return = true;
        }
        Stmt::Continue => {
            if *has_return {
                return true; // 如果已经生成 return，跳过后续生成
            }
            if let Some((start_label, _)) = loop_labels.last() {
                writeln!(ir, "  jump {}", start_label).unwrap();
            } else {
                panic!("`continue` used outside of a loop");
            }
            local_has_return = true;
        }
        Stmt::ExpStmt { exp } => {
            if *has_return {
                return true; // 如果已经生成 return，跳过后续生成
            }
            if let Some(exp) = exp {
                generate_exp(exp, ir, symbol_table,label_counter,global_symbol_table);
            }
        }
        Stmt::Block(block) => {
            local_has_return = generate_block(block, ir, symbol_table, label_counter, loop_labels,func_type.clone(),global_symbol_table);
        }
        Stmt::Matched(inner) => {
            local_has_return = generate_stmt(inner, ir, symbol_table, has_return, label_counter, loop_labels,func_type.clone(),global_symbol_table);
        }
        Stmt::Open(inner) => {
            local_has_return = generate_stmt(inner, ir, symbol_table, has_return, label_counter, loop_labels,func_type.clone(),global_symbol_table);
        }
    }
    *has_return |= local_has_return;
    local_has_return
}

// 生成表达式的 Koopa IR
fn generate_exp(exp: &Exp, ir: &mut String, symbol_table: &SymbolTable,label_counter: &mut i32,global_symbol_table: & SymbolTable) -> String {
    generate_lor_exp(&exp.lor, ir, symbol_table,label_counter,global_symbol_table)
}

fn generate_const_exp(exp: &ConstExp, ir: &mut String, symbol_table: &SymbolTable,label_counter: &mut i32,global_symbol_table: &mut SymbolTable) -> String {
    generate_exp(&exp.exp, ir, symbol_table,label_counter,global_symbol_table)
}

fn generate_lor_exp(exp: &LOrExp, ir: &mut String, symbol_table: &SymbolTable,label_counter: &mut i32,global_symbol_table: & SymbolTable) -> String {
    match exp {
        LOrExp::LAnd(exp) => generate_land_exp(exp, ir, symbol_table,label_counter,global_symbol_table),
        LOrExp::LOrLAnd(lhs, rhs) => {
            let lhs_val = generate_lor_exp(lhs, ir, symbol_table,label_counter,global_symbol_table);

            // 分配栈空间存储结果
            let result_alloc = new_alloc_temp();
            writeln!(ir, "  {} = alloc i32", result_alloc).unwrap();

            // 新建短路的基本块
            let true_label = new_label(label_counter, "true");
            let false_label = new_label(label_counter, "false");
            let end_label = new_label(label_counter, "end");

            // 如果 lhs 为真，跳转到 true_label
            writeln!(ir, "  br {}, {}, {}", lhs_val, true_label, false_label).unwrap();

            // true_label：结果为真
            writeln!(ir, "{}:", true_label).unwrap();
            writeln!(ir, "  store 1, {}", result_alloc).unwrap();
            writeln!(ir, "  jump {}", end_label).unwrap();

            // false_label：继续计算 rhs
            writeln!(ir, "{}:", false_label).unwrap();
            let rhs_val = generate_land_exp(rhs, ir, symbol_table,label_counter,global_symbol_table);
            let rhs_result = new_temp();
            writeln!(ir, "  {} = ne {}, 0", rhs_result, rhs_val).unwrap();
            writeln!(ir, "  store {}, {}", rhs_result, result_alloc).unwrap();
            writeln!(ir, "  jump {}", end_label).unwrap();

            // end_label：读取结果
            writeln!(ir, "{}:", end_label).unwrap();
            let final_result = new_temp();
            writeln!(ir, "  {} = load {}", final_result, result_alloc).unwrap();

            final_result
        }
    }
}

fn generate_land_exp(exp: &LAndExp, ir: &mut String, symbol_table: &SymbolTable,label_counter: &mut i32,global_symbol_table: & SymbolTable) -> String {
    match exp {
        LAndExp::Eq(exp) => generate_eq_exp(exp, ir, symbol_table,label_counter,global_symbol_table),
        LAndExp::LAndEq(lhs, rhs) => {
            let lhs_val = generate_land_exp(lhs, ir, symbol_table,label_counter,global_symbol_table);

            // 分配栈空间存储结果
            let result_alloc = new_alloc_temp();
            writeln!(ir, "  {} = alloc i32", result_alloc).unwrap();

            // 新建短路的基本块
            let true_label = new_label(label_counter, "true");
            let false_label = new_label(label_counter, "false");
            let end_label = new_label(label_counter, "end");

            // 如果 lhs 为假，跳转到 false_label
            writeln!(ir, "  br {}, {}, {}", lhs_val, true_label, false_label).unwrap();

            // true_label：继续计算 rhs
            writeln!(ir, "{}:", true_label).unwrap();
            let rhs_val = generate_eq_exp(rhs, ir, symbol_table,label_counter,global_symbol_table);
            let rhs_result = new_temp();
            writeln!(ir, "  {} = ne {}, 0", rhs_result, rhs_val).unwrap();
            writeln!(ir, "  store {}, {}", rhs_result, result_alloc).unwrap();
            writeln!(ir, "  jump {}", end_label).unwrap();

            // false_label：结果为假
            writeln!(ir, "{}:", false_label).unwrap();
            writeln!(ir, "  store 0, {}", result_alloc).unwrap();
            writeln!(ir, "  jump {}", end_label).unwrap();

            // end_label：读取结果
            writeln!(ir, "{}:", end_label).unwrap();
            let final_result = new_temp();
            writeln!(ir, "  {} = load {}", final_result, result_alloc).unwrap();

            final_result
        }
    }
}


fn generate_eq_exp(exp: &EqExp, ir: &mut String, symbol_table: &SymbolTable,label_counter: &mut i32,global_symbol_table: & SymbolTable) -> String {
    match exp {
        EqExp::Rel(exp) => generate_rel_exp(exp, ir, symbol_table,label_counter,global_symbol_table),
        EqExp::EqRel(lhs, op, rhs) => {
            let lhs_val = generate_eq_exp(lhs, ir, symbol_table,label_counter,global_symbol_table);
            let rhs_val = generate_rel_exp(rhs, ir, symbol_table,label_counter,global_symbol_table);
            let result = new_temp();
            match op {
                EqOp::Eq => writeln!(ir, "  {} = eq {}, {}", result, lhs_val, rhs_val).unwrap(),
                EqOp::Neq => writeln!(ir, "  {} = ne {}, {}", result, lhs_val, rhs_val).unwrap(),
            }
            result
        }
    }
}

fn generate_rel_exp(exp: &RelExp, ir: &mut String, symbol_table: &SymbolTable,label_counter: &mut i32,global_symbol_table: & SymbolTable) -> String {
    match exp {
        RelExp::Add(exp) => generate_add_exp(exp, ir, symbol_table,label_counter,global_symbol_table),
        RelExp::RelAdd(lhs, op, rhs) => {
            let lhs_val = generate_rel_exp(lhs, ir, symbol_table,label_counter,global_symbol_table);
            let rhs_val = generate_add_exp(rhs, ir, symbol_table,label_counter,global_symbol_table);
            let result = new_temp();
            match op {
                RelOp::Lt => writeln!(ir, "  {} = lt {}, {}", result, lhs_val, rhs_val).unwrap(),
                RelOp::Gt => writeln!(ir, "  {} = gt {}, {}", result, lhs_val, rhs_val).unwrap(),
                RelOp::Le => writeln!(ir, "  {} = le {}, {}", result, lhs_val, rhs_val).unwrap(),
                RelOp::Ge => writeln!(ir, "  {} = ge {}, {}", result, lhs_val, rhs_val).unwrap(),
            }
            result
        }
    }
}

fn generate_add_exp(exp: &AddExp, ir: &mut String, symbol_table: &SymbolTable,label_counter: &mut i32,global_symbol_table: & SymbolTable) -> String {
    match exp {
        AddExp::Mul(exp) => generate_mul_exp(exp, ir, symbol_table,label_counter,global_symbol_table),
        AddExp::AddMul(lhs, op, rhs) => {
            let lhs_val = generate_add_exp(lhs, ir, symbol_table,label_counter,global_symbol_table);
            let rhs_val = generate_mul_exp(rhs, ir, symbol_table,label_counter,global_symbol_table);
            let result = new_temp();
            match op {
                AddOp::Add => writeln!(ir, "  {} = add {}, {}", result, lhs_val, rhs_val).unwrap(),
                AddOp::Sub => writeln!(ir, "  {} = sub {}, {}", result, lhs_val, rhs_val).unwrap(),
            }
            result
        }
    }
}

fn generate_mul_exp(exp: &MulExp, ir: &mut String, symbol_table: &SymbolTable,label_counter: &mut i32,global_symbol_table: & SymbolTable) -> String {
    match exp {
        MulExp::Unary(exp) => generate_unary_exp(exp, ir, symbol_table,label_counter,global_symbol_table),
        MulExp::MulUnary(lhs, op, rhs) => {
            let lhs_val = generate_mul_exp(lhs, ir, symbol_table,label_counter,global_symbol_table);
            let rhs_val = generate_unary_exp(rhs, ir, symbol_table,label_counter,global_symbol_table);
            let result = new_temp();
            match op {
                MulOp::Mul => writeln!(ir, "  {} = mul {}, {}", result, lhs_val, rhs_val).unwrap(),
                MulOp::Div => writeln!(ir, "  {} = div {}, {}", result, lhs_val, rhs_val).unwrap(),
                MulOp::Mod => writeln!(ir, "  {} = mod {}, {}", result, lhs_val, rhs_val).unwrap(),
            }
            result
        }
    }
}

fn generate_unary_exp(exp: &UnaryExp, ir: &mut String, symbol_table: &SymbolTable,label_counter: &mut i32,global_symbol_table: & SymbolTable) -> String {
    match exp {
        UnaryExp::Primary(exp) => generate_primary_exp(exp, ir, symbol_table,label_counter,global_symbol_table),
        UnaryExp::Unary(op, exp) => {
            let operand = generate_unary_exp(exp, ir, symbol_table,label_counter,global_symbol_table);
            let result = new_temp();
            match op {
                UnaryOp::Neg => writeln!(ir, "  {} = sub 0, {}", result, operand).unwrap(),
                UnaryOp::LNot => writeln!(ir, "  {} = eq {}, 0", result, operand).unwrap(),
            }
            result
        }
        
        UnaryExp::Call { ident, args } => {
            let arg_strs: Vec<String> = args
                .iter()
                .map(|arg| {
                    let value = generate_exp(arg, ir, symbol_table, label_counter, global_symbol_table);
                    value
                })
                .collect();
        
            let args_ir = arg_strs.join(", ");
        
            let function_symbol = global_symbol_table.get(ident);
        
            if let Some(Symbol::Function { return_type: Some(FuncType::Int), .. }) = function_symbol {
                let result = new_temp();
                writeln!(ir, "  {} = call @{}({})", result, ident, args_ir).unwrap();
                result
            } else if let Some(Symbol::Function { return_type: Some(FuncType::Void), .. }) = function_symbol {
                writeln!(ir, "  call @{}({})", ident, args_ir).unwrap();
                String::new()
            } else {
                panic!("Function '{}' is not defined", ident);
            }
        }        
    }
}

fn generate_primary_exp(
    exp: &PrimaryExp,
    ir: &mut String,
    symbol_table: &SymbolTable,
    label_counter: &mut i32,
    global_symbol_table: &SymbolTable,
) -> String {
    match exp {
        PrimaryExp::Exp(exp) => {
            generate_exp(exp, ir, symbol_table, label_counter, global_symbol_table)
        }
        PrimaryExp::Lval(lval) => {
            let symbol = symbol_table
                .get(&lval.ident)
                .or_else(|| global_symbol_table.get(&lval.ident))
                .expect(&format!("Undefined symbol: {}", lval.ident));

            match symbol {
                Symbol::Variable(alloc) => {
                    let result = new_temp();
                    writeln!(ir, "  {} = load {}", result, alloc).unwrap();
                    result
                }
                Symbol::Constant(value) => value.to_string(),
                Symbol::Array { alloc, size, .. } => {

                    let mut ptr;
                    let mut new_ptr;
                    let size_len = size.len();
                    if let Some(indices) = &lval.indices {
                        ptr = alloc.clone();
                        let len = indices.len();
                        for (_, index_exp) in indices.iter().enumerate() {
                            let index_value = generate_exp(
                                index_exp,
                                ir,
                                symbol_table,
                                label_counter,
                                global_symbol_table,
                            );
                            new_ptr = format!("%ptr_{}", label_counter);
                            *label_counter += 1;

                            writeln!(ir, "  {} = getelemptr {}, {}", new_ptr, ptr, index_value).unwrap();
                            
                            ptr = new_ptr;
                        }
                        // 如果索引数刚好等于size_len，说明所有的[]都在了，这是一个数组元素，需要load
                        if len==size_len {
                            let result = new_temp();
                            writeln!(ir, "  {} = load {}", result, ptr).unwrap();
                            result
                        } 
                        // 如果索引数不等于size_len，说明这是一个数组指针（原数组的一部分），需要使用getelemptr且偏移量为0来降维
                        else {
                            new_ptr = format!("%ptr_{}", label_counter);
                            *label_counter += 1;
                            writeln!(ir, "  {} = getelemptr {}, 0", new_ptr, ptr).unwrap();
                            new_ptr
                        }
                    } else {
                        ptr=alloc.clone();
                        let new_ptr = format!("%ptr_{}", label_counter);
                        *label_counter += 1;
                        writeln!(ir, "  {} = getelemptr {}, 0", new_ptr, ptr).unwrap();
                        new_ptr
                    }
                }
                Symbol::Pointer { alloc, size, .. } => {
                    
                    let mut ptr = alloc.clone();
                    let mut new_ptr = format!("%ptr_{}", label_counter);
                    *label_counter += 1;
                    writeln!(ir, "  {} = load {}", new_ptr, ptr).unwrap();
                    let mut size_len = size.len();
                    if size.is_empty(){
                        size_len=0;
                    }
                    if let Some(indices) = &lval.indices {
                        let len = indices.len();
                        ptr = new_ptr;
                        for (dim_index, index_exp) in indices.iter().enumerate() {
                            let index_value = generate_exp(
                                index_exp,
                                ir,
                                symbol_table,
                                label_counter,
                                global_symbol_table,
                            );
                            new_ptr = format!("%ptr_{}", label_counter);
                            *label_counter += 1;

                            if dim_index==0{
                                // 第一维使用 getptr
                                writeln!(ir, "  {} = getptr {}, {}", new_ptr, ptr, index_value).unwrap();
                            } else {
                                // 其他情况使用 getelemptr
                                writeln!(ir, "  {} = getelemptr {}, {}", new_ptr, ptr, index_value).unwrap();
                            }
                            ptr = new_ptr;
                        }
                        // 如果索引数刚好比size_len多1，说明所有的[]都在了，这是一个数组元素，需要load
                        if len-size_len==1 {
                            let result = new_temp();
                            writeln!(ir, "  {} = load {}", result, ptr).unwrap();
                            result
                        } 
                        // 如果索引数不比size_len多，说明这是一个数组指针（原数组的一部分），需要使用getelemptr且偏移量为0来降维
                        else {
                            let new_ptr = format!("%ptr_{}", label_counter);
                            *label_counter += 1;
                            writeln!(ir, "  {} = getelemptr {}, {}", new_ptr, ptr, 0).unwrap();
                            new_ptr
                        }
                    } else {
                        new_ptr
                    }
                }
                _ => panic!("Unsupported Lval type"),
            }
        }
        PrimaryExp::Number(value) => value.to_string(),
    }
}


// 临时变量生成器
fn new_temp() -> String {
    static mut TEMP_COUNT: i32 = 0;
    unsafe {
        let temp = format!("%{}", TEMP_COUNT);
        TEMP_COUNT += 1;
        temp
    }
}

fn new_temp_ptr() -> String {
    static mut TEMP_PTR_COUNTER: i32 = 0; // 全局计数器
    unsafe {
        let name = format!("%ptr_{}", TEMP_PTR_COUNTER);
        TEMP_PTR_COUNTER += 1;
        name
    }
}

// 栈上变量生成器，生成以 @ 开头的唯一变量名
fn new_alloc_temp() -> String {
    static mut ALLOC_COUNT: i32 = 0;
    unsafe {
        let alloc = format!("@alloc_{}", ALLOC_COUNT);
        ALLOC_COUNT += 1;
        alloc
    }
}

fn new_label(counter: &mut i32, prefix: &str) -> String {
    let label = format!("%{}_{}", prefix, *counter);
    *counter += 1;
    label
}

fn postprocess_ir(ir: &mut String, global_symbol_table: &SymbolTable) {
    let mut processed_ir = String::new();
    let mut lines = ir.lines().peekable();

    let mut current_func_type = None;

    while let Some(line) = lines.next() {
        processed_ir.push_str(line);
        processed_ir.push('\n');

        // 检查是否是函数定义
        if line.starts_with("fun @") {
            if let Some(func_name_end) = line.find('(') {
                let func_name = &line[5..func_name_end];
                current_func_type = global_symbol_table
                    .get(func_name)
                    .and_then(|symbol| match symbol {
                        Symbol::Function { return_type, .. } => return_type.as_ref(),
                        _ => None,
                    });
            }
        }

        // 检查是否是基本块定义
        if line.trim_end().ends_with(':') {
            // 检查下一个非空行
            let next_line = lines.peek().map(|l| l.trim()).unwrap_or("");

            // 如果下一个行是空的或另一个块定义，插入默认指令
            if next_line.is_empty() || next_line.ends_with(':') || next_line.ends_with('}') && next_line.starts_with('}') {
                if let Some(func_type) = current_func_type {
                    match func_type {
                        FuncType::Int => processed_ir.push_str("  ret 0\n"),
                        FuncType::Void => processed_ir.push_str("  ret\n"),
                    }
                }
            }
        }
    }

    *ir = processed_ir;
}