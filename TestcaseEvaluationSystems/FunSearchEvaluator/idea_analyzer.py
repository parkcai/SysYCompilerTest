import os
from pathlib import Path
import re
import subprocess
import tempfile
from lark import Lark, Transformer, Tree, Token
import uuid
import shutil
import sys
from enum import Enum, auto, unique
from typing import List, Optional, Tuple
import shlex
from dataclasses import dataclass


'''
[Acknowledgement]   Zeyu Cai   25/04/22
    This SysY header of our Software Engineering homework relies heavily on 
    MaxXing's sysy-runtime-lib for PKU Compiler Lab 
    (https://github.com/pku-minic/sysy-runtime-lib).
    A lot of thanks to dear MaxXing and other fellow seniors!
'''
SysY_header = r"""
#ifndef LIBSYSY_SYSY_H_
#define LIBSYSY_SYSY_H_

// SysY runtime library.
// Reference: https://bit.ly/3tzTFks
// Modified by MaxXing.

// Input & output functions
int getint(), getch(), getarray(int a[]);
void putint(int num), putch(int ch), putarray(int n, int a[]);

// Timing functions
void starttime();
void stoptime();

#endif  // LIBSYSY_SYSY_H_

#ifdef NO_LIBC
#include "nolibc/io.h"
#include "nolibc/time.h"
#else
#include <sys/time.h>
#include <unistd.h>
#endif

// ============================================================
// Internal implementations of IO operations.
// ============================================================

#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')
#define IS_SPACE(c)                                           \
  ((c) == ' ' || (c) == '\f' || (c) == '\n' || (c) == '\r' || \
   (c) == '\t' || (c) == '\v')

static int last_char;
static int last_char_valid = 0;

#define OUTPUT_BUFFER_SIZE 1024
static char output_buffer[OUTPUT_BUFFER_SIZE];
static size_t output_buffer_index = 0;

static void flush_buffer(int fd) {
  if (output_buffer_index > 0) {
    write(fd, output_buffer, output_buffer_index);
    output_buffer_index = 0;
  }
}

static void write_buffer(int fd, const void *buffer, size_t size) {
  if (fd == STDERR_FILENO) {
    write(fd, buffer, size);
  }
  else {
    if (output_buffer_index + size > OUTPUT_BUFFER_SIZE) flush_buffer(fd);
    for (size_t i = 0; i < size; i++) {
      output_buffer[output_buffer_index++] = ((char *)buffer)[i];
    }
  }
}

static void PutChar(int fd, char c) {
  write_buffer(fd, &c, 1);
  if (c == '\n') flush_buffer(fd);
}

static void PutString(int fd, const char *str) {
  for (int i = 0; str[i]; ++i) PutChar(fd, str[i]);
}

static void PutInt(int fd, int num) {
  // check if is a negative integer
  if (num < 0) {
    putch('-');
    num = -num;
  }
  // convert integer to string
  char digits[21];
  int i = 20;
  if (!num) {
    i = 19;
    digits[19] = '0';
  }
  else {
    while (num) {
      i -= 1;
      digits[i] = (num % 10) + '0';
      num /= 10;
    }
  }
  // write string to stdout
  write_buffer(fd, digits + i, 20 - i);
}

// ============================================================
// Implementations of IO functions.
// ============================================================

int getint() {
  // skip spaces
  int c = getch();
  while (IS_SPACE(c)) c = getch();
  // check if is a negative integer
  int is_neg = 0;
  if (c == '-') {
    is_neg = 1;
    c = getch();
  }
  // read digits
  int num = 0;
  for (; IS_DIGIT(c); c = getch()) {
    num = num * 10 + c - '0';
  }
  // unget char
  last_char = c;
  last_char_valid = 1;
  return is_neg ? -num : num;
}

int getch() {
  if (last_char_valid) {
    // char buffer is valid, consume the char in it
    last_char_valid = 0;
    return last_char;
  }
  else {
    // char buffer is not valid, read char from stdin
    char c;
    return read(STDIN_FILENO, &c, 1) == 1 ? c : -1;
  }
}

int getarray(int a[]) {
  int n = getint();
  for (int i = 0; i < n; i++) a[i] = getint();
  return n;
}

void putint(int num) { PutInt(STDOUT_FILENO, num); }

void putch(int ch) { PutChar(STDOUT_FILENO, ch); }

void putarray(int n, int a[]) {
  putint(n);
  putch(':');
  for (int i = 0; i < n; i++) {
    putch(' ');
    putint(a[i]);
  }
  putch('\n');
}

// ============================================================
// Implementations of timing functions.
// ============================================================

#define TIMER_COUNT_MAX 1024
static struct timeval timer_start, timer_end;
static int timer_h[TIMER_COUNT_MAX], timer_m[TIMER_COUNT_MAX],
    timer_s[TIMER_COUNT_MAX], timer_us[TIMER_COUNT_MAX];
static int timer_idx = 1;

void __attribute((destructor)) after_main() {
  // clear output buffer
  write(STDOUT_FILENO, output_buffer, output_buffer_index);
  // print timing results
  if (timer_idx <= 1) return;
  for (int i = 1; i < timer_idx; i++) {
    PutString(STDERR_FILENO, "Timer: ");
    PutInt(STDERR_FILENO, timer_h[i]);
    PutString(STDERR_FILENO, "H-");
    PutInt(STDERR_FILENO, timer_m[i]);
    PutString(STDERR_FILENO, "M-");
    PutInt(STDERR_FILENO, timer_s[i]);
    PutString(STDERR_FILENO, "S-");
    PutInt(STDERR_FILENO, timer_us[i]);
    PutString(STDERR_FILENO, "us\n");
    timer_us[0] += timer_us[i];
    timer_s[0] += timer_s[i];
    timer_us[0] %= 1000000;
    timer_m[0] += timer_m[i];
    timer_s[0] %= 60;
    timer_h[0] += timer_h[i];
    timer_m[0] %= 60;
  }
  PutString(STDERR_FILENO, "TOTAL: ");
  PutInt(STDERR_FILENO, timer_h[0]);
  PutString(STDERR_FILENO, "H-");
  PutInt(STDERR_FILENO, timer_m[0]);
  PutString(STDERR_FILENO, "M-");
  PutInt(STDERR_FILENO, timer_s[0]);
  PutString(STDERR_FILENO, "S-");
  PutInt(STDERR_FILENO, timer_us[0]);
  PutString(STDERR_FILENO, "us\n");
}

// Block starttime definition
// void starttime() { gettimeofday(&timer_start, NULL); }

// Block stoptime definition
// void stoptime() {
//   gettimeofday(&timer_end, NULL);
//   timer_us[timer_idx] += 1000000 * (timer_end.tv_sec - timer_start.tv_sec) +
//                          timer_end.tv_usec - timer_start.tv_usec;
//   timer_s[timer_idx] += timer_us[timer_idx] / 1000000;
//   timer_us[timer_idx] %= 1000000;
//   timer_m[timer_idx] += timer_s[timer_idx] / 60;
//   timer_s[timer_idx] %= 60;
//   timer_h[timer_idx] += timer_m[timer_idx] / 60;
//   timer_m[timer_idx] %= 60;
//   timer_idx++;
"""

