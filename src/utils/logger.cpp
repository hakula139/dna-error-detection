#include "logger.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "config.h"

namespace fs = std::filesystem;

using std::cerr;
using std::clog;
using std::cout;
using std::flush;
using std::ofstream;
using std::ostream;
using std::string;

bool Logger::Init() {
  fs::path log_path(Config::LOG_PATH);
  fs::path log_filename(Config::LOG_FILENAME);
  fs::path err_filename(Config::ERROR_LOG_FILENAME);
  string log_filepath = log_path / log_filename;
  string err_filepath = log_path / err_filename;

  static ofstream log_file(log_filepath);
  if (!log_file) {
    Warn("Logger::Init", "Cannot create log file " + log_filepath);
    return false;
  }
  clog.rdbuf(log_file.rdbuf());

  static ofstream err_file(err_filepath);
  if (!err_file) {
    Warn("Logger::Init", "Cannot create error log file " + err_filepath);
    return false;
  }
  cerr.rdbuf(err_file.rdbuf());

  return true;
}

void Logger::Trace(const string& context, const string& message, bool endl) {
  if (Config::LOG_LEVEL <= Config::Level::TRACE) {
    clog << "[TRACE] ";
    Log(clog, context, message, endl);
  }
}

void Logger::Debug(const string& context, const string& message, bool endl) {
  if (Config::LOG_LEVEL <= Config::Level::DEBUG) {
    clog << "[DEBUG] ";
    Log(clog, context, message, endl);
  }
}

void Logger::Info(const string& context, const string& message, bool endl) {
  if (Config::LOG_LEVEL <= Config::Level::INFO) {
    cout << "[INFO ] ";
    Log(cout, context, message, endl);
  }
}

void Logger::Warn(const string& context, const string& message, bool endl) {
  if (Config::LOG_LEVEL <= Config::Level::WARN) {
    cerr << "[WARN ] ";
    Log(cerr, context, message, endl);
  }
}

void Logger::Error(const string& context, const string& message, bool endl) {
  if (Config::LOG_LEVEL <= Config::Level::ERROR) {
    cerr << "[ERROR] ";
    Log(cerr, context, message, endl);
  }
}

void Logger::Fatal(const string& context, const string& message, bool endl) {
  if (Config::LOG_LEVEL <= Config::Level::FATAL) {
    cerr << "[FATAL] ";
    Log(cerr, context, message, endl);
  }
}

void Logger::Log(
    ostream& output, const string& context, const string& message, bool endl) {
  if (context.length()) output << context << ": ";
  output << message;
  if (endl) output << "\n";
  output << flush;
}
