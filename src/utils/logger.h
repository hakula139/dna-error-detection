#ifndef SRC_UTILS_LOGGER_H_
#define SRC_UTILS_LOGGER_H_

#include <string>

class Logger {
 public:
  void Trace(const std::string& context, const std::string& message);
  void Debug(const std::string& context, const std::string& message);
  void Info(const std::string& context, const std::string& message);
  void Warn(const std::string& context, const std::string& message);
  void Error(const std::string& context, const std::string& message);
  void Fatal(const std::string& context, const std::string& message);

 protected:
  void Log(const std::string& context, const std::string& message);
};

#endif  // SRC_UTILS_LOGGER_H_