grammar_lv5 = r"""
?start: comp_unit

comp_unit: func_def

func_def: func_type IDENT "(" ")" block
func_type: "int"

block: "{" block_item* "}"
block_item: decl | stmt

decl: const_decl | var_decl
const_decl: "const" btype const_def ("," const_def)* ";"
const_def: IDENT "=" const_init_val
const_init_val: exp

var_decl: btype var_def ("," var_def)* ";"
var_def: IDENT | IDENT "=" init_val
init_val: exp

stmt: lval "=" exp ";"
    | exp? ";"
    | block
    | "return" exp? ";"

exp: lor_exp
lor_exp: land_exp ("||" land_exp)*
land_exp: eq_exp ("&&" eq_exp)*
eq_exp: rel_exp (("==" | "!=") rel_exp)*
rel_exp: add_exp (("<" | ">" | "<=" | ">=") add_exp)*
add_exp: mul_exp (("+" | "-") mul_exp)*
mul_exp: unary_exp (("*" | "/" | "%") unary_exp)*
unary_exp: primary_exp | unary_op unary_exp
unary_op: "+" | "-" | "!"
primary_exp: "(" exp ")" | lval | number

lval: IDENT
number: INT_CONST
btype: "int"

%import common.CNAME -> IDENT
%import common.INT -> INT_CONST
%import common.WS
%ignore WS
"""

