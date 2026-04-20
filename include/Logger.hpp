#pragma once
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

using TimeStamp = std::chrono::time_point<std::chrono::steady_clock>;

constexpr size_t logBufferSize = 1024 * 1024;
constexpr size_t diskCheckInterval = 5;

class Logger {
  public:
    Logger(const char *file, int line);
    ~Logger();

    template <typename T>
    Logger &operator<<(const T &msg) {
      _oss << msg;
      return *this;
    }
    /**
     * @brief Sets the logfile the server is writing to
     *
     * @param filename
     * @return void
     */
    static void setLogFile(const std::string &filename);

  private:
    std::ostringstream _oss;
    const char        *_file;
    int                _line;

    static std::ofstream _logFile;
    static std::string   _logPath;
    static TimeStamp     _lastCheck;
    static bool          _hasSpace;

    std::string getTimestamp();

    /**
     * @brief Checks if there is sufficient disk space: the length of the
     * message + logBufferSize (1MB)
     *
     * @param line The string to be logged
     * @return bool
     */
    static bool hasDiskSpace(const std::string &line);
};

#define LOG Logger(__FILE__, __LINE__)
