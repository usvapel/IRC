#include "Logger.hpp"

#include <fcntl.h>

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>

std::mutex    Logger::logMutex;
std::ofstream Logger::logFile;

Logger::Logger(const char *file, int line) : file_(file), line_(line) {}

std::string Logger::getTimestamp() {
  std::time_t now_time =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::ostringstream ss;
  ss << std::put_time(std::localtime(&now_time), "%Y-%m-%d %H:%M:%S");
  return ss.str();
}

Logger::~Logger() {
  std::string fileStr(file_);
  size_t      lastSlash = fileStr.find_last_of("/\\");
  std::string shortFile = (lastSlash == std::string::npos)
                              ? fileStr
                              : fileStr.substr(lastSlash + 1);

  std::ostringstream finalLog;
  finalLog << "[" << getTimestamp() << "] "
           << "[" << shortFile << ":" << line_ << "] " << oss_.str() << "\n";

  std::lock_guard<std::mutex> lock(logMutex);
  std::ofstream               file("log", std::ios::app);
  std::ostream               &out = file;
  // std::ostream               &out = std::cout;
  out << finalLog.str();
  out.flush();
  if (logFile.is_open()) {
    logFile << finalLog.str();
    logFile.flush();
  }
}

void Logger::setLogFile(const std::string &filename) {
  logFile.open(filename, std::ios_base::app);
  if (!logFile.is_open()) {
    std::cerr << "Failed to open log file: " << filename << std::endl;
  }
}
