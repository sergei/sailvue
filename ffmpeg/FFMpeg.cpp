#include "FFMpeg.h"
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <csignal>
#include <fstream>

std::string FFMpeg::s_ffmpeg = "/dev/null";
 std::string FFMpeg::s_ffprobe = "/dev/null";

bool FFMpeg::setBinDir(const std::string &binDir) {
    std::filesystem::path ffmpegPath(binDir);
    std::filesystem::path absoluteFfmpegPath = std::filesystem::absolute(binDir);


    // Check if the directory exist
    if (!std::filesystem::exists(absoluteFfmpegPath)) {
        std::cerr << "FFMpeg::setBinDir: " << absoluteFfmpegPath << " does not exist" << std::endl;
        return false;
    }

    // Check if ffmpeg and ffprobe files exist
    s_ffmpeg = absoluteFfmpegPath / "ffmpeg";
    s_ffprobe = absoluteFfmpegPath / "ffprobe";

    if (!std::filesystem::is_regular_file(s_ffmpeg ))  {
        std::cerr << "FFMpeg::setBinDir: " << s_ffmpeg << " does not exist" << std::endl;
        return false;
    }

    if (!std::filesystem::is_regular_file(s_ffprobe )) {
        std::cerr << "FFMpeg::setBinDir: " << s_ffprobe << " does not exist" << std::endl;
        return false;
    }

    return true;
}

std::tuple<int, int> FFMpeg::getVideoResolution(const std::string &mp4name) {
    // Build the ffprobe command
    std::string cmd = s_ffprobe
            + " -v error -select_streams v:0 -show_entries stream=width,height,fps -of csv=p=0 "
            + "\"" + mp4name + "\"";

    // Execute it
    CommandResult res = Command::exec(cmd);
    if ( res.exitstatus == 0){
        std::istringstream iss(res.output);
        std::string width;
        std::string height;
        std::getline(iss, width, ',');
        std::getline(iss, height, ',');
        return {stoi(width), stoi(height)};
    }
    return {-1, -1};
}

void FFMpeg::addOverlayPngSequence(int x, int y, float fps, const std::filesystem::path &path, const std::string &filePattern) {
    m_pOverlays.emplace_back(x, y, fps, path, filePattern);
}

void FFMpeg::setBackgroundClip(std::list<ClipFragment> *pClipFragments, bool changeDuration, float durationScale) {
    m_pOverlays.clear();
    m_pClipFragments = pClipFragments;
    m_changeDuration = changeDuration;
    m_durationScale = durationScale;
}
#define READ 0
#define WRITE 1

pid_t
popen2(const char *command, int *infp, int *outfp)
{
    int p_stdin[2], p_stdout[2];
    pid_t pid;

    if (pipe(p_stdin) != 0 || pipe(p_stdout) != 0)
        return -1;

    pid = fork();

    if (pid < 0)
        return pid;
    else if (pid == 0)
    {
        close(p_stdin[WRITE]);
        dup2(p_stdin[READ], READ);
        close(p_stdout[READ]);
        dup2(p_stdout[WRITE], WRITE);

        execl("/bin/sh", "sh", "-c", command, NULL);
        perror("execl");
        exit(1);
    }

    if (infp == nullptr)
        close(p_stdin[WRITE]);
    else
        *infp = p_stdin[WRITE];

    if (outfp == nullptr)
        close(p_stdout[READ]);
    else
        *outfp = p_stdout[READ];

    return pid;
}
void FFMpeg::makeClip(const std::string &clipPath, FfmpegProgressListener &progress) {
    // make ffmpeg arguments
    std::string ffmpegArgs = makeClipFfmpegArgs(clipPath);
    std::cout <<  "[" << ffmpegArgs << "]" << std::endl;

    // Execute ffmpeg
    executeFfmpeg(ffmpegArgs, progress);

    std::cout << "FFMpeg::makeClip: done " << std::endl;
}

void FFMpeg::executeFfmpeg(const std::string &ffmpegArgs, FfmpegProgressListener &progress) {
    int outfp;
    int pid = popen2((const char *)ffmpegArgs.c_str(), nullptr, &outfp);
    if (pid == -1) {
        throw std::runtime_error("popen2() failed!");
    }
    std::cout << "FFMpeg::makeClip: PID " <<  pid << std::endl;

    FILE *outStream = fdopen(outfp, "r");

    try {
        std::array<char, 256> buffer{};

        while(fgets(buffer.data(), sizeof(buffer), outStream) != nullptr) {
            std::string  out = std::string(buffer.data());
//            std::cout << out  << std::endl;
            // Split in two strings separated by = sign
            std::string::size_type pos = out.find('=');
            if ( pos == std::string::npos){
                continue;
            }
            // The first one is the keyword
            std::string keyword = out.substr(0, pos);
            std::string value = out.substr(pos+1);
            value.pop_back(); // Remove the trailing \n
            if ( keyword == "out_time_us" ){
                // The second one is the value
                // Convert to milliseconds
                int64_t msEncoded = std::stoll(value) / 1000;
                bool stopRequested = progress.ffmpegProgress(msEncoded);
                if( stopRequested ){
                    std::cout << "FFMpeg::makeClip: killing PID " <<  pid << std::endl;
                    kill(pid, SIGINT);
                    fclose(outStream);
                    break;
                }
            } else if( keyword == "progress"  ){
                if ( value == "end" ){
                    std::cout << "FFMpeg::makeClip: reached the end " <<  pid << std::endl;
                    int stat;
                    fclose(outStream);
                    waitpid(pid, &stat, 0);
                    break;
                }
            }
        }
    } catch (...) {
        fclose(outStream);
        throw;
    }
}

