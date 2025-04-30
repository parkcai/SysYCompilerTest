/*
 * 本测试用例旨在验证SysY语言中的数组操作、函数定义与调用、逻辑运算短路特性以及变量作用域shadow行为。
 * 功能：实现一个简单的整数求和程序，并通过用户输入的整数数组进行求和，最后输出求和结果。
 * 同时，该程序还包括了对一些逻辑运算短路特性的简单应用。
 */

// 求和函数
int sum_array(int arr[], int n) {
    int i = 0;
    int sum = 0;
    while (i < n) {
        sum = sum + arr[i];
        i = i + 1;
    }
    return sum;
}

// 打印整数
void print_int(int num) {
    putint(num);
    putch(10); // 输出换行符
}

int main() {
    int input[10];
    int size = getarray(input); // 获取用户输入的整数数组
    
    // 测试变量shadow
    {
        int size = 5; // 影子变量，不影响外部size
        print_int(size);  // 输出影子变量值
    }
    
    // 求和数组
    int total_sum = sum_array(input, size);
    
    // 利用逻辑运算的短路特性避免不必要的计算或访问未初始化数据
    if (size > 0 && size <= 10) {
        print_int(total_sum);
    } else {
        putch(33);  // 如果输入无效，则输出'!'
    }

    return 0;
}