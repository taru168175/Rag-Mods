#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <thread>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <cstring>
#include <algorithm>

using namespace std;

void searchAndReplaceMemory(pid_t pid, const vector<uint8_t>& searchBytes, const vector<uint8_t>& replaceBytes) {
    string memPath = "/proc/" + to_string(pid) + "/mem";
    ifstream mapsFile("/proc/" + to_string(pid) + "/maps");
    string line;

    if (!mapsFile.is_open()) return;

    int memFd = open(memPath.c_str(), O_RDWR | O_SYNC);
    if (memFd == -1) return;

    while (getline(mapsFile, line)) {
        stringstream ss(line);
        string addressRange, perms;
        ss >> addressRange >> perms;
        
        if (perms.find("rw-p") != string::npos) {  
            string startAddrStr = addressRange.substr(0, addressRange.find('-'));
            string endAddrStr = addressRange.substr(addressRange.find('-') + 1);
            uintptr_t startAddr = strtoull(startAddrStr.c_str(), nullptr, 16);
            uintptr_t endAddr = strtoull(endAddrStr.c_str(), nullptr, 16);
            size_t regionSize = endAddr - startAddr;

            void* mappedMemory = mmap(nullptr, regionSize, PROT_READ | PROT_WRITE, MAP_SHARED, memFd, startAddr);
            if (mappedMemory == MAP_FAILED) continue;

            uint8_t* memoryBytes = static_cast<uint8_t*>(mappedMemory);

            auto it = search(memoryBytes, memoryBytes + regionSize, searchBytes.begin(), searchBytes.end());
            if (it != memoryBytes + regionSize) {
                memcpy(it, replaceBytes.data(), replaceBytes.size());
            }

            munmap(mappedMemory, regionSize);
        }
    }

    close(memFd);
}

// Run in Background Thread
void searchAndReplaceMemoryAsync(pid_t pid, const vector<uint8_t>& searchBytes, const vector<uint8_t>& replaceBytes) {
    std::thread([pid, searchBytes, replaceBytes]() {
        searchAndReplaceMemory(pid, searchBytes, replaceBytes);
    }).detach();
}