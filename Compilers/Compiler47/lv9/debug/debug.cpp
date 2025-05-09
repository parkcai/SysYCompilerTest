#include "debug.h"
std::ofstream debug;


// 获取当前时间字符串，精确到微秒
std::string getCurrentTimeString() {
    // 获取当前时间点
    auto now = std::chrono::system_clock::now();
    // 转换为时间点
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    
    // 获取微秒部分
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000000;
    
    // 转换为本地时间
    std::tm* localTime = std::localtime(&in_time_t);
    
    // 格式化时间字符串
    std::ostringstream oss;
    oss << std::put_time(localTime, "%Y-%m-%d_%H-%M-%S");
    oss << "_" << std::setfill('0') << std::setw(6) << micros.count();
    
    return oss.str();
}

void init_debug() {
    // 获取当前时间字符串
    std::string currentTime = getCurrentTimeString();
    // 构造文件名
    std::string fileName = "./debug/" + currentTime + ".txt";
    
    // 打开文件
    std::ofstream outFile(fileName);
    debug = std::move(outFile);
}
void logg(string x)
{
    std::ofstream outFile("log.txt",ios_base:: app);
    outFile <<x<<endl;;
}