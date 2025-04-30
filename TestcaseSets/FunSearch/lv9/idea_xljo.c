/*
 * 测试数组操作、函数递归和逻辑运算短路
 * 功能：实现二分查找并验证结果
 */

// 递归实现二分查找
int binary_search(int arr[], int left, int right, int target) {
    if (left > right) {
        return -1;
    }
    int mid = left + (right - left) / 2;
    if (arr[mid] == target) {
        return mid;
    } else if (arr[mid] < target) {
        return binary_search(arr, mid + 1, right, target);
    } else {
        return binary_search(arr, left, mid - 1, target);
    }
}

// 打印查找结果
void print_result(int index, int target) {
    if (index == -1) {
        putch(78); // 'N'
        putch(111); // 'o'
        putch(116); // 't'
        putch(32); // ' '
        putch(102); // 'f'
        putch(111); // 'o'
        putch(117); // 'u'
        putch(110); // 'n'
        putch(100); // 'd'
    } else {
        putch(70); // 'F'
        putch(111); // 'o'
        putch(117); // 'u'
        putch(110); // 'n'
        putch(100); // 'd'
        putch(32); // ' '
        putint(target);
        putch(32); // ' '
        putch(97); // 'a'
        putch(116); // 't'
        putch(32); // ' '
        putint(index);
    }
    putch(10); // 换行
}

int main() {
    // 获取排序数组
    int size = getint();
    int arr[10];
    int i = 0;
    while (i < size) {
        arr[i] = getint();
        i = i + 1;
    }

    // 测试短路特性
    if (size <= 0 || size > 10) {
        putch(73); // 'I'
        putch(110); // 'n'
        putch(118); // 'v'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
        putch(32); // ' '
        putch(115); // 's'
        putch(105); // 'i'
        putch(122); // 'z'
        putch(101); // 'e'
        return 0;
    }

    // 获取查找目标
    int target = getint();

    // 测试shadow变量
    {
        int target = 999;
        putint(target); // 应输出999
        putch(10); // 换行
    }

    // 执行二分查找
    int result = binary_search(arr, 0, size - 1, target);
    print_result(result, target);

    return 0;
}