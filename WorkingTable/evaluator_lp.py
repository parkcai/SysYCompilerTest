import re
import os
import subprocess
import tempfile
import shutil
import numpy as np
from bs4 import BeautifulSoup
from lark import Lark, Transformer, Tree, Token
from typing import Optional
from programs.SysYCompilerTest.user_code.grammar import grammar_lv5, grammar_lv9
from programs.SysYCompilerTest.user_code.SysY_header import SysY_header


__all__ = [
    "evaluate",
]


SysYCompilerTest_random_generator = np.random.default_rng()


def evaluate(idea: str)-> tuple[float, Optional[str]]:
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
    coverage_full_mark = 30.0
    complexity_full_mark = 10.0
    
    coverage_score = (get_coverage_score(c_file = c_file) / 100.0) * coverage_full_mark
    complexity_score = (get_complexity_score(c_file = c_file) / 100.0) * complexity_full_mark
    
    score += coverage_score + complexity_score
    info = (
        "这个测试用例格式正确，语法正确，语义正确，满足基本要求！\n"
        f"在基础分60.00之上，它获得了{coverage_score:.2f}分的覆盖率得分（该项满分{coverage_full_mark:.2f}分）"
        f"和{complexity_score:.2f}分的复杂度得分（该项满分{complexity_full_mark:.2f}分）。\n"
        "可以向它学习，但不要过于模仿它。"
    )
    
    return score, info


def get_coverage_score(
    compiler_path: str,
    c_file: str,
) -> float:
    with tempfile.TemporaryDirectory() as temp_dir:
        c_path = os.path.join(temp_dir, "test.c")
        with open(c_path, "w") as f:
            f.write(c_file)
        
        exe_path = os.path.join(temp_dir, "test")
        compile_cmd = [
            compiler_path,
            "-fprofile-arcs",
            "-ftest-coverage",
            c_path,
            "-o", exe_path,
        ]
        try:
            subprocess.run(
                compile_cmd,
                check=True,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.PIPE,
            )
        except subprocess.CalledProcessError as e:
            return 0.0  
        
        try:
            subprocess.run(
                [exe_path],
                cwd=temp_dir,
                check=True,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                timeout=10,
            )
        except:
            pass  
        
        try:
            subprocess.run(
                ["lcov", "--capture", "--directory", temp_dir, "--output-file", "coverage.info"],
                cwd=temp_dir,
                check=True,
            )
            subprocess.run(
                ["genhtml", "coverage.info", "--output-directory", "htmlcov"],
                cwd=temp_dir,
                check=True,
            )
        except:
            return 0.0  
        
        html_path = os.path.join(temp_dir, "htmlcov", "index.html")
        if not os.path.exists(html_path):
            return 0.0
        
        with open(html_path, "r", encoding="utf-8") as f:
            soup = BeautifulSoup(f.read(), "html.parser")
        
        coverage_span = soup.find("span", class_="pc_cov")
        if not coverage_span:
            return 0.0
        
        try:
            return float(coverage_span.text.strip().rstrip("%"))
        except:
            return 0.0


def get_complexity_score(
    c_file: str,
)-> float:
    
    return SysYCompilerTest_random_generator.uniform(0.0, 100.0)
    
    
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