#pragma once

#include <string>
#include <vector>

#include <spdlog/common.h>

namespace Ship {
class ConsoleWindow;
}

// The log as the game shows it: every message goes to the console's Logs channel, and while an extraction runs,
// its warnings and errors and what it is working on are kept for the extraction's own panel and error popup.
namespace LogFeed {

struct Line {
    spdlog::level::level_enum level;
    std::string text;
};

// Adds the feed to the default logger. Call once, after the logger is set up.
void Install();
// Moves new messages into the console's Logs channel. Call every frame from the UI thread.
void Pump(Ship::ConsoleWindow& console);

// Starts and stops collecting an extraction's problems and progress.
void BeginCapture();
void EndCapture();
// The warnings and errors collected since BeginCapture, oldest first, at most the last max of them.
std::vector<Line> CapturedProblems(size_t max);
// The asset or file the extraction is working on.
std::string CurrentActivity();

} // namespace LogFeed
