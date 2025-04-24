/*
 * 测试数组操作、函数递归和短路求值
 * 功能：实现并测试二分查找算法
 */

// 递归实现二分查找
int binary_search(int arr[], int left, int right, int target) {
    if (left > right) {
        return -1;
    }
    
    int mid = left + (right - left) / 2;
    
    // 测试短路求值
    if (arr[mid] == target || (left == right && arr[left] == target)) {
        return mid;
    }
    
    if (arr[mid] > target) {
        return binary_search(arr, left, mid - 1, target);
    } else {
        return binary_search(arr, mid + 1, right, target);
    }
}

// 打印查找结果
void print_result(int index) {
    if (index != -1) {
        putint(index);
    } else {
        putch(78); // 'N'
        putch(111); // 'o'
    }
    putch(10);
}

int main() {
    // 测试数组输入
    int arr[10];
    int n = getarray(arr);
    
    // 测试排序
    int i = 0;
    while (i < n) {
        int j = i + 1;
        while (j < n) {
            if (arr[i] > arr[j]) {
                int temp = arr[i];
                arr[i] = arr[j];
                arr[j] = temp;
            }
            j = j + 1;
        }
        i = i + 1;
    }
    
    // 测试shadow
    {
        int n = 3;
        putint(n); // 应输出3
        putch(10);
    }
    
    // 查找目标值
    int target = getint();
    int result = binary_search(arr, 0, n - 1, target);
    print_result(result);
    
    return 0;
}