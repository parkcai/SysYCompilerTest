/*
 * 测试多维数组、逻辑运算短路、变量shadow以及库函数的使用
 * 功能：实现一个简单的学生成绩管理系统，包括成绩输入、计算平均分、输出成绩表等功能
 */

// 计算平均分
int calculate_average(int scores[], int count) {
    int sum = 0;
    int i = 0;
    while (i < count) {
        sum = sum + scores[i];
        i = i + 1;
    }
    return sum / count;
}

// 打印成绩表
void print_scores(int scores[], int count) {
    int i = 0;
    while (i < count) {
        putint(scores[i]);
        if (i < count - 1) {
            putch(32); // 空格
        }
        i = i + 1;
    }
    putch(10); // 换行
}

int main() {
    // 定义一个数组存储学生成绩
    int scores[5];

    // 输入学生成绩
    int i = 0;
    while (i < 5) {
        scores[i] = getint();
        i = i + 1;
    }

    // 计算平均分
    int average = calculate_average(scores, 5);

    // 测试变量shadow
    {
        int average = 100;
        putint(average); // 应输出100
        putch(10);
    }

    // 输出成绩表
    print_scores(scores, 5);

    // 输出平均分
    putint(average);
    putch(10);

    // 测试逻辑运算短路
    if (scores[0] > 0 && (100 / scores[0] > 0)) {
        putch(65); // 输出'A'表示通过
    } else {
        putch(70); // 输出'F'表示不及格
    }

    return 0;
}