grammar_lv9 = r"""
?start: comp_unit

comp_unit: (decl | func_def)*

decl: const_decl | var_decl

const_decl: "const" btype const_def ("," const_def)* ";"
const_def: IDENT ("[" const_exp "]")* "=" const_init_val
const_init_val: const_exp 
              | "{" (const_init_val ("," const_init_val)*)? "}"

var_decl: btype var_def ("," var_def)* ";"
var_def: IDENT ("[" const_exp "]")* 
       | IDENT ("[" const_exp "]")* "=" init_val
init_val: exp 
        | "{" (init_val ("," init_val)*)? "}"

func_def: func_type IDENT "(" func_fparams? ")" block
func_type: "void" | "int"

func_fparams: func_fparam ("," func_fparam)*
func_fparam: btype IDENT ("[" "]" ("[" const_exp "]")*)?

block: "{" block_item* "}"
block_item: decl | stmt

stmt: lval "=" exp ";"
    | exp? ";"
    | block
    | "if" "(" exp ")" stmt ("else" stmt)?
    | "while" "(" exp ")" stmt
    | "break" ";"
    | "continue" ";"
    | "return" exp? ";"

exp: lor_exp
lor_exp: land_exp ("||" land_exp)*
land_exp: eq_exp ("&&" eq_exp)*
eq_exp: rel_exp (("==" | "!=") rel_exp)*
rel_exp: add_exp (("<" | ">" | "<=" | ">=") add_exp)*
add_exp: mul_exp (("+" | "-") mul_exp)*
mul_exp: unary_exp (("*" | "/" | "%") unary_exp)*
unary_exp: primary_exp
         | IDENT "(" func_rparams? ")"
         | unary_op unary_exp
unary_op: "+" | "-" | "!"

func_rparams: exp ("," exp)*

primary_exp: "(" exp ")" | lval | number
lval: IDENT ("[" exp "]")*
number: INT_CONST
INT_CONST: /0[xX][0-9a-fA-F]+/ | /0[bB][01]+/ | /[1-9][0-9]*|0/
const_exp: exp
btype: "int"

%import common.CNAME -> IDENT
%import common.WS
%ignore WS
"""

def parse_idea(idea: str):
    # 使用正则匹配 .c 和 .in 段
    match = re.match(r'^\*{6}\.c\*{6}\n(.*?)(?:\n\*{6}\.in\*{6}\n(.*))?$', idea, re.DOTALL)
    if not match:
        raise ValueError("输入格式不符合要求")
    
    c_file = match.group(1).strip()
    in_file = match.group(2).strip() if match.group(2) is not None else None
    return c_file, in_file

def remove_comments(code: str) -> str:
    # 去除 // 单行注释
    code = re.sub(r"//.*?$", "", code, flags=re.MULTILINE)
    # 去除 /* 多行注释 */
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.DOTALL)
    return code


class SysYValidator(Transformer):
    def const_exp(self, children):
        node = children[0]
        if self.contains_constexp_disallowed_expr(node):
            raise ValueError(
                "SysY语言的常整数（const int）定义与C语言有所不同，"
                "ConstExp必须能在编译期被求值，因此不能含有函数调用或变量/数组引用！"
            )
        return node
    
    def init_val(self, children):
        node = Tree("init_val", children)
        if isinstance(children[0], Tree) and children[0].data == "init_val":
            if self.contains_initval_disallowed_expr(node):
                raise ValueError(
                    "为了避免函数求值顺序不确定所导致的未定义行为，"
                    "用于初始化数组的InitVal下不可以出现函数调用！"
                )
        return node

    def contains_constexp_disallowed_expr(self, node):
        if isinstance(node, Tree):
            # 禁止 lval 节点
            if node.data == "lval":
                return True
            # 禁止函数调用：unary_exp -> IDENT "(" ...
            if node.data == "unary_exp":
                if len(node.children) >= 1 and isinstance(node.children[0], Token) and node.children[0].type == "IDENT":
                    return True
            # 递归检查所有子节点
            return any(self.contains_constexp_disallowed_expr(child) for child in node.children)
        return False
    
    def contains_initval_disallowed_expr(self, node):
        if isinstance(node, Tree):
            # 禁止函数调用：unary_exp -> IDENT "(" ...
            if node.data == "unary_exp":
                if len(node.children) >= 1 and isinstance(node.children[0], Token) and node.children[0].type == "IDENT":
                    return True
            # 递归检查所有子节点
            return any(self.contains_initval_disallowed_expr(child) for child in node.children)
        return False

