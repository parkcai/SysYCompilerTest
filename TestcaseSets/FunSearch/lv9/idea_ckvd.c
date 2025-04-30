/*
 * 测试数组操作、函数递归、逻辑运算短路和变量shadow
 * 功能：实现汉诺塔问题求解并输出移动步骤
 */

// 汉诺塔移动步骤记录函数
void hanoi(int n, int from, int to, int via) {
    if (n == 1) {
        putint(from);
        putch(45); // '-'
        putch(62); // '>'
        putint(to);
        putch(10); // 换行
        return;
    }
    hanoi(n - 1, from, via, to);
    hanoi(1, from, to, via);
    hanoi(n - 1, via, to, from);
}

// 测试数组初始化和逻辑短路
int check_array(int arr[], int size) {
    int i = 0;
    while (i < size) {
        if (arr[i] != i + 1 && (i == 0 || arr[i-1] != i)) {
            return 0;
        }
        i = i + 1;
    }
    return 1;
}

int main() {
    // 获取输入层数
    int n = getint();
    
    // 测试shadow变量
    {
        int n = 3; // shadow变量
        putch(72); // 'H'
        putch(97); // 'a'
        putch(110); // 'n'
        putch(111); // 'o'
        putch(105); // 'i'
        putch(32); // 空格
        putint(n);
        putch(58); // ':'
        putch(10); // 换行
        hanoi(n, 1, 3, 2); // 输出3层汉诺塔解法
    }
    
    // 测试数组初始化和逻辑短路
    int arr[5] = {1, 2, 3, 4, 5};
    if (n > 0 && check_array(arr, 5)) {
        putch(86); // 'V'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
        putch(10); // 换行
    } else {
        putch(73); // 'I'
        putch(110); // 'n'
        putch(118); // 'v'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
        putch(10); // 换行
    }
    
    // 输出实际输入层数的解法
    putch(10); // 换行
    putch(72); // 'H'
    putch(97); // 'a'
    putch(110); // 'n'
    putch(111); // 'o'
    putch(105); // 'i'
    putch(32); // 空格
    putint(n);
    putch(58); // ':'
    putch(10); // 换行
    hanoi(n, 1, 3, 2);
    
    return 0;
}