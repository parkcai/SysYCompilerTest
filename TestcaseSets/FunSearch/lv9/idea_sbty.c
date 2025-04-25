/*
 * 测试数组操作、递归函数、逻辑运算短路和变量shadow
 * 功能：实现二分查找并验证结果
 */

// 递归二分查找
int binary_search(int arr[], int left, int right, int target) {
    if (left > right) {
        return -1; // 未找到
    }
    int mid = left + (right - left) / 2;
    if (arr[mid] == target) {
        return mid;
    } else if (arr[mid] > target && left <= mid - 1) { // 测试短路求值
        return binary_search(arr, left, mid - 1, target);
    } else {
        return binary_search(arr, mid + 1, right, target);
    }
}

// 打印数组
void print_array(int arr[], int size) {
    int i = 0;
    while (i < size) {
        putint(arr[i]);
        if (i < size - 1) {
            putch(32); // 空格
        }
        i = i + 1;
    }
    putch(10); // 换行
}

int main() {
    // 初始化有序数组
    int arr[10] = {1, 3, 5, 7, 9, 11, 13, 15, 17, 19};
    
    // 测试shadow变量
    {
        int arr = 12345;
        putint(arr); // 应输出12345
        putch(10);
    }
    
    // 查找目标值
    int target = getint();
    int result = binary_search(arr, 0, 9, target);
    
    // 输出结果
    print_array(arr, 10);
    if (result == -1) {
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
        putch(97); // 'a'
        putch(116); // 't'
        putch(32); // ' '
        putint(result);
    }
    putch(10);
    
    return 0;
}