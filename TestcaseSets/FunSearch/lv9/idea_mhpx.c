/*
 * 测试多维数组初始化、函数递归调用、逻辑运算短路特性
 * 功能：计算斐波那契数列并验证数组操作
 */

const int FIB_INIT[3] = {0, 1, 1}; // 全局常量数组

// 递归计算斐波那契数列
int fibonacci(int n) {
    if (n == 0) return FIB_INIT[0];
    if (n == 1 || n == 2) return FIB_INIT[n];
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// 验证数组元素是否为斐波那契数
int check_fib(int arr[], int len) {
    int cnt = 0;
    int i = 0;
    while (i < len) {
        // 短路测试：当arr[i]<0时跳过判断
        if (arr[i] >= 0 && (arr[i] == fibonacci(arr[i]))) {
            cnt = cnt + 1;
        } else {
            putint(arr[i]); // 输出非斐波那契数
            putch(32); // 空格
        }
        i = i + 1;
    }
    return cnt;
}

int main() {
    int matrix[2][3] = {{1, 4, 6}, {8, 9, 10}}; // 多维数组初始化
    int input[5];
    int size = getarray(input); // 获取输入数组
    
    // shadow测试
    {
        int FIB_INIT = 5; // shadow全局常量
        putint(FIB_INIT); // 应输出5
        putch(10); // 换行
    }
    
    // 多维数组访问测试
    putint(matrix[1][2]); // 应输出10
    putch(10); // 换行
    
    int result = check_fib(input, size);
    putint(result); // 输出斐波那契数个数
    
    return 0;
}