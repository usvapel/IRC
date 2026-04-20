#include "Logger.hpp"


// static variable inits
std::ofstream Logger::_logFile;
std::string   Logger::_logPath;
TimeStamp     Logger::_lastCheck;
bool          Logger::_hasSpace = true;

Logger::Logger(const char *file, int line) : _file(file), _line(line) {}

std::string Logger::getTimestamp() {
  std::time_t now_time =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::ostringstream ss;
  ss << std::put_time(std::localtime(&now_time), "%Y-%m-%d %H:%M:%S");
  return ss.str();
}

Logger::~Logger() {
  std::string        fileStr(_file);
  size_t             lastSlash = fileStr.find_last_of("/\\");
  std::string        shortFile = (lastSlash == std::string::npos)
                                     ? fileStr
                                     : fileStr.substr(lastSlash + 1);
  std::ostringstream finalLog;
  finalLog << "[" << getTimestamp() << "] "
           << "[" << shortFile << ":" << _line << "] " << _oss.str() << "\n";
  if (_logFile.is_open()) {
    std::string line = finalLog.str();
    if (hasDiskSpace(line)) {
      _logFile << line;
      _logFile.flush();
    } else {
      std::cerr << "Insufficent disk space for logging, nothing written"
                << std::endl;
    }
  }
}

void Logger::setLogFile(const std::string &filename) {
  _logFile.open(filename, std::ios_base::app);
  if (!_logFile.is_open()) {
    std::cerr << "Failed to open log file: " << filename << std::endl;
  } else {
    _logPath = filename;
    _hasSpace = hasDiskSpace(filename);
  }
}

bool Logger::hasDiskSpace(const std::string &line) {
  TimeStamp now = std::chrono::steady_clock::now();
  if (now - _lastCheck < std::chrono::seconds(diskCheckInterval))
    return _hasSpace;
  try {
    _lastCheck = now;
    std::filesystem::space_info si = std::filesystem::space(_logPath);
    _hasSpace = ((si.available > line.length() + logBufferSize) ? true : false);
  } catch (std::filesystem::filesystem_error &err) {
    std::cerr << "Error checking disk space: " << err.what() << std::endl;
    _hasSpace = false;
  }
  return _hasSpace;
}
