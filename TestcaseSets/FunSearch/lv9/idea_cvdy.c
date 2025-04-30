/*
 * 本测试用例旨在验证SysY语言中的数组操作、函数定义与调用、以及作用域shadow行为。
 * 功能：实现一个简单的斐波那契数列生成器，并通过用户输入的数字决定生成多少个斐波那契数，
 *       最后输出这些数到标准输出。同时，该程序还包括了对一些逻辑运算短路特性的简单应用。
 */

// 斐波那契数列生成器
void fibonacci(int n, int result[]) {
    if (n <= 0) return; // 当n为非正整数时，直接返回
    if (n == 1) {
        result[0] = 0;
        return;
    }
    
    result[0] = 0;
    result[1] = 1;
    int i = 2;
    while (i < n) {
        result[i] = result[i - 1] + result[i - 2];
        i = i + 1;
    }
}

// 打印整数数组
void print_ints(int arr[], int count) {
    int i = 0;
    while (i < count) {
        putint(arr[i]);
        if (i != count - 1) {
            putch(32); // 输出空格
        }
        i = i + 1;
    }
    putch(10); // 输出换行符
}

int main() {
    int input = getint(); // 获取用户指定的斐波那契数列长度
    
    // 测试变量shadow
    {
        int input = 10; // 影子变量，不影响外部input
        putint(input);  // 输出影子变量值
        putch(10);
    }
    
    int fibo[100];  // 用于存储斐波那契数列的结果
    fibonacci(input, fibo);  // 生成斐波那契数列
    
    // 利用逻辑运算的短路特性避免不必要的计算或访问未初始化数据
    if (input > 0 && input < 100) {
        print_ints(fibo, input);
    } else {
        putch(33);  // 如果输入无效，则输出'!'
    }

    return 0;
}