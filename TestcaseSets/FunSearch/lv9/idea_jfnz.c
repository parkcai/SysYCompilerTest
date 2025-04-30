/*
 * 测试多维数组初始化、函数递归调用、逻辑运算短路特性
 * 功能：实现一个简单的计算器，支持加法、减法、乘法和除法
 */

// 定义常量数组用于存储操作符
const int OPERATORS[4] = {1, 2, 3, 4}; // 1: 加, 2: 减, 3: 乘, 4: 除

// 计算表达式的值
int calculate(int a, int b, int op) {
    if (op == OPERATORS[0]) return a + b; // 加法
    if (op == OPERATORS[1]) return a - b; // 减法
    if (op == OPERATORS[2]) return a * b; // 乘法
    if (op == OPERATORS[3]) {
        if (b == 0) return 0; // 除数为0时返回0
        return a / b; // 除法
    }
    return 0; // 操作符无效
}

// 解析输入并计算结果
void parse_and_calculate() {
    int a, b, op;
    putch(71); // 输出'G'
    putch(10); // 换行
    putch(65); // 输出'A'
    putch(10); // 换行
    putch(83); // 输出'S'
    putch(10); // 换行
    putch(79); // 输出'O'
    putch(10); // 换行
    putch(73); // 输出'I'
    putch(10); // 换行
    putch(78); // 输出'N'
    putch(10); // 换行
    putch(84); // 输出'T'
    putch(10); // 换行
    putch(73); // 输出'I'
    putch(10); // 换行
    putch(78); // 输出'N'
    putch(10); // 换行
    putch(71); // 输出'G'
    putch(10); // 换行
    putch(65); // 输出'A'
    putch(10); // 换行
    putch(83); // 输出'S'
    putch(10); // 换行
    putch(79); // 输出'O'
    putch(10); // 换行
    putch(73); // 输出'I'
    putch(10); // 换行
    putch(78); // 输出'N'
    putch(10); // 换行
    putch(84); // 输出'T'
    putch(10); // 换行
    putch(73); // 输出'I'
    putch(10); // 换行
    putch(78); // 输出'N'
    putch(10); // 换行
    putch(80); // 输出'P'
    putch(10); // 换行
    putch(85); // 输出'U'
    putch(10); // 换行
    putch(84); // 输出'T'
    putch(10); // 换行
    putch(32); // 输出空格
    putch(97); // 输出'a'
    putch(32); // 输出空格
    putch(98); // 输出'b'
    putch(32); // 输出空格
    putch(111); // 输出'o'
    putch(112); // 输出'p'
    putch(10); // 换行
    a = getint(); // 获取第一个操作数
    b = getint(); // 获取第二个操作数
    op = getint(); // 获取操作符
    putint(calculate(a, b, op)); // 输出计算结果
    putch(10); // 换行
}

int main() {
    // 测试shadow变量
    {
        const int OPERATORS = 5; // shadow全局常量
        putint(OPERATORS); // 应输出5
        putch(10); // 换行
    }

    // 多维数组访问测试
    int matrix[2][3] = {{1, 2, 3}, {4, 5, 6}};
    putint(matrix[1][2]); // 应输出6
    putch(10); // 换行

    // 调用解析和计算函数
    parse_and_calculate();

    return 0;
}