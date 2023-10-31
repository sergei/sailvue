#ifndef SAILVUE_FFMPEG_H
#define SAILVUE_FFMPEG_H

#include <string>
#include <array>
#include <ostream>
#include <string>
#include <cstdio>
#include <filesystem>

#include <list>

struct ClipFragment {
    ClipFragment(int64_t in, int64_t out, const std::string &fileName, int w, int h):
            in(in), out(out),
            fileName(fileName), width(w), height(h) {}

    int64_t in; // Milliseconds
    int64_t out; // Milliseconds
    const int width;
    const int height;
    const std::string &fileName;
};


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

struct OverlayPngSequence {
    OverlayPngSequence(int x_, int y_, float fps_, std::filesystem::path path_, std::string filePattern_)
    :x(x_), y(y_), fps(fps_), path(std::move(path_)), filePattern(std::move(filePattern_)){
    }
    int x;
    int y;
    float fps;
    const std::filesystem::path path;
    const std::string filePattern;
};

class FfmpegProgressListener {
public:
    virtual bool ffmpegProgress(uint64_t msEncoded) = 0;
};


class FFMpeg {
public:
    static bool setBinDir(const std::string &binDir);
    static std::tuple<int , int > getVideoResolution(const std::string &mp4name);

    void setBackgroundClip(std::list<ClipFragment> *pClipFragments, bool changeDuration, float durationScale);
    void addOverlayPngSequence(int x, int y, float fps, const std::filesystem::path &path, const std::string &filePattern);

    void makeClip(const std::string &clipPath, FfmpegProgressListener &progress);

    static void joinChapters(std::list<std::string> &chaptersList, const std::basic_string<char> &moviePath,
                      FfmpegProgressListener &progressListener);

private:
    static std::string s_ffmpeg;
    static std::string s_ffprobe;

    std::list<ClipFragment> *m_pClipFragments;
    std::list<OverlayPngSequence> m_pOverlays;
    bool m_changeDuration = false;
    float m_durationScale = 1;

    std::string makeClipFfmpegArgs(const std::string &clipPath);
    static std::string makeJoinChaptersFfmpegArgs(std::list<std::string> &chaptersList,const std::basic_string<char> &outPath);

    static void executeFfmpeg( const std::string &ffmpegArgs, FfmpegProgressListener &progress) ;

};


#endif //SAILVUE_FFMPEG_H
