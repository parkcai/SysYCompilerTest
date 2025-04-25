/*
 * 测试数组操作、函数递归和短路逻辑运算
 * 功能：实现二分查找并验证结果
 */

// 递归二分查找
int binary_search(int arr[], int left, int right, int target) {
    if (left > right) {
        return -1;
    }
    
    int mid = left + (right - left) / 2;
    
    // 测试短路求值
    if (arr[mid] == target || left == right) {
        return mid;
    }
    
    if (arr[mid] < target) {
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
        putch(70); // 'F'
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
    const int SIZE = 10;
    int arr[10] = {1, 3, 5, 7, 9, 11, 13, 15, 17, 19};
    
    // 测试shadow
    {
        int SIZE = 5;
        int arr[5] = {2, 4, 6, 8, 10};
        int target = getint();
        int result = binary_search(arr, 0, SIZE - 1, target);
        print_result(result, target);
    }
    
    // 测试主数组
    int target = getint();
    int result = binary_search(arr, 0, SIZE - 1, target);
    print_result(result, target);
    
    return 0;
}