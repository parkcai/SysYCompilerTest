#!/usr/bin/env python3


'''
[Acknowledgement]   Zeyu Cai   25/04/08
    This testing script of our Software Engineering homework relies heavily on 
    MaxXing's autotest.py for PKU Compiler Lab 
    (https://github.com/pku-minic/compiler-dev/blob/master/autotest/autotest).
    A lot of thanks to dear MaxXing and other fellow seniors!
'''


from dataclasses import dataclass
from enum import Enum, auto, unique
import multiprocessing
import os
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
from typing import List, Optional, Tuple
import uuid


# some parameters
LIBRARY_PATH = os.environ['CDE_LIBRARY_PATH']
INCLUDE_PATH = os.environ['CDE_INCLUDE_PATH']
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


def eprint(*args, **kwargs):
    '''
    Prints to `stderr`.
    '''
    print(*args, file=sys.stderr, **kwargs)
    sys.stderr.flush()


def files_exists(base_dir: str, *files: str) -> Optional[str]:
    '''
    Checks if the specific files exists.

    Returns file path if exists, otherwise returns `None`.
    '''
    for file in files:
        file = os.path.join(base_dir, file)
        if os.path.exists(file):
            return file
    return None


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


def parse_time_info(stderr: str) -> str:
    '''
    Parses time information from the given `stderr`.
    '''
    time = RE_TOTAL_TIME.findall(stderr)
    if time:
        time = time[0]
    else:
        time = 'time information not found'
    return time


def remove_prefix(s: str, prefix: str) -> str:
    '''
    Removes the prefix from the given string.
    '''
    if s.startswith(prefix):
        return s[len(prefix):]
    return s


def scan_test_cases(test_case_dir: str) -> List[TestCase]:
    '''
    Scans all test cases recursively in the given test case directory.

    Returns the list of test case paths.
    '''
    cases = []
    # walk in test case directory
    for root, _, files in os.walk(test_case_dir):
        prefix = remove_prefix(root, test_case_dir).lstrip('/')
        files = set(files)
        for file in files:
            # check if is SysY source file
            if file.endswith('.c') or file.endswith('.sy'):
                name = os.path.splitext(file)[0]
                out_file = f'{name}.out'
                # check if output file exists
                if out_file in files:
                    source_file = os.path.join(root, file)
                    output_file = os.path.join(root, out_file)
                    in_file = f'{name}.in'
                    input_file = os.path.join(root, in_file) if in_file in files else None
                    # add to cases list
                    name = os.path.join(prefix, name)
                    cases.append(TestCase(name, source_file, output_file, input_file))
    cases.sort(key=lambda x: x.source_file)
    return cases


def build_make_project(repo_dir: str, working_dir: str) -> Tuple[str, str]:
    '''
    Returns build command and compile command of the given Make project.
    '''
    flags = f'LIB_DIR="{LIBRARY_PATH}/native" INC_DIR="{INCLUDE_PATH}"'
    build_cmd = f'make DEBUG=0 BUILD_DIR="{working_dir}" {flags} -C "{repo_dir}"'
    comp_cmd = os.path.join(working_dir, 'compiler')
    return (build_cmd, f'"{comp_cmd}"')


def build_cmake_project(repo_dir: str, workding_dir: str) -> Tuple[str, str]:
    '''
    Returns build command and compile command of the given CMake project.
    '''
    # generate build configuration
    flags = f'-DLIB_DIR="{LIBRARY_PATH}/native" -DINC_DIR="{INCLUDE_PATH}"'
    gen_cmd = f'cmake -S "{repo_dir}" -B "{workding_dir}" {flags}'
    result = subprocess.run(shlex.split(gen_cmd))
    if result.returncode:
        msg = f'failed to generate the build configuration of repository with CMake\n'
        msg += f'CMake command: {gen_cmd}'
        raise RuntimeError(msg)
    # get command lines
    build_cmd = f'cmake --build "{workding_dir}" -j {multiprocessing.cpu_count()}'
    comp_cmd = os.path.join(workding_dir, 'compiler')
    return (build_cmd, f'"{comp_cmd}"')


def build_cargo_project(manifest: str, workding_dir: str) -> Tuple[str, str]:
    '''
    Returns build command and compile command of the given Cargo project.
    '''
    flags = f'--manifest-path="{manifest}" --target-dir="{workding_dir}" --release'
    return (f'cargo build {flags}', f'cargo run {flags} -q --')


