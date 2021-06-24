#ifndef SRC_UTILS_LOGGER_H_
#define SRC_UTILS_LOGGER_H_

#include <iostream>
#include <string>

class Logger {
 public:
  static bool Init();

  static void Trace(
      const std::string& context, const std::string& message, bool endl = true);
  static void Debug(
      const std::string& context, const std::string& message, bool endl = true);
  static void Info(
      const std::string& context, const std::string& message, bool endl = true);
  static void Warn(
      const std::string& context, const std::string& message, bool endl = true);
  static void Error(
      const std::string& context, const std::string& message, bool endl = true);
  static void Fatal(
      const std::string& context, const std::string& message, bool endl = true);

 protected:
  static void Log(
      std::ostream& output,
      const std::string& context,
      const std::string& message,
      bool endl = true);
};

#endif  // SRC_UTILS_LOGGER_H_