def check_c_code(c_file : str, grammar : str) -> tuple[bool, str]:
    parser = Lark(grammar)
    try:
        tree = parser.parse(remove_comments(c_file))
        validator = SysYValidator()
        validator.transform(tree)
        return True, None
    except Exception as e:
        return False, e


def check_semantic_correctness(c_code: str) -> tuple[bool, str]:
    with tempfile.NamedTemporaryFile(suffix=".c", delete=False) as tmp_c_file:
        tmp_c_file.write(c_code.encode())
        tmp_c_file.flush()
        c_filename = tmp_c_file.name

    exe_filename = c_filename + ".out"
    
    try:
        try:
            # 设置 timeout=20 限制运行时间为 20 秒
            result = subprocess.run(
                [
                    "gcc", "-Wuninitialized", "-Werror", "-O1",
                    c_filename, "-o", exe_filename
                ],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.PIPE,
                text=True,
                timeout=20  # ⏱️ 设置时间限制为 20 秒
            )
            success = result.returncode == 0
            error_message = result.stderr.strip()
            return success, error_message
        except subprocess.TimeoutExpired:
            raise RuntimeError("gcc 编译超时：超过 20 秒未完成")
    finally:
        os.remove(c_filename)
        if os.path.exists(exe_filename):
            os.remove(exe_filename)


def evaluate(idea: str) -> tuple[float, str]:
    """
    对语言模型生成的答案进行评估，返回分数和评语。

    Args:
        idea (str): 语言模型生成的程序/文本。

    Returns:
        tuple[float, str]: 包含两个元素的元组：
            - float: 回答的评分（0~100）。
            - str: 对回答的简要评语或解释信息（可为 None）。
    """
    try:
        c_file, in_file = parse_idea(idea)
    except:
        score = 0.00
        info = (
            "这个测试用例的格式不正确，没有以******.c******开头，"
            "或者没有用******.in******来标识可选的input section！\n"
        )
        return score, info
    
    c_code_correctness, error = check_c_code(c_file, grammar_lv9)
    
    if not c_code_correctness:
        score = 0.00
        info = (
            f"这个测试用例的.c程序部分有语法错误：{error}！"
        )
        return score, info
    
    semantic_correctness, semantic_error = check_semantic_correctness(SysY_header + c_file)
    
    if not semantic_correctness:
        score = 0.00
        info = (
            f"这个测试用例的.c程序部分无法通过gcc编译，疑似有语义错误！\n"
            "错误信息：\n"
            f"{semantic_error}"
        )
        return score, info
    
    score = 60.00
    info = "这个测试用例符合格式要求，语法正确，语义正确，挺好的！可以向它学习，但不要过于模仿它。"
    return score, info

