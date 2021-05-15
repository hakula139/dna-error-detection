#include "logger.h"

#include <cstdio>
#include <cstdlib>
#include <string>

#include "config.h"

using std::string;

extern Config config;

void Logger::Debug(const string& context, const string& message) {
  if (config.log_level <= Config::Level::DEBUG) {
    printf("[DEBUG] ");
    Log(context, message);
  }
}

void Logger::Info(const string& context, const string& message) {
  if (config.log_level <= Config::Level::INFO) {
    printf("[INFO ] ");
    Log(context, message);
  }
}

void Logger::Warn(const string& context, const string& message) {
  if (config.log_level <= Config::Level::WARN) {
    printf("[WARN ] ");
    Log(context, message);
  }
}

void Logger::Error(const string& context, const string& message) {
  if (config.log_level <= Config::Level::ERROR) {
    printf("[ERROR] ");
    Log(context, message);
  }
}

void Logger::Fatal(const string& context, const string& message) {
  if (config.log_level <= Config::Level::FATAL) {
    printf("[FATAL] ");
    Log(context, message);
    exit(EXIT_FAILURE);
  }
}

void Logger::Log(const string& context, const string& message) {
  printf("%s: %s\n", context.c_str(), message.c_str());
}

Logger logger;
