#include "FFMpeg.h"
#include <iostream>
#include <sstream>
#include <filesystem>

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

void FFMpeg::addOverlayPngSequence(int x, int y, int fps, const std::string &path, const std::string &filePattern) {
    m_pOverlays.push_back({x, y, fps, path, filePattern});
}

void FFMpeg::setBackgroundClip(std::list<ClipFragment> *pClipFragments) {
    m_pOverlays.clear();
    m_pClipFragments = pClipFragments;
}

void FFMpeg::makeClip(const std::string &clipPath) {
    // make ffmpeg arguments

    std::string ffmpegArgs = s_ffmpeg + " -y ";

    // Specify the main video stream
    for(const auto& src: *m_pClipFragments){
        if ( src.in != -1)
            ffmpegArgs += " -ss " +  std::to_string(src.in) + "ms ";

        if ( src.out != -1)
            ffmpegArgs += " -to " +  std::to_string(src.out) + "ms ";

        ffmpegArgs += " -i \"" + src.fileName + "\"";
    }
    ffmpegArgs += " \\\n";

    // List of overlay files
    for(const auto& overlay: m_pOverlays){
        ffmpegArgs += " -framerate " + std::to_string(overlay.fps) + " -i " + overlay.path + "/" + overlay.filePattern;
    }

    ffmpegArgs += " \\\n";

    // Construct the filter_complex argument
    ffmpegArgs += " -filter_complex  ";
    ffmpegArgs += " \\\n";


    int idx = 1;
    std::string bkg = "[0:v]";
    std::string ovl;
    std::string merged;
    ffmpegArgs += "\"";
    for(const auto& overlay: m_pOverlays){
        ovl = "[" + std::to_string(idx) + "]";
        merged = "[out" + std::to_string(idx) + "]";
        ffmpegArgs += bkg; // Bottom video stream
        ffmpegArgs += ovl; // Overlay stream
        ffmpegArgs += "overlay=";  // Command
        ffmpegArgs += std::to_string(overlay.x) + ":" + std::to_string(overlay.y);  // Command arguments
        ffmpegArgs += merged;  // Output stream
        ffmpegArgs += ","; // Separator
        bkg = merged;
        idx++;
    }
    ffmpegArgs += "\"";

    ffmpegArgs += " \\\n";
    ffmpegArgs += " -map " + merged + " -map 0:a";
    ffmpegArgs += " \\\n";
    ffmpegArgs += " \"" + clipPath + "\"";
    std::cout <<  "[" << ffmpegArgs << "]" << std::endl;

}
