#include "logger.h"

#include <iostream>
#include <string>

#include "config.h"

using std::cout;
using std::flush;
using std::string;

void Logger::Trace(const string& context, const string& message, bool endl) {
  if (Config::LOG_LEVEL <= Config::Level::TRACE) {
    cout << "[TRACE] ";
    Log(context, message, endl);
  }
}

void Logger::Debug(const string& context, const string& message, bool endl) {
  if (Config::LOG_LEVEL <= Config::Level::DEBUG) {
    cout << "[DEBUG] ";
    Log(context, message, endl);
  }
}

void Logger::Info(const string& context, const string& message, bool endl) {
  if (Config::LOG_LEVEL <= Config::Level::INFO) {
    cout << "[INFO ] ";
    Log(context, message, endl);
  }
}

void Logger::Warn(const string& context, const string& message, bool endl) {
  if (Config::LOG_LEVEL <= Config::Level::WARN) {
    cout << "[WARN ] ";
    Log(context, message, endl);
  }
}

void Logger::Error(const string& context, const string& message, bool endl) {
  if (Config::LOG_LEVEL <= Config::Level::ERROR) {
    cout << "[ERROR] ";
    Log(context, message, endl);
  }
}

void Logger::Fatal(const string& context, const string& message, bool endl) {
  if (Config::LOG_LEVEL <= Config::Level::FATAL) {
    cout << "[FATAL] ";
    Log(context, message, endl);
  }
}

void Logger::Log(const string& context, const string& message, bool endl) {
  cout << context << ": " << message + (endl ? "\n" : "") << flush;
}
