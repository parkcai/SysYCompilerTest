/*
 * 测试多维数组初始化、函数递归调用、逻辑运算短路特性
 * 功能：计算阶乘并验证数组操作
 */

const int FACT_INIT[3] = {1, 1, 2}; // 全局常量数组

// 递归计算阶乘
int factorial(int n) {
    if (n == 0 || n == 1) return FACT_INIT[n];
    return n * factorial(n - 1);
}

// 验证数组元素是否为阶乘
int check_factorial(int arr[], int len) {
    int cnt = 0;
    int i = 0;
    while (i < len) {
        // 短路测试：当arr[i]<0时跳过判断
        if (arr[i] >= 0 && (arr[i] == factorial(arr[i]))) {
            cnt = cnt + 1;
        } else {
            putint(arr[i]); // 输出非阶乘数
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
        int FACT_INIT = 5; // shadow全局常量
        putint(FACT_INIT); // 应输出5
        putch(10); // 换行
    }
    
    // 多维数组访问测试
    putint(matrix[1][2]); // 应输出10
    putch(10); // 换行
    
    int result = check_factorial(input, size);
    putint(result); // 输出阶乘数个数
    
    return 0;
}