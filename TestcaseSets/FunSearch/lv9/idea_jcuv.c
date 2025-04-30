/*
 * 测试用例：汉诺塔问题
 * 功能：使用递归解决汉诺塔问题，测试递归调用、数组操作和库函数使用
 */

// 打印移动步骤
void print_move(int disk, int from, int to) {
    putch(68); // 'D'
    putch(105); // 'i'
    putch(115); // 's'
    putch(107); // 'k'
    putch(32); // 空格
    putint(disk);
    putch(32); // 空格
    putch(109); // 'm'
    putch(111); // 'o'
    putch(118); // 'v'
    putch(101); // 'e'
    putch(100); // 'd'
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

// 汉诺塔递归实现
void hanoi(int n, int from, int to, int aux) {
    if (n == 1) {
        print_move(1, from, to);
        return;
    }
    hanoi(n - 1, from, aux, to);
    print_move(n, from, to);
    hanoi(n - 1, aux, to, from);
}

// 测试数组初始化和输出
void test_array() {
    int arr[5] = {0};
    int i = 0;
    
    // 初始化数组
    while (i < 5) {
        arr[i] = getint();
        i = i + 1;
    }
    
    // 输出数组
    putarray(5, arr);
}

int main() {
    int n = getint();
    
    // 测试汉诺塔
    hanoi(n, 1, 3, 2);
    
    // 测试变量shadow
    {
        int n = 10;
        putint(n);
        putch(10);
    }
    
    // 测试数组操作
    test_array();
    
    return 0;
}