#pragma once
#include <vector>
#include <string>
#include <mutex>

namespace MultiLauncher {

class Logger {
public:
    static Logger& instance() {
        static Logger inst;
        return inst;
    }

    void info(const std::string& s) { append("[INFO] " + s); }
    void error(const std::string& s) { append("[ERROR] " + s); }
    void clear() { std::lock_guard<std::mutex> lk(m_); logs_.clear(); }

    std::vector<std::string> getLogs() const {
        std::lock_guard<std::mutex> lk(m_);
        return logs_;
    }

private:
    Logger() = default;
    void append(const std::string& s) {
        std::lock_guard<std::mutex> lk(m_);
        logs_.push_back(s);
        if (logs_.size() > 1000) logs_.erase(logs_.begin());
    }

    mutable std::mutex m_;
    std::vector<std::string> logs_;
};

} // namespace MultiLauncher
