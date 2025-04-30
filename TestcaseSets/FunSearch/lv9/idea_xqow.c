/*
 * 测试一维数组操作、逻辑运算短路和变量shadow
 * 功能：实现一个简单的数组排序，并验证结果
 */

// 交换数组元素
void swap(int arr[], int i, int j) {
    int temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
}

// 冒泡排序
void bubble_sort(int arr[], int size) {
    int i = 0;
    while (i < size - 1) {
        int j = 0;
        while (j < size - 1 - i) {
            if (arr[j] > arr[j + 1]) {
                swap(arr, j, j + 1);
            }
            j = j + 1;
        }
        i = i + 1;
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
    // 定义一个一维数组
    int array[10] = {5, 3, 8, 1, 2, 4, 7, 6, 9, 0};

    // 测试shadow变量
    {
        int array = 42; // shadow外层的array
        putint(array);  // 应输出42
        putch(10);      // 换行
    }

    // 排序前打印
    putch(79); // 'O'
    putch(114); // 'r'
    putch(105); // 'i'
    putch(103); // 'g'
    putch(105); // 'i'
    putch(110); // 'n'
    putch(97); // 'a'
    putch(108); // 'l'
    putch(58); // ':'
    putch(10); // 换行
    print_array(array, 10);

    // 对数组进行排序
    bubble_sort(array, 10);

    // 排序后打印
    putch(83); // 'S'
    putch(111); // 'o'
    putch(114); // 'r'
    putch(116); // 't'
    putch(101); // 'e'
    putch(100); // 'd'
    putch(58); // ':'
    putch(10); // 换行
    print_array(array, 10);

    // 测试短路逻辑
    if (array[0] == 0 && array[9] == 9 || array[5] != 5) {
        putch(33); // '!'
        putch(10); // 换行
    }

    return 0;
}