def analyze_idea(idea_path, output_path):
    # 读取 idea 文件内容
    print(f"analyzing {idea_path}")
    with open(idea_path, 'r', encoding='utf-8') as f:
        idea = f.read()
    
    # 解析出 .c 和 .in 内容
    c_file, in_file = parse_idea(idea)

    # 构造文件名基础（无后缀）
    base_filename = os.path.splitext(os.path.basename(idea_path))[0]
    if not os.path.exists(output_path):
        os.mkdir(output_path)

    # 输出文件路径
    c_filepath = os.path.join(output_path, base_filename + '.c')
    c_filepath_full = os.path.join(output_path, base_filename + '_full.c')
    in_filepath = os.path.join(output_path, base_filename + '.in') if in_file else None
    out_filepath = os.path.join(output_path, base_filename + '.out')
    exe_filepath = os.path.join(output_path, base_filename + '.exe')

    # 写入 .c 和 .c_full 文件
    with open(c_filepath, 'w', encoding='utf-8', newline = "\n") as f:
        f.write(c_file)
    with open(c_filepath_full, 'w', encoding='utf-8', newline = "\n") as f:
        f.write(SysY_header + c_file)

    # 写入 .in 文件（如果存在）
    if in_file is not None:
        with open(in_filepath, 'w', encoding='utf-8', newline = "\n") as f:
            f.write(in_file)
            
    compile_timeout = 3
    run_timeout = 3

    # 使用 gcc 编译 .c_full 文件
    try:
        compile_result = subprocess.run(
            ['gcc', c_filepath_full, '-o', exe_filepath],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=compile_timeout,
        )
    except Exception as e:
        if isinstance(e, subprocess.TimeoutExpired):
            print(f"在analyze {idea_path}的过程中发生了错误：编译时间超过了限定的{compile_timeout:.1f}秒！")
        else:
            print(f"在analyze {idea_path}的过程中发生了错误：\n{e}")
        return

    if compile_result.returncode != 0:
        # 编译失败：写入 stderr 到 .out 文件
        with open(out_filepath, 'w', encoding='utf-8') as f:
            f.write("编译失败！\n")
            f.write(compile_result.stderr)
    else:
        with open(out_filepath, 'w', encoding='utf-8', newline='\n') as fout:
            if in_file is not None:
                with open(in_filepath, 'r', encoding='utf-8') as fin:
                    try:
                        run_result = subprocess.run(
                            [exe_filepath],
                            stdin=fin,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT,
                            text=True,
                            timeout=run_timeout,
                        )
                    except Exception as e:
                        if isinstance(e, subprocess.TimeoutExpired):
                            print(f"在analyze {idea_path}的过程中发生了错误：运行时间超过了限定的{run_timeout:.1f}秒！")
                        else:
                            print(f"在analyze {idea_path}的过程中发生了错误：\n{e}")
                        return
                    
            else:
                try:
                    run_result = subprocess.run(
                        [exe_filepath],
                        stdout=subprocess.PIPE,
                        stderr=subprocess.STDOUT,
                        text=True,
                        timeout=run_timeout,
                    )
                except Exception as e:
                    if isinstance(e, subprocess.TimeoutExpired):
                        print(f"在analyze {idea_path}的过程中发生了错误：运行时间超过了限定的{run_timeout:.1f}秒！")
                    else:
                        print(f"在analyze {idea_path}的过程中发生了错误：\n{e}")
                    return
                
            # 统一换行符为 \n
            output = run_result.stdout.replace('\r\n', '\n')

            if not output:
                fout.write(f"{run_result.returncode}\n")
            elif output.endswith('\n'):
                fout.write(output)
                fout.write(f"{run_result.returncode}\n")
            else:
                fout.write(output)
                fout.write(f"\n{run_result.returncode}\n")
                
    # 清理临时文件
    try:
        if os.path.exists(exe_filepath):
            os.remove(exe_filepath)
        if os.path.exists(c_filepath_full):
            os.remove(c_filepath_full)
    except Exception as e:
        print(f"在analyze {idea_path}的过程中发生了错误：清理临时文件时，\n{e}")
        
        
        
def guarantee_path_exist(path):
    if not os.path.exists(path): os.makedirs(path)

# some parameters
TEMP_OUTPUT_FILE = f'{str(uuid.uuid4()).replace("-", "")}.S'
TEMP_OBJECT_FILE = f'{str(uuid.uuid4()).replace("-", "")}.o'
TEMP_EXECUTABLE_FILE = f'{str(uuid.uuid4()).replace("-", "")}'
COMP_TIMEOUT_SEC = 300
ASM_TIMEOUT_SEC = 60
RUN_TIMEOUT_SEC = 120
RE_TOTAL_TIME = re.compile(r'TOTAL: (\d+H-\d+M-\d+S-\d+us)')

@unique
class TestMode(Enum):
    '''
    Test mode.
    '''
    KOOPA = auto()
    RISCV = auto()
    PERF = auto()

    def to_opt(self) -> str:
        '''
        Converts the current test mode to command line option.
        '''
        return {
            TestMode.KOOPA: '-koopa',
            TestMode.RISCV: '-riscv',
            TestMode.PERF: '-perf',
        }[self]

def eprint(*args, **kwargs):
    '''
    Prints to `stderr`.
    '''
    print(*args, file=sys.stderr, **kwargs)
    sys.stderr.flush()
    
def decode_bytes(b: bytes) -> str:
    '''
    Decodes bytes to UTF-8 string.
    '''
    try:
        return b.decode('utf-8')
    except UnicodeDecodeError:
        return str(b)


def decode_result(result: 'subprocess.CompletedProcess[bytes]') -> Tuple[str, str]:
    '''
    Decodes result returned by `subprocess.run`, extracts `stdout` and `stderr`.
    '''
    return (decode_bytes(result.stdout), decode_bytes(result.stderr))

@dataclass(frozen=True)
class CompilerInfo:
    '''
    Compiler related information.
    '''
    working_dir: str
    compile_cmd: str
    clean_wd: bool

    def clean(self):
        '''
        Deletes the working directory.
        '''
        if self.clean_wd:
            shutil.rmtree(self.working_dir)


@dataclass(frozen=True)
class TestCase:
    '''
    Test case information.
    '''
    name: str
    source_file: str
    output_file: str
    input_file: Optional[str]


