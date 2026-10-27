#include "LogFeed.h"

#include <deque>
#include <memory>
#include <mutex>

#include <ship/window/gui/ConsoleWindow.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/spdlog.h>

namespace {

constexpr size_t kPendingMax = 4000; // messages waiting for the next frame
constexpr size_t kHistoryMax = 2000; // what the Logs channel shows
constexpr size_t kProblemsMax = 200; // warnings and errors kept per extraction

std::mutex sMutex;
std::deque<LogFeed::Line> sPending;
std::deque<LogFeed::Line> sHistory;
size_t sShown = 0; // lines appended to the Logs channel since it was last refilled

bool sCapturing = false;
std::deque<LogFeed::Line> sProblems;
std::string sActivity;

// Torch logs "- [TYPE] Processing <name> at <offset>" for each asset it extracts.
std::string ActivityOf(const std::string& text) {
    if (text.rfind("- [", 0) != 0) {
        return "";
    }
    const size_t start = text.find("Processing ");
    if (start == std::string::npos) {
        return "";
    }
    const size_t name = start + sizeof("Processing ") - 1;
    const size_t end = text.find(" at ", name);
    return text.substr(name, end == std::string::npos ? std::string::npos : end - name);
}

class FeedSink final : public spdlog::sinks::base_sink<std::mutex> {
  protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        std::string text(msg.payload.data(), msg.payload.size());
        while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) {
            text.pop_back();
        }
        std::lock_guard<std::mutex> lock(sMutex);
        if (sCapturing) {
            if (msg.level >= spdlog::level::warn) {
                sProblems.push_back({ msg.level, text });
                if (sProblems.size() > kProblemsMax) {
                    sProblems.pop_front();
                }
            } else if (auto activity = ActivityOf(text); !activity.empty()) {
                sActivity = std::move(activity);
            }
        }
        sPending.push_back({ msg.level, std::move(text) });
        if (sPending.size() > kPendingMax) {
            sPending.pop_front();
        }
    }

    void flush_() override {
    }
};

} // namespace

void LogFeed::Install() {
    auto logger = spdlog::default_logger();
    if (logger == nullptr) {
        return;
    }
    logger->sinks().push_back(std::make_shared<FeedSink>());
}

void LogFeed::Pump(Ship::ConsoleWindow& console) {
    std::deque<Line> lines;
    {
        std::lock_guard<std::mutex> lock(sMutex);
        lines.swap(sPending);
    }
    if (lines.empty()) {
        return;
    }
    for (auto& line : lines) {
        sHistory.push_back(line);
        if (sHistory.size() > kHistoryMax) {
            sHistory.pop_front();
        }
    }
    // The console keeps every line it is given, so start the channel over from the history once it has grown.
    if (sShown + lines.size() > kHistoryMax * 2) {
        console.ClearLogs("Logs");
        sShown = 0;
        lines = sHistory;
    }
    for (const auto& line : lines) {
        console.Append("Logs", line.level, "%s", line.text.c_str());
    }
    sShown += lines.size();
}

void LogFeed::BeginCapture() {
    std::lock_guard<std::mutex> lock(sMutex);
    sCapturing = true;
    sProblems.clear();
    sActivity.clear();
}

void LogFeed::EndCapture() {
    std::lock_guard<std::mutex> lock(sMutex);
    sCapturing = false;
    sActivity.clear();
}

std::vector<LogFeed::Line> LogFeed::CapturedProblems(size_t max) {
    std::lock_guard<std::mutex> lock(sMutex);
    const size_t from = sProblems.size() > max ? sProblems.size() - max : 0;
    return { sProblems.begin() + from, sProblems.end() };
}

std::string LogFeed::CurrentActivity() {
    std::lock_guard<std::mutex> lock(sMutex);
    return sActivity;
}
