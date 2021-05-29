#ifndef SRC_UTILS_LOGGER_H_
#define SRC_UTILS_LOGGER_H_

#include <string>

class Logger {
 public:
  static void Trace(const std::string& context, const std::string& message);
  static void Debug(const std::string& context, const std::string& message);
  static void Info(const std::string& context, const std::string& message);
  static void Warn(const std::string& context, const std::string& message);
  static void Error(const std::string& context, const std::string& message);
  static void Fatal(const std::string& context, const std::string& message);

 protected:
  static void Log(const std::string& context, const std::string& message);
};

#endif  // SRC_UTILS_LOGGER_H_
