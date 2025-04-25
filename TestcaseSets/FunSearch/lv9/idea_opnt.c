/*
 * 测试数组操作、函数递归和逻辑运算
 * 功能：模拟栈操作并测试各种边界情况
 */

// 全局栈定义
int stack[100];
int top = 0;

// 压栈操作
void push(int val) {
    stack[top] = val;
    top = top + 1;
}

// 弹栈操作
int pop() {
    top = top - 1;
    return stack[top];
}

// 判断栈是否为空
int is_empty() {
    return top == 0;
}

// 递归反转栈
void reverse_stack() {
    if (is_empty()) {
        return;
    }
    int temp = pop();
    reverse_stack();
    push(temp);
}

int main() {
    // 测试数组边界
    int arr[5] = {1, 2, 3, 4, 5};
    putarray(5, arr);
    putch(10); // 换行

    // 测试栈操作
    int n = getint();
    while (n > 0) {
        push(n % 10);
        n = n / 10;
    }

    // 测试shadow
    {
        int top = 999;
        putint(top); // 应输出999
        putch(10);
    }

    // 测试短路求值
    if (!is_empty() || 1) {
        reverse_stack();
    }

    // 输出反转后的栈
    while (!is_empty()) {
        putint(pop());
        putch(32); // 空格
    }

    return 0;
}