#ifndef SAILVUE_FFMPEG_H
#define SAILVUE_FFMPEG_H

#include <string>
#include <array>
#include <ostream>
#include <string>
#include <cstdio>

struct CommandResult {
    std::string output;
    int exitstatus;
    friend std::ostream &operator<<(std::ostream &os, const CommandResult &result) {
        os << "command exitstatus: " << result.exitstatus << " output: " << result.output;
        return os;
    }
    bool operator==(const CommandResult &rhs) const {
        return output == rhs.output &&
               exitstatus == rhs.exitstatus;
    }
    bool operator!=(const CommandResult &rhs) const {
        return !(rhs == *this);
    }
};

class Command {
public:
    /**
         * Execute system command and get STDOUT result.
         * Regular system() only gives back exit status, this gives back output as well.
         * @param command system command to execute
         * @return commandResult containing STDOUT (not stderr) output & exitstatus
         * of command. Empty if command failed (or has no output). If you want stderr,
         * use shell redirection (2&>1).
         */
    static CommandResult exec(const std::string &command) {
        int exitcode = 0;
        std::array<char, 8192> buffer{};
        std::string result;
#ifdef _WIN32
        #define popen _popen
#define pclose _pclose
#define WEXITSTATUS
#endif
        FILE *pipe = popen(command.c_str(), "r");
        if (pipe == nullptr) {
            throw std::runtime_error("popen() failed!");
        }
        try {
            std::size_t bytesread;
            while ((bytesread = std::fread(buffer.data(), sizeof(buffer.at(0)), sizeof(buffer), pipe)) != 0) {
                result += std::string(buffer.data(), bytesread);
            }
        } catch (...) {
            pclose(pipe);
            throw;
        }
        // Workaround "error: cannot take the address of an rvalue of type 'int'" on MacOS
        // see e.g. https://github.com/BestImageViewer/geeqie/commit/75c7df8b96592e10f7936dc1a28983be4089578c
        int res = pclose(pipe);
        exitcode = WEXITSTATUS(res);
        return CommandResult{result, exitcode};
    }

};

class FFMpeg {
public:
    static bool setBinDir(const std::string &binDir);
    static std::tuple<int , int > getVideoResolution(const std::string &mp4name);

private:
    static std::string s_ffmpeg;
    static std::string s_ffprobe;
};


#endif //SAILVUE_FFMPEG_H
