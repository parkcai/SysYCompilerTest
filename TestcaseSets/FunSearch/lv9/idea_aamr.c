/*
 * 测试数组操作、递归、逻辑运算和变量shadow
 * 功能：实现二分查找并统计查找次数
 */

// 递归二分查找
int binary_search(int arr[], int left, int right, int target, int count) {
    if (left > right) {
        return -1; // 未找到
    }
    
    int mid = left + (right - left) / 2;
    count = count + 1;
    
    if (arr[mid] == target) {
        return count;
    } else if (arr[mid] > target) {
        return binary_search(arr, left, mid - 1, target, count);
    } else {
        return binary_search(arr, mid + 1, right, target, count);
    }
}

// 冒泡排序
void sort_array(int arr[], int len) {
    int i = 0;
    while (i < len - 1) {
        int j = 0;
        while (j < len - i - 1) {
            if (arr[j] > arr[j + 1]) {
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
            j = j + 1;
        }
        i = i + 1;
    }
}

int main() {
    int arr[10];
    int count = getarray(arr);
    
    // 处理输入不足的情况
    if (count < 10) {
        putch(69); // 'E'
        putch(114); // 'r'
        putch(114); // 'r'
        putch(111); // 'o'
        putch(114); // 'r'
        return -1;
    }
    
    // 排序数组
    sort_array(arr, 10);
    
    // 测试shadow变量
    {
        int count = 999;
        putint(count); // 应输出999
        putch(10);
    }
    
    // 查找目标值
    int target = getint();
    int search_count = binary_search(arr, 0, 9, target, 0);
    
    if (search_count == -1) {
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
        putch(67); // 'C'
        putch(111); // 'o'
        putch(117); // 'u'
        putch(110); // 'n'
        putch(116); // 't'
        putch(58); // ':'
        putch(32); // ' '
        putint(search_count);
    }
    
    return 0;
}