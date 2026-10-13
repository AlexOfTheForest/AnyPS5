#include <ConsoleProgress.hpp>
#include <chrono>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

using Domain::ProgressEvent;
using Domain::ProgressStage;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void requireOutput(const std::ostringstream& stream, const std::string& expected, const std::string& what) {
    require(stream.str() == expected, what + ": got \"" + stream.str() + "\"");
}

std::string row(const std::string& label, const std::string& name, std::size_t namePadding, const std::string& percent) {
    return label + std::string(22 - label.size(), ' ') + " | " + name + std::string(namePadding, ' ') + " | " + percent;
}

void percentLine() {
    std::ostringstream stream;
    Cli::ConsoleProgress progress(stream);
    progress.Report({ProgressStage::Syscalls, "/x/a.elf", 0, 100});
    requireOutput(stream, "\r" + row("Syscall scan", "a.elf", 0, "  0%"), "first update");
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    progress.Report({ProgressStage::Syscalls, "/x/a.elf", 50, 100});
    requireOutput(stream, "\r" + row("Syscall scan", "a.elf", 0, "  0%") + "\r" + row("Syscall scan", "a.elf", 0, " 50%"), "second update");
    progress.Report({ProgressStage::Syscalls, "/x/a.elf", 50, 100});
    requireOutput(stream, "\r" + row("Syscall scan", "a.elf", 0, "  0%") + "\r" + row("Syscall scan", "a.elf", 0, " 50%"), "an unchanged percentage was printed again");
    progress.Report({ProgressStage::Syscalls, "/x/a.elf", 100, 100});
    requireOutput(stream, "\r" + row("Syscall scan", "a.elf", 0, "  0%") + "\r" + row("Syscall scan", "a.elf", 0, " 50%") + "\r" + row("Syscall scan", "a.elf", 0, "100%") + "\n", "completion did not end the line");
}

void emptyTotalIsComplete() {
    std::ostringstream stream;
    Cli::ConsoleProgress progress(stream);
    progress.Report({ProgressStage::Decode, "a.elf", 0, 0});
    requireOutput(stream, "\r" + row("Instruction scan", "a.elf", 0, "100%") + "\n", "a stage with no work");
}

void moduleCounter() {
    std::ostringstream stream;
    Cli::ConsoleProgress progress(stream);
    progress.Report({ProgressStage::Modules, "dir\\mod.prx", 1, 4});
    requireOutput(stream, "\r" + row("PRX conversion", "mod.prx", 0, " 25% (1/4)"), "module counter");
}

void executableStage() {
    std::ostringstream stream;
    Cli::ConsoleProgress progress(stream);
    progress.Report({ProgressStage::Executable, "C:\\dir\\a.elf", 0, 1});
    requireOutput(stream, "Converting a.elf\n", "executable start");
    progress.Report({ProgressStage::Executable, "C:\\dir\\a.elf", 1, 1});
    requireOutput(stream, "Converting a.elf\n\n", "executable end");
}

void importStage() {
    std::ostringstream stream;
    Cli::ConsoleProgress progress(stream);
    progress.Report({ProgressStage::Imports, "a.elf", 0, 1});
    requireOutput(stream, "\nImport analysis\n", "import analysis start");
    progress.Report({ProgressStage::Imports, "a.elf", 1, 1});
    requireOutput(stream, "\nImport analysis\n\n", "import analysis end");
}

void messages() {
    std::ostringstream stream;
    Cli::ConsoleProgress progress(stream);
    progress.Message("plain");
    requireOutput(stream, "plain\n", "a message without a newline");
    progress.Message("done\n");
    requireOutput(stream, "plain\ndone\n", "a message with a newline");

    std::ostringstream active;
    Cli::ConsoleProgress interrupted(active);
    interrupted.Report({ProgressStage::Write, "a.elf", 1, 2});
    interrupted.Message("note\n");
    requireOutput(active, "\r" + row("Write output", "a.elf", 0, " 50%") + "\nnote\n", "a message did not end the active line");
}

void nameWidth() {
    std::ostringstream stream;
    Cli::ConsoleProgress progress(stream);
    progress.IncludeFile("/dir/abcdefgh.bin");
    progress.IncludeFile("C:\\dir\\short.bin");
    progress.Report({ProgressStage::Lowering, "x/a.bin", 0, 10});
    requireOutput(stream, "\r" + row("Instruction conversion", "a.bin", 7, "  0%"), "names were not padded to the widest included file");

    std::ostringstream wide;
    Cli::ConsoleProgress utf8(wide);
    utf8.IncludeFile("abcdefgh.bin");
    utf8.Report({ProgressStage::Image, "\xc3\xa4\xe2\x82\xac\xf0\x9f\x8e\xae.bin", 0, 10});
    requireOutput(wide, "\r" + row("Output image", "\xc3\xa4\xe2\x82\xac\xf0\x9f\x8e\xae.bin", 5, "  0%"), "the name width counted bytes instead of characters");
}

void invalidEvents() {
    std::ostringstream stream;
    Cli::ConsoleProgress progress(stream);
    try {
        progress.Report({ProgressStage::Syscalls, "a.elf", 5, 4});
        throw std::runtime_error("counters above the total were accepted");
    } catch (const std::logic_error& error) {
        require(std::string(error.what()) == "Invalid progress counters", std::string("wrong error: ") + error.what());
    }
    try {
        progress.Report({static_cast<ProgressStage>(99), "a.elf", 0, 0});
        throw std::runtime_error("an unknown stage was accepted");
    } catch (const std::logic_error& error) {
        require(std::string(error.what()) == "Unknown progress stage", std::string("wrong error: ") + error.what());
    }
    requireOutput(stream, "", "a rejected event wrote output");
}

}

int main() {
    try {
        percentLine();
        emptyTotalIsComplete();
        moduleCounter();
        executableStage();
        importStage();
        messages();
        nameWidth();
        invalidEvents();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