def build_repo(
    repo_dir: str,
    working_dir: Optional[str] = None
) -> Optional[CompilerInfo]:
    '''
    Builds the given repository.

    Returns compiler information.
    '''
    # initialize working directory
    if not working_dir:
        working_dir = tempfile.mkdtemp()
        clean_wd = True
    else:
        working_dir = os.path.abspath(working_dir)
        clean_wd = False
    eprint(f'working directory: {working_dir}')
    try:
        # check if is Make/CMake/Cargo project
        if files_exists(repo_dir, 'Makefile', 'makefile'):
            (build_cmd, compile_cmd) = build_make_project(repo_dir, working_dir)
        elif files_exists(repo_dir, 'CMakeLists.txt'):
            (build_cmd, compile_cmd) = build_cmake_project(repo_dir, working_dir)
        elif manifest := files_exists(repo_dir, 'Cargo.toml'):
            (build_cmd, compile_cmd) = build_cargo_project(manifest, working_dir)
        else:
            eprint(f'repository "{repo_dir}" does not contain ' +
                   'any build configurations')
            return None
        # build repository
        result = subprocess.run(shlex.split(build_cmd))
        if result.returncode:
            eprint('failed to build the repository')
            eprint(f'compilation command: {build_cmd}')
            return None
        # check if the compiler has been generated properly
        if compile_cmd.startswith('"'):
            compiler_file = compile_cmd.strip('"')
            if not os.path.exists(compiler_file):
                eprint(f'compiler "{compile_cmd}" not found')
                eprint('please check your Make/CMake build configuration')
                return None
        return CompilerInfo(working_dir, compile_cmd, clean_wd)
    except Exception as e:
        eprint(e)
        # delete the working directory
        if clean_wd:
            shutil.rmtree(working_dir)
        return None


def asm_koopa(output: str, obj: str, exe: str) -> Optional[TestResult]:
    '''
    Converts Koopa IR to executable.
    '''
    if result := execute(f'llc --filetype=obj -o {obj}', ASM_TIMEOUT_SEC,
                         TestStatus.ASM_ERROR, TestStatus.ASM_TIME_EXCEEDED,
                         pipe_cmd=f'koopac {output}'):
        return result
    lib_flags = f'-L{LIBRARY_PATH}/native -lsysy'
    cmd = f'clang {obj} {lib_flags} -o {exe}'
    if result := execute(cmd, ASM_TIMEOUT_SEC, TestStatus.ASM_ERROR,
                         TestStatus.ASM_TIME_EXCEEDED):
        return result
    return None


def asm_riscv(output: str, obj: str, exe: str) -> Optional[TestResult]:
    '''
    Converts RISC-V assembly to executable.
    '''
    asm_flags = '-target riscv32-unknown-linux-elf -march=rv32im -mabi=ilp32'
    cmd = f'clang {output} -c -o {obj} {asm_flags}'
    if result := execute(cmd, ASM_TIMEOUT_SEC, TestStatus.ASM_ERROR,
                         TestStatus.ASM_TIME_EXCEEDED):
        return result
    lib_flags = f'-L{LIBRARY_PATH}/riscv32 -lsysy'
    cmd = f'ld.lld {obj} {lib_flags} -o {exe}'
    if result := execute(cmd, ASM_TIMEOUT_SEC, TestStatus.ASM_ERROR,
                         TestStatus.ASM_TIME_EXCEEDED):
        return result
    return None


def run_output(mode: TestMode, case: TestCase, exe: str) -> TestResult:
    '''
    Runs the generated output executable and checks the result.
    '''
    # execute output file
    inputs = None
    if case.input_file:
        with open(case.input_file, 'r') as f:
            inputs = f.read().encode('utf-8')
    if mode != TestMode.KOOPA:
        exe = f'qemu-riscv32-static {exe}'
    try:
        result = subprocess.run(shlex.split(exe), timeout=RUN_TIMEOUT_SEC,
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                input=inputs)
    except subprocess.TimeoutExpired:
        return TestResult(TestStatus.RUN_TIME_EXCEEDED)
    # get actual output
    (stdout, stderr) = decode_result(result)
    if not stdout or stdout.endswith('\n'):
        out = f'{stdout}{result.returncode}'
    else:
        out = f'{stdout}\n{result.returncode}'
    # get expected output
    with open(case.output_file, mode='r', newline='') as f:
        ref = f.read().rstrip()
    # get time information
    time = None
    if mode == TestMode.PERF:
        time = parse_time_info(stderr)
    # generate result
    status = TestStatus.PASSED if out == ref else TestStatus.WRONG_ANSWER
    return TestResult(status, out, time, (stdout, stderr))


