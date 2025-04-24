/*
 * 测试多维数组初始化、逻辑运算短路特性、变量shadow和递归函数
 * 功能：计算输入数组的加权和，权重为斐波那契数列
 */

const int FIB[5] = {1, 1, 2, 3, 5}; // 斐波那契权重数组

// 递归计算数组加权和
int weighted_sum(int arr[], int len, int index) {
    if (index >= len) return 0;
    // 利用短路特性避免数组越界
    if (index < 0 || index >= 5 || arr[index] % 2 == 0) {
        return weighted_sum(arr, len, index + 1);
    }
    return arr[index] * FIB[index] + weighted_sum(arr, len, index + 1);
}

int main() {
    int data[5];
    int size = getarray(data);
    
    // shadow测试
    {
        int FIB[3] = {10, 20, 30}; // shadow全局FIB数组
        putarray(3, FIB); // 应输出10 20 30
    }
    
    // 测试多维数组初始化
    int matrix[2][2] = {{1, weighted_sum(data, size, 0)}, {3, 4}};
    putint(matrix[0][1]); // 输出加权和结果
    
    return 0;
}