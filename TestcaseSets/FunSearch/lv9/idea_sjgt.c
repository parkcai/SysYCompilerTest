/*
 * 测试数组操作、函数递归、短路求值和变量shadow
 * 功能：实现二分查找算法并验证结果
 */

// 递归实现二分查找
int binary_search(int arr[], int left, int right, int target) {
    if (left > right) {
        return -1; // 未找到
    }
    int mid = left + (right - left) / 2;
    if (arr[mid] == target) {
        return mid;
    } else if (arr[mid] > target) {
        return binary_search(arr, left, mid - 1, target);
    } else {
        return binary_search(arr, mid + 1, right, target);
    }
}

// 冒泡排序函数
void bubble_sort(int arr[], int size) {
    int i = 0;
    while (i < size - 1) {
        int j = 0;
        while (j < size - i - 1) {
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
    // 获取输入数组
    int input[10];
    int size = getarray(input);
    
    // 测试shadow
    {
        int size = 5;
        int test_arr[5] = {5,4,3,2,1};
        bubble_sort(test_arr, size);
        putarray(size, test_arr); // 应输出排序后的数组
    }
    
    // 排序输入数组
    bubble_sort(input, size);
    
    // 查找目标值
    int target = getint();
    
    // 测试短路求值
    if (size <= 0 || size > 10) {
        putint(-1);
        return 0;
    }
    
    int result = binary_search(input, 0, size - 1, target);
    
    // 输出结果
    putint(result);
    putch(10);
    putarray(size, input); // 输出排序后的数组
    
    return 0;
}