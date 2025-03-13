#pragma once

#include <Arduino.h>
#include "esp_log.h"

// 日志级别定义
typedef enum {
    LOG_LEVEL_NONE = 0,   // 不输出任何日志
    LOG_LEVEL_ERROR,      // 只输出错误日志
    LOG_LEVEL_WARN,       // 输出警告和错误日志
    LOG_LEVEL_INFO,       // 输出信息、警告和错误日志
    LOG_LEVEL_DEBUG,      // 输出调试、信息、警告和错误日志
    LOG_LEVEL_VERBOSE     // 输出所有日志
} LogLevel;

class Logger {
public:
    // 初始化日志系统
    static void init(LogLevel level = LOG_LEVEL_INFO) {
        logLevel = level;
        // 配置ESP日志库
        esp_log_level_set("*", ESP_LOG_INFO);  // 默认所有标签为INFO级别
    }

    // 设置日志级别
    static void setLogLevel(LogLevel level) {
        logLevel = level;
    }

    // 获取当前日志级别
    static LogLevel getLogLevel() {
        return logLevel;
    }

    // 错误日志
    static void error(const char* tag, const char* format, ...) {
        if (logLevel >= LOG_LEVEL_ERROR) {
            va_list args;
            va_start(args, format);
            log(tag, "E", format, args);
            va_end(args);
        }
    }

    // 警告日志
    static void warn(const char* tag, const char* format, ...) {
        if (logLevel >= LOG_LEVEL_WARN) {
            va_list args;
            va_start(args, format);
            log(tag, "W", format, args);
            va_end(args);
        }
    }

    // 信息日志
    static void info(const char* tag, const char* format, ...) {
        if (logLevel >= LOG_LEVEL_INFO) {
            va_list args;
            va_start(args, format);
            log(tag, "I", format, args);
            va_end(args);
        }
    }

    // 调试日志
    static void debug(const char* tag, const char* format, ...) {
        if (logLevel >= LOG_LEVEL_DEBUG) {
            va_list args;
            va_start(args, format);
            log(tag, "D", format, args);
            va_end(args);
        }
    }

    // 详细日志
    static void verbose(const char* tag, const char* format, ...) {
        if (logLevel >= LOG_LEVEL_VERBOSE) {
            va_list args;
            va_start(args, format);
            log(tag, "V", format, args);
            va_end(args);
        }
    }

private:
    static LogLevel logLevel;

    // 内部日志输出函数
    static void log(const char* tag, const char* level, const char* format, va_list args) {
        // 计算所需缓冲区大小
        char temp[2];  // 用于测试大小的临时缓冲区
        int len = vsnprintf(temp, sizeof(temp), format, args);
        if (len < 0) return;  // 格式化错误
        
        // 创建足够大的缓冲区
        size_t size = len + 1;
        char* buffer = (char*)malloc(size);
        if (!buffer) return;  // 内存分配失败
        
        // 格式化消息
        vsnprintf(buffer, size, format, args);
        
        // 输出到串口
        Serial.printf("[%s][%s] %s\n", level, tag, buffer);
        
        free(buffer);
    }
};

// 定义静态成员变量
LogLevel Logger::logLevel = LOG_LEVEL_INFO; 