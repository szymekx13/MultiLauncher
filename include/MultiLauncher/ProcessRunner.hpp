#pragma once
#include <string>
#include <functional>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif
#include <thread>
#ifndef _WIN32
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <array>
#include <memory>
#endif
#include "Logger.hpp"

namespace MultiLauncher {
    class ProcessRunner {
    public:
        using OutputCallback = std::function<void(const std::string&)>;

#ifdef _WIN32
        static int run(const std::string& command, OutputCallback callback = nullptr, const std::string& workingDir = "") {
            SECURITY_ATTRIBUTES saAttr;
            saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
            saAttr.bInheritHandle = TRUE;
            saAttr.lpSecurityDescriptor = NULL;

            HANDLE hChildStd_OUT_Rd = NULL;
            HANDLE hChildStd_OUT_Wr = NULL;

            if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0)) {
                return -1;
            }

            if (!SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) {
                return -1;
            }

            STARTUPINFOA si = { 0 };
            si.cb = sizeof(STARTUPINFOA);
            si.hStdError = hChildStd_OUT_Wr;
            si.hStdOutput = hChildStd_OUT_Wr;
            si.dwFlags |= STARTF_USESTDHANDLES;

            PROCESS_INFORMATION pi = { 0 };
            char* cmd = _strdup(command.c_str());

            BOOL bSuccess = CreateProcessA(NULL, cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, 
                                         workingDir.empty() ? NULL : workingDir.c_str(), &si, &pi);
            
            free(cmd);

            if (!bSuccess) {
                CloseHandle(hChildStd_OUT_Wr);
                CloseHandle(hChildStd_OUT_Rd);
                return -1;
            }

            CloseHandle(hChildStd_OUT_Wr);

            char chBuf[4096];
            DWORD dwRead;
            std::string lineBuffer;

            while (ReadFile(hChildStd_OUT_Rd, chBuf, 4095, &dwRead, NULL) && dwRead > 0) {
                chBuf[dwRead] = '\0';
                lineBuffer += chBuf;
                
                size_t pos;
                while ((pos = lineBuffer.find('\n')) != std::string::npos) {
                    std::string line = lineBuffer.substr(0, pos);
                    if (!line.empty() && line.back() == '\r') line.pop_back();
                    if (callback) callback(line);
                    lineBuffer.erase(0, pos + 1);
                }
            }

            if (callback && !lineBuffer.empty()) {
                if (lineBuffer.back() == '\r') lineBuffer.pop_back();
                callback(lineBuffer);
            }

            WaitForSingleObject(pi.hProcess, INFINITE);
            DWORD exitCode;
            GetExitCodeProcess(pi.hProcess, &exitCode);

            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            CloseHandle(hChildStd_OUT_Rd);

            return (int)exitCode;
            return (int)exitCode;
        }
#else
        static int run(const std::string& command, OutputCallback callback = nullptr, const std::string& workingDir = "") {
            std::string finalCmd = command;
            if (!workingDir.empty()) {
                finalCmd = "cd \"" + workingDir + "\" && " + command;
            }
            // Redirect stderr to stdout
            finalCmd += " 2>&1";

            std::array<char, 128> buffer;
            std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(finalCmd.c_str(), "r"), pclose);
            if (!pipe) {
                return -1;
            }

            while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
                std::string line(buffer.data());
                while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
                    line.pop_back();
                }
                if (callback) callback(line);
            }
            
            int returnCode = pclose(pipe.get());
            if (WIFEXITED(returnCode)) {
                return WEXITSTATUS(returnCode);
            }
            return -1;
        }
#endif

        static void runAsync(const std::string& command, std::function<void(int)> onComplete, OutputCallback callback = nullptr, const std::string& workingDir = "") {
            std::thread([=]() {
                int result = run(command, callback, workingDir);
                if (onComplete) onComplete(result);
            }).detach();
        }
    };
}
