/*
 * 测试数组操作、函数递归、短路求值和变量shadow
 * 功能：实现汉诺塔问题的求解并输出移动步骤
 */

// 汉诺塔递归求解
void hanoi(int n, int from, int to, int aux) {
    if (n == 1) {
        putint(from);
        putch(45); // '-'
        putch(62); // '>'
        putint(to);
        putch(10); // 换行
        return;
    }
    
    // 测试短路求值
    if (n > 0 && n < 10) {
        hanoi(n - 1, from, aux, to);
        putint(from);
        putch(45); // '-'
        putch(62); // '>'
        putint(to);
        putch(10); // 换行
        hanoi(n - 1, aux, to, from);
    }
}

// 测试数组初始化
void init_array(int arr[], int size) {
    int i = 0;
    while (i < size) {
        arr[i] = getint();
        i = i + 1;
    }
}

int main() {
    // 获取盘子数量
    int n = getint();
    
    // 测试shadow变量
    {
        int n = 3; // shadow外部n
        putch(83); // 'S'
        putch(104); // 'h'
        putch(97); // 'a'
        putch(100); // 'd'
        putch(111); // 'o'
        putch(119); // 'w'
        putch(58); // ':'
        putch(32); // 空格
        putint(n);
        putch(10); // 换行
    }
    
    // 测试数组初始化
    int test_arr[3];
    init_array(test_arr, 3);
    putarray(3, test_arr);
    
    // 解决汉诺塔问题
    hanoi(n, 1, 3, 2);
    
    return 0;
}