#pragma once
#include <vector>
#include <string>
#include <mutex>

namespace MultiLauncher {

class Logger {
public:
    enum LogLevel{
        Info,
        Error
    };
    struct LogEntry{
        LogLevel lvl;
        std::string s;
    };
    static Logger& instance() {
        static Logger inst;
        return inst;
    }

    void info(const std::string& s) { append(LogLevel::Info, s); }
    void error(const std::string& s) { append(LogLevel::Error, s); }
    void clear() { std::lock_guard<std::mutex> lk(m_); logs_.clear(); }

    std::vector<LogEntry> getLogs() const {
        std::lock_guard<std::mutex> lk(m_);
        return logs_;
    }
    bool autoscroll = true;
    
private:
    Logger() = default;
    void append(LogLevel lvl, const std::string& s) {
        std::lock_guard<std::mutex> lk(m_);
        logs_.push_back({lvl, s});
        if (logs_.size() > 1000) logs_.erase(logs_.begin());
    }
    mutable std::mutex m_;
    std::vector<LogEntry> logs_;
};

} // namespace MultiLauncher
