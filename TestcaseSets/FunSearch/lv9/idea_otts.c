/*
 * 测试数组操作、逻辑运算短路特性和函数递归
 * 功能：查找数组中的第一个非零元素并计算其阶乘
 */

// 递归计算阶乘
int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

// 查找第一个非零元素
int find_first_nonzero(int arr[], int len, int index) {
    if (index >= len) return -1;  // 未找到
    // 利用短路特性避免除零错误
    if (arr[index] != 0 && (100 / arr[index] > 0)) {
        return arr[index];
    }
    return find_first_nonzero(arr, len, index + 1);
}

int main() {
    int data[5];
    int size = getarray(data);
    
    // 测试shadow
    {
        int size = 3;
        int shadow_arr[3] = {0, 1, 0};
        putint(find_first_nonzero(shadow_arr, size, 0)); // 应输出1
        putch(10); // 换行
    }
    
    int target = find_first_nonzero(data, size, 0);
    if (target != -1) {
        putint(factorial(target)); // 输出第一个非零元素的阶乘
    } else {
        putint(0); // 未找到非零元素
    }
    
    return 0;
}