/* 计算斐波那契数列并验证数组操作 */
const int FIB_SEED = 10;  // 初始项数

// 递归计算斐波那契数
int fib(int n) {
    if (n <= 2) {
        return 1; /* 基础情况 */
    }
    return fib(n-1) + fib(n-2); // 递归调用
}

int main() {
    int arr[6] = {0, 1};  // 初始化部分元素
    int i = 2;
    
    /* 填充数组 */
    while (i < 6) {
        arr[i] = arr[i-1] + arr[i-2];
        if (i % 2 == 0) {
            putint(arr[i]);  // 输出偶数项
            putch(10);        // 换行符
        }
        i = i + 1;
    }
    
    // 验证递归函数
    int x = fib(5);
    putint(x);
    return 0;
}