@unique
class TestStatus(Enum):
    '''
    Status of a test.
    '''
    PASSED = auto()
    COMP_ERROR = auto()
    COMP_TIME_EXCEEDED = auto()
    OUTPUT_NOT_FOUND = auto()
    ASM_ERROR = auto()
    ASM_TIME_EXCEEDED = auto()
    RUN_ERROR = auto()
    RUN_TIME_EXCEEDED = auto()
    WRONG_ANSWER = auto()

    def to_msg(self):
        '''
        Converts the current test status to `eprint` message.
        '''
        return {
            TestStatus.PASSED: '\033[0;32mPASSED\033[0m',
            TestStatus.COMP_ERROR: '\033[0;35mCASE COMPILE ERROR\033[0m',
            TestStatus.COMP_TIME_EXCEEDED: '\033[0;34mCASE COMPILE TIME EXCEEDED\033[0m',
            TestStatus.OUTPUT_NOT_FOUND: '\033[0;31mOUTPUT NOT FOUND\033[0m',
            TestStatus.ASM_ERROR: '\033[0;35mCASE ASSEMBLE ERROR\033[0m',
            TestStatus.ASM_TIME_EXCEEDED: '\033[0;34mCASE ASSEMBLE TIME EXCEEDED\033[0m',
            TestStatus.RUN_ERROR: '\033[0;35mRUNTIME ERROR\033[0m',
            TestStatus.RUN_TIME_EXCEEDED: '\033[0;34mTIME LIMIT EXCEEDED\033[0m',
            TestStatus.WRONG_ANSWER: '\033[0;31mWRONG ANSWER\033[0m',
        }[self]


@dataclass(frozen=True)
class TestResult:
    '''
    Result of a single test.
    '''
    status: TestStatus
    answer: str = ''
    time: Optional[str] = None
    output_error: Optional[Tuple[str, str]] = None

    def show_details(self):
        '''
        Prints the details of the current test result to `stderr`.
        '''
        eprint(self.status.to_msg())
        if self.status == TestStatus.WRONG_ANSWER:
            eprint('your answer:')
            eprint(self.answer)
        elif self.status != TestStatus.PASSED and self.output_error:
            eprint('stdout:')
            eprint(self.output_error[0])
            eprint('stderr:')
            eprint(self.output_error[1])

    def show_perf(self):
        '''
        Prints the performance test result to `stderr`.
        '''
        eprint(f'time elapsed: {self.time}')
        
def execute(cmd: str, timeout: int, error_status: TestStatus,
            timeout_status: TestStatus,
            pipe_cmd: Optional[str] = None) -> Optional[TestResult]:
    '''
    Executes the given command line, returns test result if failed.
    '''
    try:
        if pipe_cmd:
            pipe = subprocess.Popen(shlex.split(pipe_cmd), stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE)
            result = subprocess.run(shlex.split(cmd), timeout=timeout, stdin=pipe.stdout,
                                    stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            if pipe.wait():
                return TestResult(error_status,
                                  output_error=(decode_bytes(pipe.stdout.read()),
                                                decode_bytes(pipe.stderr.read())))
        else:
            result = subprocess.run(shlex.split(cmd), timeout=timeout,
                                    stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if result.returncode:
            return TestResult(error_status, output_error=decode_result(result))
    except subprocess.TimeoutExpired:
        return TestResult(timeout_status)
    return None
            
def sync_testcase_path_with_idea_database(
    idea_database_path : str,
    testcase_path : str,
) -> None:
    
    # 清空 testcase_path 文件夹（若存在），然后重建
    testcase_dir = Path(testcase_path)
    if testcase_dir.exists():
        shutil.rmtree(testcase_dir)
    testcase_dir.mkdir(parents=True, exist_ok=True)
    
    for path in Path(idea_database_path).rglob('*.idea'):
        
        with open(path, "r", encoding = "UTF-8") as file:
            idea = file.read()

        score, _ = evaluate(idea)
        if score < 60.00: continue
        
        analyze_idea(
            idea_path = path,
            output_path = testcase_path
        )
        
        
if __name__ == "__main__":
    
    sync_testcase_path_with_idea_database(
        idea_database_path = "D:\MyGithubPrograms\IdeaSearch\programs\SysYCompilerTest\database",
        testcase_path = "D:\MyGithubPrograms\SysYCompilerTest\TestcaseSets\FunSearch\lv9",
    )