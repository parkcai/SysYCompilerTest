use colored::Colorize;
use lalrpop_util::lexer::Token;
use lalrpop_util::ParseError;
use std::process::exit;

pub fn show_error(error: &str, exit_code: i32) -> ! {
    eprintln!("{} {}", "Error:".red().bold(), error.bold());
    exit(exit_code)
}

pub fn show_error_no_exit(error: &str) {
    eprintln!("{} {}", "Error:".red().bold(), error.bold());
}

pub fn show_parse_error(error: ParseError<usize, Token, &str>, input: &str, file_path: &str) -> ! {
    match error {
        ParseError::InvalidToken { location } => {
            show_error_position(input, location, "Invalid token", file_path)
        }
        ParseError::UnrecognizedEof {
            location,
            expected: _,
        } => {
            show_error_position(input, location, "Unexpected end of file", file_path);
        }
        ParseError::UnrecognizedToken {
            token: (start, _, end),
            expected: _,
        } => {
            show_error_range(input, start, end, "Unrecognized token", file_path);
        }
        ParseError::ExtraToken {
            token: (start, _, end),
        } => show_error_range(input, start, end, "Extra token", file_path),
        ParseError::User { error: message } => {
            show_error(&format!("Error: {}", message), 1);
        }
    }
    exit(1)
}

fn get_position(input: &str, location: usize) -> (usize, usize) {
    let mut line = 1;
    let mut column = 1;
    for (i, c) in input.chars().enumerate() {
        if i == location {
            break;
        }
        if c == '\n' {
            line += 1;
            column = 1;
        } else {
            column += 1;
        }
    }
    (line, column)
}

fn show_error_line(
    error_line: &str,
    line_no: usize,
    start_col: usize,
    end_col: usize,
    header_space: usize,
) {
    let line_no = line_no.to_string();
    let space_no = if header_space > line_no.len() {
        header_space - line_no.len()
    } else {
        0
    };
    eprintln!(
        "{}{} {} {}",
        line_no.bright_cyan().bold(),
        " ".repeat(space_no),
        "|".bright_cyan().bold(),
        error_line
    );
    eprintln!(
        "{} {} {}{}",
        " ".repeat(header_space),
        "|".bright_cyan().bold(),
        " ".repeat(start_col - 1),
        "^".repeat(end_col - start_col).red().bold()
    );
}

fn show_file_info(file_path: &str, line_no: usize, column: usize) {
    eprintln!(
        " {} {}:{}:{}",
        "-->".bright_cyan().bold(),
        file_path,
        line_no,
        column
    );
}

pub fn show_error_position(input: &str, location: usize, message: &str, file_path: &str) {
    let (line_no, column) = get_position(input, location);
    let lines: Vec<&str> = input.split('\n').collect();
    let line = lines[line_no - 1];
    show_error_no_exit(message);
    let line_no_width = line_no.to_string().len();
    show_file_info(file_path, line_no, column);
    eprintln!("{} {}", " ".repeat(line_no_width), "|".bright_cyan().bold());
    show_error_line(line, line_no, column, column + 1, line_no_width);
}

pub fn show_error_range(input: &str, start: usize, end: usize, message: &str, file_path: &str) {
    let (start_line, start_column) = get_position(input, start);
    let (end_line, end_column) = get_position(input, end);
    let lines: Vec<&str> = input.split('\n').collect();
    show_error_no_exit(message);
    show_file_info(file_path, start_line, start_column);
    if start_line == end_line {
        let line = lines[start_line - 1];
        let line_no = start_line.to_string();
        let line_no_width = line_no.len();
        eprintln!("{} {}", " ".repeat(line_no_width), "|".bright_cyan().bold());
        show_error_line(line, start_line, start_column, end_column, line_no_width);
    } else {
        let error_lines = &lines[start_line - 1..end_line];
        let max_line_no_width = (start_line..=end_line)
            .map(|n| n.to_string().len())
            .max()
            .unwrap();
        eprintln!(
            "{} {}",
            " ".repeat(max_line_no_width),
            "|".bright_cyan().bold(),
        );
        for (i, line) in error_lines.iter().enumerate() {
            let start_column = if i == 0 { start_column } else { 1 };
            let end_column = if i == error_lines.len() - 1 {
                end_column
            } else {
                line.len()
            };
            show_error_line(
                line,
                start_line + i,
                start_column,
                end_column,
                max_line_no_width,
            );
        }
    }
}
