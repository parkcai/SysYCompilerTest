import os
import subprocess
from typing import Optional,List




def compiler_coverage_score(
    compiler_path: List[str],
    c_source_codes: List[str],
) -> Optional[float]:
    # 创建一个列表来存储每个编译器的分数
    scores = []

    # 遍历每个编译器路径
    for compiler in compiler_path:
        # 创建一个列表来存储 profraw 文件名
        profraw_files = []

        # 遍历每个 C 源代码文件
        for c_file in c_source_codes:
            # 提取文件名（不包含扩展名）作为 profraw 和输出文件名的一部分
            file_name_without_ext = os.path.splitext(os.path.basename(c_file))[0]
            profraw_file = f"{file_name_without_ext}.profraw"
            output_file = f"{file_name_without_ext}.riscv.S"

            # 构建编译命令并执行
            compile_cmd = [
                f"LLVM_PROFILE_FILE={profraw_file}",
                compiler,
                "-riscv",
                c_file,
                "-o",
                output_file
            ]
            # 执行编译命令
            compile_result = subprocess.run(" ".join(compile_cmd), shell=True)
            if compile_result.returncode != 0:
                print(f"编译文件 {c_file} 失败")
                return None  # 编译失败按需处理，这里直接返回 None

            # 将生成的 profraw 文件名添加到列表中
            profraw_files.append(profraw_file)

        # 检查是否有 profraw 文件生成
        if not profraw_files:
            print("没有找到 C 文件或生成 profraw 文件失败")
            return None

        # 合并 profraw 文件为 profdata 文件
        file_name_without_ext = os.path.splitext(os.path.basename(compiler))[0]
        profdata_file = f"default_{file_name_without_ext}.profdata"
        merge_cmd = [
            "llvm-profdata-13",
            "merge",
            "-sparse",
            "-o",
            profdata_file
        ] + profraw_files
        merge_result = subprocess.run(merge_cmd)
        if merge_result.returncode != 0:
            print("合并 profraw 文件失败")
            return None  # 合并失败按需处理，这里直接返回 None

        # 将报告重定向到文件
        report_file = f"coverage_report_{file_name_without_ext}.txt"
        report_cmd = [
            "llvm-cov",
            "report",
            compiler,
            "-instr-profile=" + profdata_file,
            ">",
            report_file
        ]
        report_result = subprocess.run(" ".join(report_cmd), shell=True)
        if report_result.returncode != 0:
            print("生成覆盖率报告失败")
            return None

        # 从报告文件中提取分数
        try:
            with open(report_file, "r") as f:
                report_content = f.read()

            # 最后一行是包含块覆盖，函数覆盖，分支覆盖覆盖率的行
            lines = report_content.splitlines()

            # 清理副作用
            subprocess.run("rm -f *.profraw *.profdata *.riscv.S coverage_report.txt")

            if lines:
                last_line = lines[-1]
                # 提取百分比数值部分
                parts = last_line.split()
                percentages = [float(part.strip("%")) for part in parts if '%' in part]
                first_score = percentages[0]
                last_score = percentages[-1]
                # 取块覆盖，分支覆盖平均值
                average_score = (first_score + last_score) / 2
                scores.append(average_score)  # 将当前编译器的分数添加到列表中
            else:
                print("报告文件内容为空")
                return None
        except Exception as e:
            print(f"读取或解析报告文件出错：{e}")
            return None

    # 计算所有编译器的平均分数
    if scores:
        return sum(scores) / len(scores)
    


def compiler_coverage_score_original(
    compiler_path: str, 
    c_files_folder_path: str, 
)-> Optional[float]:
    # 创建一个列表来存储 profraw 文件名
    profraw_files = []

    # 遍历 C 文件夹中的所有 C 文件
    for root, dirs, files in os.walk(c_files_folder_path):
        for file in files:
            if file.endswith(".c"):
                c_file_path = os.path.join(root, file)
                # 提取文件名（不包含扩展名）作为 profraw 和输出文件名的一部分
                file_name_without_ext = os.path.splitext(file)[0]
                profraw_file = f"{file_name_without_ext}.profraw"
                output_file = f"{file_name_without_ext}.riscv.S"

                # 构建编译命令并执行
                compile_cmd = [
                    f"LLVM_PROFILE_FILE={profraw_file}",
                    compiler_path,
                    "-riscv",
                    c_file_path,
                    "-o",
                    output_file
                ]
                # 执行编译命令
                compile_result = subprocess.run(" ".join(compile_cmd), shell=True)
                if compile_result.returncode != 0:
                    print(f"编译文件 {c_file_path} 失败")
                    return None  # 编译失败按需处理，这里直接返回 None

                # 将生成的 profraw 文件名添加到列表中
                profraw_files.append(profraw_file)

    # 检查是否有 profraw 文件生成
    if not profraw_files:
        print("没有找到 C 文件或生成 profraw 文件失败")
        return None

    # 合并 profraw 文件为 profdata 文件
    profdata_file = "default.profdata"
    merge_cmd = [
        "llvm-profdata-13",
        "merge",
        "-sparse",
        "-o",
        profdata_file
    ] + profraw_files
    merge_result = subprocess.run(merge_cmd)
    if merge_result.returncode != 0:
        print("合并 profraw 文件失败")
        return None  # 合并失败按需处理，这里直接返回 None

    # 将报告重定向到文件
    report_file = "coverage_report.txt"
    report_cmd = [
        "llvm-cov",
        "report",
        compiler_path,
        "-instr-profile=" + profdata_file,
        ">",
        report_file
    ]
    report_result = subprocess.run(" ".join(report_cmd), shell=True)
    if report_result.returncode != 0:
        print("生成覆盖率报告失败")
        return None

    # 从报告文件中提取分数
    try:
        with open(report_file, "r") as f:
            report_content = f.read()

        # 最后一行是包含块覆盖，函数覆盖，分支覆盖覆盖率的行
        lines = report_content.splitlines()

        #清理（想观察报告则注释掉此步）
        clean_cmd = ["make", "coverage_clean"]
        subprocess.run(clean_cmd)

        if lines:
            last_line = lines[-1]
            # 提取百分比数值部分
            parts = last_line.split()
            percentages = [float(part.strip("%")) for part in parts if '%' in part]
            first_score = percentages[0]
            last_score = percentages[-1]
            # 取块覆盖，分支覆盖平均值
            average_score = (first_score + last_score) / 2
            return average_score  # 直接返回百分比数值（满分 100）
        else:
            print("报告文件内容为空")
            return None
    except Exception as e:
        print(f"读取或解析报告文件出错：{e}")
        return None
        
    
def main():
    compiler_path = "/root/SysYCompilerTest/Compilers/Compiler2/lv9/build/compiler"  # 修改为你的编译器实际路径
    c_files_folder_path = "/root/SysYCompilerTest/TestcaseSets/CourseOriginal/lv8"  # 修改为你的 C 文件夹实际路径

    try:
        score = compiler_coverage_score(compiler_path, c_files_folder_path)
        if score is not None:
            print(f"覆盖率分数：{score:.2f}")
        else:
            print("未能获取覆盖率分数")
    except Exception as e:
        print(f"程序运行出错：{e}")


if __name__ == "__main__":
    main()
    #usage : python3 /root/SysYCompilerTest/WorkingTable/assessment.py
    #lv8 : 47.05
    #lv7 : 40.34