/*
 * 测试多维数组、递归、短路求值和变量shadow
 * 功能：实现汉诺塔问题的求解并输出移动步骤
 */

// 输出移动步骤
void move_disk(int n, int from, int to) {
    putch(77); // 'M'
    putch(111); // 'o'
    putch(118); // 'v'
    putch(101); // 'e'
    putch(32); // 空格
    putint(n);
    putch(32); // 空格
    putch(102); // 'f'
    putch(114); // 'r'
    putch(111); // 'o'
    putch(109); // 'm'
    putch(32); // 空格
    putint(from);
    putch(32); // 空格
    putch(116); // 't'
    putch(111); // 'o'
    putch(32); // 空格
    putint(to);
    putch(10); // 换行
}

// 递归解决汉诺塔问题
void hanoi(int n, int from, int to, int aux) {
    if (n == 1) {
        move_disk(n, from, to);
        return;
    }
    // 测试短路求值
    if (n > 1 && from != to) {
        hanoi(n - 1, from, aux, to);
        move_disk(n, from, to);
        hanoi(n - 1, aux, to, from);
    }
}

// 测试多维数组和变量shadow
void test_array() {
    int arr[2][3] = {{1, 2, 3}, {4, 5, 6}};
    putarray(3, arr[0]); // 输出1 2 3
    putarray(3, arr[1]); // 输出4 5 6
    
    {
        int arr[3] = {7, 8, 9};
        putarray(3, arr); // 输出7 8 9
    }
}

int main() {
    int n = getint(); // 获取盘子数量
    
    // 测试输入合法性
    if (n <= 0 || n > 10) {
        putch(73); // 'I'
        putch(110); // 'n'
        putch(118); // 'v'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
        return 0;
    }
    
    // 测试多维数组
    test_array();
    
    // 解决汉诺塔问题
    hanoi(n, 1, 3, 2);
    
    return 0;
}