use std::{
    collections::HashMap,
    io::{Error, ErrorKind},
};

use regex::Regex;

// 定义宏的结构体，包含名称、参数和替换体
struct MacroDefinition {
    // name: String,
    params: Vec<String>, // 空向量表示无参数宏
    body: String,
}

pub fn macro_processor(input: String, output: &mut String) -> Result<(), Box<dyn std::error::Error>> {
    let mut macro_definitions: HashMap<String, MacroDefinition> = HashMap::new();
    let mut processed_lines = Vec::new();

    // 正则表达式用于匹配宏定义
    let re = Regex::new(r"(?m)^\s*#\s*define\s+(\w+)(\(([^)]*)\))?\s*(.*)?$")
        .map_err(|e| Error::new(ErrorKind::Other, e))?;

    for line in input.lines() {
        if let Some(cap) = re.captures(line) {
            let key = cap.get(1).unwrap().as_str();
            let args = cap.get(3).map_or("", |m| m.as_str());
            let body = cap.get(4).map_or("", |m| m.as_str());

            let macro_def = if args.is_empty() {
                // 无参数宏
                MacroDefinition {
                    // name: key.to_string(),
                    params: Vec::new(),
                    body: body.to_string(),
                }
            } else {
                // 带参数宏
                let args_vec: Vec<String> = args.split(',')
                    .map(|s| s.trim().to_string())
                    .collect();

                MacroDefinition {
                    // name: key.to_string(),
                    params: args_vec,
                    body: body.to_string(),
                }
            };

            macro_definitions.insert(key.to_string(), macro_def);
        } else {
            processed_lines.push(line);
        }
    }

    // 替换宏定义
    let processed_code = replace_macros(&processed_lines.join("\n"), &macro_definitions);
    *output = processed_code;
    Ok(())
}

fn replace_macros(code: &str, macros: &HashMap<String, MacroDefinition>) -> String {
    let mut result = code.to_string();

    // 先处理无参数的宏定义
    for (key, macro_def) in macros.iter().filter(|(_, m)| m.params.is_empty()) {
        let re = Regex::new(&format!(r"\b{}\b", regex::escape(key))).unwrap();
        result = re.replace_all(&result, macro_def.body.as_str()).to_string();
    }

    // 处理带参数的宏定义
    for (key, macro_def) in macros.iter().filter(|(_, m)| !m.params.is_empty()) {
        // 构建匹配宏调用的正则表达式，例如: MACRO(arg1, arg2)
        let re_pattern = format!(
            r"\b{}\s*\(\s*([^)]*?)\s*\)",
            regex::escape(key)
        );
        let re = Regex::new(&re_pattern).unwrap();

        result = re.replace_all(&result, |caps: &regex::Captures| {
            let args_str = caps.get(1).unwrap().as_str();
            let args: Vec<&str> = args_str.split(',').map(|s| s.trim()).collect();

            if args.len() != macro_def.params.len() {
                // 参数数量不匹配，保留原始文本
                return caps.get(0).unwrap().as_str().to_string();
            }

            // 创建一个临时字符串用于替换参数
            let mut replaced_body = macro_def.body.clone();

            for (i, arg) in args.iter().enumerate() {
                // 使用正则表达式替换参数名为实际传递的参数值
                // 使用 word boundary \b 确保只替换完整的参数名
                let param_re = Regex::new(&format!(r"\b{}\b", regex::escape(&macro_def.params[i]))).unwrap();
                replaced_body = param_re.replace_all(&replaced_body, *arg).to_string();
            }

            replaced_body
        }).to_string();
    }

    result
}
