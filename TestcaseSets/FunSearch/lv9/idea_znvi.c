/*
 * 计算阶乘和并验证多维数组操作
 * 使用递归与循环结合实现
 */

// 全局常量初始化
const int BASE[3] = {1, 1, 2};  // 0!,1!,2!

// 递归计算阶乘
int factorial(int n) {
    if (n < 3) {
        return BASE[n];  // 访问全局常量数组
    }
    return n * factorial(n - 1);
}

// 辅助函数：计算数组元素和
int sum_array(int arr[], int len) {
    int total = 0;
    int i = 0;
    while (i < len) {
        // 变量shadow测试
        {
            int i = i + 5;  // 正确shadow（但实际未使用）
            putch(35);      // 输出#作为调试标记
        }
        total = total + arr[i];
        i = i + 1;
    }
    return total;
}

int main() {
    int n = getint();
    int matrix[2][3] = {{1,2,3},{4,5,6}};  // 多维数组测试
    
    // 计算结果数组
    int res[5];
    int i = 0;
    while (i < 5) {
        if (i <= n) {
            res[i] = factorial(i);
        } else {
            res[i] = -1;
        }
        i = i + 1;
    }
    
    // 嵌套作用域测试
    {
        int matrix = 42;  // shadow外层数组名
        putint(matrix);   // 应输出42
    }
    
    putarray(5, res);           // 输出计算结果
    putint(sum_array(res, 5));  // 输出总和
    return matrix[1][2];        // 返回多维数组元素6
}