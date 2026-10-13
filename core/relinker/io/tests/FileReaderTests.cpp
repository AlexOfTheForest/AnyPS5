#include <io/FileReader.hpp>
#include <io/FileWriter.hpp>
#include <io/NativePath.hpp>
#include <domain/Types.hpp>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void requireReadFailure(const std::string& path, const std::string& expectedMessage) {
    try {
        Io::FileReader{}.Read(path);
    } catch (const Domain::RelinkerException& error) {
        require(error.what() == expectedMessage, "wrong error for " + path + ": got \"" + error.what() + "\", expected \"" + expectedMessage + "\"");
        return;
    }
    throw std::runtime_error("expected a read failure for " + path);
}

void roundTrip(const std::filesystem::path& directory) {
    std::vector<std::uint8_t> data(70000);
    for (std::size_t i = 0; i < data.size(); ++i) data[i] = static_cast<std::uint8_t>(i * 31 + 7);
    data[0] = 0x00;
    data[1] = 0xff;
    data[2] = 0x0d;
    data[3] = 0x0a;
    const auto path = Io::Utf8Path(directory / "binary.bin");
    Io::FileWriter{}.Write(path, data);
    require(Io::FileReader{}.Read(path) == data, "binary content did not survive a write and read");
}

void emptyFile(const std::filesystem::path& directory) {
    const auto path = Io::Utf8Path(directory / "empty.bin");
    Io::FileWriter{}.Write(path, std::vector<std::uint8_t>{});
    require(Io::FileReader{}.Read(path).empty(), "an empty file did not read as an empty buffer");
}

void nonAsciiPath(const std::filesystem::path& directory) {
    const std::string name = "\xc3\xa4\xe2\x82\xac\xf0\x9f\x8e\xae.bin";
    const std::vector<std::uint8_t> data{1, 2, 3, 4};
    const auto path = Io::Utf8Path(directory / Io::NativePath(name));
    require(Io::Utf8Path(Io::NativePath(name)) == name, "a UTF-8 name did not survive the native path conversion");
    Io::FileWriter{}.Write(path, data);
    require(Io::FileReader{}.Read(path) == data, "a file under a non-ASCII name was not read back");
}

void failures(const std::filesystem::path& directory) {
    const auto missing = Io::Utf8Path(directory / "missing.bin");
    requireReadFailure(missing, "Cannot open file: " + missing);
    const auto folder = Io::Utf8Path(directory);
    requireReadFailure(folder, "Cannot open file: " + folder);
}

}

int main() {
    std::error_code error;
    const auto directory = std::filesystem::temp_directory_path(error) / "anyps5-file-reader-tests";
    try {
        require(!error, "no temporary directory is available");
        std::filesystem::remove_all(directory, error);
        std::filesystem::create_directories(directory);
        roundTrip(directory);
        emptyFile(directory);
        nonAsciiPath(directory);
        failures(directory);
    } catch (const std::exception& caught) {
        std::cerr << caught.what() << '\n';
        std::filesystem::remove_all(directory, error);
        return 1;
    }
    std::filesystem::remove_all(directory, error);
    return 0;
}
