/*
 * 测试数组操作、函数递归和逻辑运算
 * 功能：实现二分查找并验证查找结果
 */

// 二分查找函数
int binary_search(int arr[], int size, int target) {
    int left = 0;
    int right = size - 1;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (arr[mid] == target) {
            return mid;
        } else if (arr[mid] < target) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    return -1;
}

// 打印查找结果
void print_result(int index, int target) {
    if (index != -1) {
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
        putch(10); // 换行
    } else {
        putch(78); // 'N'
        putch(111); // 'o'
        putch(116); // 't'
        putch(32); // ' '
        putch(102); // 'f'
        putch(111); // 'o'
        putch(117); // 'u'
        putch(110); // 'n'
        putch(100); // 'd'
        putch(10); // 换行
    }
}

int main() {
    int arr[10];
    int size = getarray(arr);
    int target = getint();
    
    // 测试短路特性
    if (size > 0 && size <= 10) {
        int index = binary_search(arr, size, target);
        print_result(index, target);
    } else {
        putch(33); // 输出'!'表示无效输入
    }
    
    // 测试shadow变量
    {
        int size = 5;
        int arr[5] = {1, 3, 5, 7, 9};
        int target = 5;
        int index = binary_search(arr, size, target);
        print_result(index, target); // 应输出Found 5 at 2
    }
    
    return 0;
}