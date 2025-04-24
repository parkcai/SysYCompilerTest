/*
 * 测试数组操作、逻辑运算短路特性和函数递归
 * 功能：计算输入数组的逆序数（即满足i<j且a[i]>a[j]的数对数量）
 */

// 递归计算逆序数
int count_inversions(int arr[], int start, int end) {
    if (start >= end) return 0;
    
    int mid = (start + end) / 2;
    int left = count_inversions(arr, start, mid);
    int right = count_inversions(arr, mid + 1, end);
    
    // 合并并计算跨区间的逆序数
    int count = 0;
    int i = start;
    int j = mid + 1;
    int temp[100];
    int k = 0;
    
    while (i <= mid && j <= end) {
        // 利用短路特性防止数组越界
        if (j > end || (i <= mid && arr[i] <= arr[j])) {
            temp[k] = arr[i];
            i = i + 1;
        } else {
            temp[k] = arr[j];
            count = count + (mid - i + 1);
            j = j + 1;
        }
        k = k + 1;
    }
    
    // 复制剩余元素
    while (i <= mid) {
        temp[k] = arr[i];
        i = i + 1;
        k = k + 1;
    }
    
    // 测试shadow变量
    {
        int k = start;
        while (k <= end) {
            arr[k] = temp[k - start];
            k = k + 1;
        }
    }
    
    return left + right + count;
}

int main() {
    int data[10];
    int size = getarray(data);
    
    // 输出原始数组
    putarray(size, data);
    
    // 计算并输出逆序数
    int inversions = count_inversions(data, 0, size - 1);
    putint(inversions);
    
    return 0;
}