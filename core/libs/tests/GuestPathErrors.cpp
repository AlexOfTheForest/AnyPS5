#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libkernel/File/include/FileFlags.hpp"
#include "prx/libkernel/KernelErrors.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

extern "C" {
int APS5_VABI sceKernelOpen(const char* path, int flags, std::uint16_t mode);
int APS5_VABI sceKernelUnlink(const char* path);
int APS5_VABI sceKernelClose(int d);
int APS5_VABI open_nid_postfix(const char* path, int flags, int mode);
int APS5_VABI unlink_nid_postfix(const char* path);
int* APS5_VABI __error_nid_postfix();
}

namespace {

void Require(bool value, int line) {
    if (!value) {
        std::fprintf(stderr, "path error check failed at line %d\n", line);
        std::exit(1);
    }
}
#define Check(value) Require((value), __LINE__)

constexpr int GuestEloop = 62;
constexpr int GuestEnametoolong = 63;
constexpr int SceEloop = SceKernelError(GuestEloop);
constexpr int SceEnametoolong = SceKernelError(GuestEnametoolong);

bool PosixFails(int result, int error) {
    const bool failed = result == -1 && *__error_nid_postfix() == error;
    *__error_nid_postfix() = 0;
    return failed;
}

std::filesystem::path EnterFreshRoot() {
    const auto root = std::filesystem::temp_directory_path() /
        ("anyps5-path-errors-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);
    std::filesystem::current_path(root);
    Check(::symlink("loop", "loop") == 0);
    Check(::symlink("second", "first") == 0 && ::symlink("first", "second") == 0);
    { std::ofstream file("plain.txt"); file << "plain"; }
    return root;
}

const std::string LongName(300, 'n');

void OpenAndUnlink() {
    Check(sceKernelOpen("loop", SCE_KERNEL_O_RDONLY, 0) == SceEloop);
    Check(sceKernelOpen("first", SCE_KERNEL_O_RDONLY, 0) == SceEloop);
    Check(sceKernelOpen("loop/inside", SCE_KERNEL_O_RDONLY, 0) == SceEloop);
    Check(sceKernelOpen(LongName.c_str(), SCE_KERNEL_O_RDONLY, 0) == SceEnametoolong);
    Check(sceKernelOpen((LongName + "/inside").c_str(), SCE_KERNEL_O_RDONLY, 0) == SceEnametoolong);
    Check(sceKernelOpen(LongName.c_str(), SCE_KERNEL_O_WRONLY | SCE_KERNEL_O_CREAT, 0644) == SceEnametoolong);
    Check(PosixFails(open_nid_postfix("loop", SCE_KERNEL_O_RDONLY, 0), GuestEloop));
    Check(PosixFails(open_nid_postfix(LongName.c_str(), SCE_KERNEL_O_RDONLY, 0), GuestEnametoolong));
    Check(sceKernelUnlink(LongName.c_str()) == SceEnametoolong);
    Check(sceKernelUnlink("loop/inside") == SceEloop);
    Check(PosixFails(unlink_nid_postfix(LongName.c_str()), GuestEnametoolong));
    const int plain = sceKernelOpen("plain.txt", SCE_KERNEL_O_RDONLY, 0);
    Check(plain >= 0 && sceKernelClose(plain) == 0);
}

}

int main() {
    const auto root = EnterFreshRoot();
    OpenAndUnlink();
    std::error_code error;
    std::filesystem::current_path(std::filesystem::temp_directory_path(), error);
    std::filesystem::remove_all(root, error);
    return 0;
}
