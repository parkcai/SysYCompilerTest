/*
 * 测试数组操作、递归、短路求值、变量shadow和库函数使用
 * 功能：模拟简单计算器，支持加减乘除和阶乘运算
 */

// 计算阶乘
int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

// 打印菜单
void print_menu() {
    putch(67); // 'C'
    putch(97); // 'a'
    putch(108); // 'l'
    putch(99); // 'c'
    putch(117); // 'u'
    putch(108); // 'l'
    putch(97); // 'a'
    putch(116); // 't'
    putch(111); // 'o'
    putch(114); // 'r'
    putch(10); // 换行
    putch(49); // '1'
    putch(58); // ':'
    putch(32); // ' '
    putch(43); // '+'
    putch(10); // 换行
    putch(50); // '2'
    putch(58); // ':'
    putch(32); // ' '
    putch(45); // '-'
    putch(10); // 换行
    putch(51); // '3'
    putch(58); // ':'
    putch(32); // ' '
    putch(42); // '*'
    putch(10); // 换行
    putch(52); // '4'
    putch(58); // ':'
    putch(32); // ' '
    putch(47); // '/'
    putch(10); // 换行
    putch(53); // '5'
    putch(58); // ':'
    putch(32); // ' '
    putch(33); // '!'
    putch(10); // 换行
}

int main() {
    int op, a, b;
    print_menu();
    
    // 测试shadow变量
    {
        int op = getint();
        putint(op); // 输出第一个输入
        putch(10);
    }
    
    op = getint();
    // 测试短路求值
    if (op < 1 || op > 5) {
        putch(69); // 'E'
        putch(114); // 'r'
        putch(114); // 'r'
        putch(111); // 'o'
        putch(114); // 'r'
        return 0;
    }
    
    // 根据操作符执行不同运算
    if (op == 5) {
        a = getint();
        putint(factorial(a));
    } else {
        a = getint();
        b = getint();
        if (op == 1) putint(a + b);
        else if (op == 2) putint(a - b);
        else if (op == 3) putint(a * b);
        else if (op == 4) {
            // 测试除零错误
            if (b == 0) {
                putch(68); // 'D'
                putch(105); // 'i'
                putch(118); // 'v'
                putch(48); // '0'
            } else {
                putint(a / b);
            }
        }
    }
    
    return 0;
}