std::string FFMpeg::makeClipFfmpegArgs(const std::string &clipPath) {
    std::string ffmpegArgs = s_ffmpeg + " -progress - -nostats -y ";
    ffmpegArgs += " \\\n";

    int clipIdx = 0;
    // Specify the main video stream
    for(const auto& src: *m_pClipFragments){
        if ( src.in != -1)
            ffmpegArgs += " -ss " +  std::to_string(src.in) + "ms ";

        if ( src.out != -1)
            ffmpegArgs += " -to " +  std::to_string(src.out) + "ms ";

        ffmpegArgs += " -i \"" + src.fileName + "\"";
        ffmpegArgs += " \\\n";
        clipIdx++;
    }

    int firstOverlayIdx = clipIdx;

    // List of overlay files
    for(const auto& overlay: m_pOverlays){
        ffmpegArgs += " -framerate " + std::to_string(overlay.fps) + " -i " + overlay.path.string() + "/" + overlay.filePattern;
        ffmpegArgs += " \\\n";
        clipIdx++;
    }

    // Construct the filter_complex argument
    ffmpegArgs += " -filter_complex  ";
    ffmpegArgs += " \\\n";
    ffmpegArgs += "\"";

    bool concatIsRequired = m_pClipFragments->size() > 1;

    if ( concatIsRequired ){
        // Concatenate the video streams
        for(int i=0; i<firstOverlayIdx; i++){
            ffmpegArgs += "[" + std::to_string(i) + "]";
        }
        ffmpegArgs += " concat=n=" + std::to_string(m_pClipFragments->size()) + ":v=1:a=0 [concv];";

        // Concatenate the audio streams
        if ( !m_changeDuration ) {  // If duration is changed, don't copy the audio
            for (int i = 0; i < firstOverlayIdx; i++) {
                ffmpegArgs += "[" + std::to_string(i) + "]";
            }
            ffmpegArgs += " concat=n=" + std::to_string(m_pClipFragments->size()) + ":v=0:a=1 [conca];";
        }
    }

    std::string bkg;
    std::string audio;
    if ( concatIsRequired ) {
        bkg = "[concv]";  // Overlays go on top of concatenated clips (video)
        audio = "[conca]";  //Output clip audio is  concatenated clips audio
    }else{
        bkg = "[0:v]";   // Overlays go on top of the first clip (video)
        audio = "0:a";   // Output clip audio is the first clip audio
    }


    if ( m_changeDuration ){
        ffmpegArgs += bkg + "setpts=" + std::to_string(m_durationScale) + "*PTS [ovl];";
        bkg = "[ovl]";
    }


    int idx = firstOverlayIdx;
    std::string ovl;
    std::string merged;
    for(const auto& overlay: m_pOverlays){
        ovl = "[" + std::to_string(idx) + "]";
        merged = "[out" + std::to_string(idx) + "]";
        ffmpegArgs += bkg; // Bottom video stream
        ffmpegArgs += ovl; // Overlay stream
        ffmpegArgs += " overlay=";  // Command
        ffmpegArgs += std::to_string(overlay.x) + ":" + std::to_string(overlay.y);  // Command arguments
        ffmpegArgs += " ";
        ffmpegArgs += merged;  // Output stream
        ffmpegArgs += "; "; // Separator
        bkg = merged;
        idx++;
    }
    ffmpegArgs += "\"";

    ffmpegArgs += " \\\n";
    ffmpegArgs += " -map " + merged;
    if ( !m_changeDuration ){  // If duration is changed, don't copy the audio
        ffmpegArgs += " -map " + audio;
    }
    ffmpegArgs += " \\\n";
    ffmpegArgs += " \"" + clipPath + "\"";
    return ffmpegArgs;
}

std::string FFMpeg::makeJoinChaptersFfmpegArgs(std::list<std::string> &chaptersList, const std::basic_string<char> &outPath) {

    // Create file containing the list of clips being merged
    std::filesystem::path listPath = std::filesystem::temp_directory_path() / "list.txt";
    std::ofstream listFile(listPath);
    for( const auto& chapter: chaptersList) {
        listFile << "file '" << chapter << "'" << std::endl;
    }
    listFile.close();

    std::string ffmpegArgs = s_ffmpeg + " -progress - -nostats -y ";
    ffmpegArgs += " \\\n";

    ffmpegArgs += " -f concat -safe 0 -i \"" + listPath.string() + "\"";

    ffmpegArgs += " -c copy \"" + outPath + "\"";

    return ffmpegArgs;
}


void FFMpeg::joinChapters(std::list<std::string> &chaptersList, const std::basic_string<char> &moviePath,
                          FfmpegProgressListener &progress) {
    // make ffmpeg arguments
    std::string ffmpegArgs = makeJoinChaptersFfmpegArgs(chaptersList, moviePath);
    std::cout <<  "[" << ffmpegArgs << "]" << std::endl;

    // Execute ffmpeg
    executeFfmpeg(ffmpegArgs, progress);

    std::cout << "FFMpeg::makeClip: done " << std::endl;
}