def run_test(mode: TestMode, compiler: CompilerInfo, case: TestCase) -> TestResult:
    '''
    Runs the test for the given compiler by using the given test case.

    Returns test result.
    '''
    output = os.path.join(compiler.working_dir, TEMP_OUTPUT_FILE)
    obj = os.path.join(compiler.working_dir, TEMP_OBJECT_FILE)
    exe = os.path.join(compiler.working_dir, TEMP_EXECUTABLE_FILE)
    # compile test case
    cmd = f'{compiler.compile_cmd} {mode.to_opt()} {case.source_file} -o {output}'
    if result := execute(cmd, COMP_TIMEOUT_SEC, TestStatus.COMP_ERROR,
                         TestStatus.COMP_TIME_EXCEEDED):
        return result
    # check output file
    if not os.path.exists(output):
        return TestResult(TestStatus.OUTPUT_NOT_FOUND)
    # assembly output file
    asm = asm_koopa if mode == TestMode.KOOPA else asm_riscv
    if result := asm(output, obj, exe):
        return result
    return run_output(mode, case, exe)


def run_tests(mode: TestMode, compiler: CompilerInfo, cases: List[TestCase]):
    '''
    Runs test for all test cases.

    Prints test results to `stderr` when necessary.
    '''
    passed = 0
    # run all tests
    for case in cases:
        eprint(f'running test "{case.name}" ... ', end='')
        result = run_test(mode, compiler, case)
        result.show_details()
        if result.status == TestStatus.PASSED:
            if mode == TestMode.PERF:
                result.show_perf()
            passed += 1
    # print test result
    total = len(cases)
    status = TestStatus.PASSED if passed == total else TestStatus.WRONG_ANSWER
    eprint(f'{status.to_msg()} ({passed}/{total})')
    
    
# ---------------------- parkcai append ----------------------


COMPILERS_PATH = "/root/SysYCompilerTest/Compilers/"
TESTCASESETS_PATH = "/root/SysYCompilerTest/TestcaseSets/"


def get_testcase_names(testcase_path: str) -> List[str]:
    
    result = []
    
    # 遍历指定目录
    for _, _, files in os.walk(testcase_path):
        # 使用集合加速查找
        file_set = set(files)
        
        # 检查每个文件名
        for file in files:
            if file.endswith('.c'):
                # 获取不带扩展名的文件名
                base_name = os.path.splitext(file)[0]
                # 检查对应的 .out 文件是否存在
                if f"{base_name}.out" in file_set:
                    result.append(base_name)
    
    return result


def test_compiler_with_testcase_set(
    compiler_name: str,
    compiler_level: int,
    testcase_set_name: str,
    testcase_level: int 
) -> float:
    
    compiler_to_test = build_repo(
        repo_dir = COMPILERS_PATH + f"{compiler_name}/lv{compiler_level}/",
        working_dir = COMPILERS_PATH + f"{compiler_name}/lv{compiler_level}/build",
    ) 
    
    run_tests(
        mode = TestMode.RISCV,
        compiler = compiler_to_test,
        cases=[
            TestCase(
                name = testcase_name,
                source_file = TESTCASESETS_PATH + f"{testcase_set_name}/lv{testcase_level}/{testcase_name}.c",
                output_file = TESTCASESETS_PATH + f"{testcase_set_name}/lv{testcase_level}/{testcase_name}.out",
                input_file = TESTCASESETS_PATH + f"{testcase_set_name}/lv{testcase_level}/{testcase_name}.in"
                            if os.path.exists(TESTCASESETS_PATH + f"{testcase_set_name}/lv{testcase_level}/{testcase_name}.in")
                            else None,
            ) for testcase_name in get_testcase_names(
                testcase_path = TESTCASESETS_PATH + f"{testcase_set_name}/lv{testcase_level}/"
            )
        ]
    )
    
    compiler_to_test.clean()
    
    
if __name__ == "__main__":
    
    test_compiler_with_testcase_set(
        compiler_name = "Compiler1",
        compiler_level = 9,
        testcase_set_name = "CourseOriginal",
        testcase_level = 5,
    )
    
    