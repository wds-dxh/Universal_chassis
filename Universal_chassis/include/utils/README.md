# ESP32 日志系统

这个日志系统为ESP32项目提供了灵活的日志记录功能，支持不同级别的日志输出和运行时日志级别调整。

## 特性

- 支持多个日志级别（NONE, ERROR, WARN, INFO, DEBUG, VERBOSE）
- 支持标签分类，便于识别日志来源
- 格式化输出，支持变量插入
- 运行时可调整日志级别
- 编译时可通过宏定义控制默认日志级别

## 使用方法

### 初始化

在程序开始时初始化日志系统：

```cpp
// 默认使用INFO级别
Logger::init(LOG_LEVEL_INFO);

// 或者使用条件编译控制日志级别
#ifdef DEBUG_MODE
    Logger::init(LOG_LEVEL_DEBUG);
#else
    Logger::init(LOG_LEVEL_NONE);  // 生产环境禁用日志
#endif
```

### 记录日志

```cpp
// 使用不同级别记录日志
Logger::error("TAG", "Error message: %d", errorCode);
Logger::warn("TAG", "Warning message");
Logger::info("TAG", "Information: %s", infoString);
Logger::debug("TAG", "Debug value: %.2f", debugValue);
Logger::verbose("TAG", "Verbose data");
```

### 运行时调整日志级别

```cpp
// 设置为调试级别
Logger::setLogLevel(LOG_LEVEL_DEBUG);

// 完全禁用日志
Logger::setLogLevel(LOG_LEVEL_NONE);
```

## 日志级别说明

1. **LOG_LEVEL_NONE**: 不输出任何日志
2. **LOG_LEVEL_ERROR**: 只输出错误日志
3. **LOG_LEVEL_WARN**: 输出警告和错误日志
4. **LOG_LEVEL_INFO**: 输出信息、警告和错误日志
5. **LOG_LEVEL_DEBUG**: 输出调试、信息、警告和错误日志
6. **LOG_LEVEL_VERBOSE**: 输出所有级别的日志

## 最佳实践

- 在开发阶段使用 DEBUG 或 VERBOSE 级别
- 在测试阶段使用 INFO 级别
- 在生产环境使用 NONE 或 ERROR 级别
- 使用有意义的标签来区分不同模块的日志
- 避免在性能关键的循环中使用 VERBOSE 级别日志

## 注意事项

- 日志输出会占用一定的处理时间和内存资源
- 大量日志可能会影响实时性能
- 在生产环境中，建议默认禁用日志，仅在需要时通过命令启用 