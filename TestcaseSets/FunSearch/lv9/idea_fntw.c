/* 递归计算阶乘并验证数组传递 */
int factorial(int n) { // 递归函数示例
    if (n <= 1) return 1;
    else return n * factorial(n - 1);
}

const int MAX_SIZE = 10; // 编译期常量

int main() {
    int arr[3][2] = {{1, 2}, {3}, {5}}; // 多维数组部分初始化
    int num = getint();
    
    /* 变量shadow测试 */
    {
        int MAX_SIZE = 5; // 合法shadow
        putint(MAX_SIZE); // 应输出5
        putch(10); // 换行符
    }

    // 短路逻辑测试
    int a = 0;
    while (a < num) {
        if (a > 2 && (factorial(a) % 2 == 0)) {
            putint(arr[a][1]);
            putch(32); // 空格
        } else if (a == 0 || 1) {
            break;
        }
        a = a + 1;
    }
    
    putarray(3, arr[0]); // 数组输出测试
    return factorial(num);
}