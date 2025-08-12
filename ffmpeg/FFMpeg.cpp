#include "FFMpeg.h"
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <csignal>
#include <fstream>
#include <algorithm>

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
    if( ! executeFfmpeg(ffmpegArgs, progress) ){
            std::cout << "Deleting ffmpeg output file"  << std::endl;
            std::filesystem::remove(clipPath);
    }

    std::cout << "FFMpeg::makeClip: done " << std::endl;
}

bool FFMpeg::executeFfmpeg(const std::string &ffmpegArgs, FfmpegProgressListener &progress) {
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
                    return false;
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
        return false;
    }
    return true;
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
        ffmpegArgs += " -framerate " + std::to_string(overlay.fps) + " -i \"" + overlay.path.string() + "/" + overlay.filePattern + "\"";
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
    if ( !m_changeDuration && !m_pClipFragments->empty() ){  // If duration is changed or no GOPRO clips presents, don't copy the audio
        ffmpegArgs += " -map " + audio;
    }
    ffmpegArgs += " \\\n";

    if( m_pClipFragments->empty() ){
        ffmpegArgs += "-vcodec png ";
        ffmpegArgs += " \\\n";
        ffmpegArgs += "-pix_fmt yuva420p ";
        ffmpegArgs += " \\\n";
    }


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


void FFMpeg::addOverlayFrameSequence(int x, int y, float fps) {
    m_frameOverlays.emplace_back(x, y, fps);
}

void FFMpeg::addFrameToOverlay(AVFrame* frame) {
    if (m_frameOverlays.empty()) {
        throw std::runtime_error("No frame overlay sequence initialized");
    }
    m_frameOverlays.back().frames.push_back(frame);
}

bool FFMpeg::encodeFrame(AVFrame* frame) {
    if (!m_codecContext) {
        std::cerr << "Encoder not initialized" << std::endl;
        return false;
    }

    if (!frame) {
        // For flushing, use avcodec_send_frame directly
        int ret = avcodec_send_frame(m_codecContext, nullptr);
        if (ret < 0) {
            char errBuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errBuf, AV_ERROR_MAX_STRING_SIZE);
            std::cerr << "Error flushing encoder: " << errBuf << std::endl;
            return false;
        }
        return true;
    }

    // Set frame PTS
    frame->pts = m_nextPts++;

    // Send frame to encoder
    int ret = avcodec_send_frame(m_codecContext, frame);
    if (ret < 0) {
        char errBuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errBuf, AV_ERROR_MAX_STRING_SIZE);
        std::cerr << "Error sending frame to encoder: " << errBuf << std::endl;
        return false;
    }

    // Get packets from encoder
    while (true) {
        AVPacket* packet = av_packet_alloc();
        ret = avcodec_receive_packet(m_codecContext, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            av_packet_free(&packet);
            break;
        } else if (ret < 0) {
            av_packet_free(&packet);
            char errBuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errBuf, AV_ERROR_MAX_STRING_SIZE);
            std::cerr << "Error receiving packet: " << errBuf << std::endl;
            return false;
        }

        // Rescale packet timestamps
        av_packet_rescale_ts(packet, m_codecContext->time_base, m_videoStream->time_base);
        packet->stream_index = m_videoStream->index;

        // Write packet to file
        ret = av_interleaved_write_frame(m_formatContext, packet);
        av_packet_free(&packet);
        if (ret < 0) {
            char errBuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errBuf, AV_ERROR_MAX_STRING_SIZE);
            std::cerr << "Error writing packet: " << errBuf << std::endl;
            return false;
        }
    }

    return true;
}
bool FFMpeg::finalizeEncoding() {
    if (!m_codecContext || !m_formatContext) {
        std::cerr << "Encoder not initialized" << std::endl;
        return false;
    }

    // Flush the encoder without calling encodeFrame(nullptr)
    int ret = avcodec_send_frame(m_codecContext, nullptr);
    if (ret < 0) {
        char errBuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errBuf, AV_ERROR_MAX_STRING_SIZE);
        std::cerr << "Error flushing encoder: " << errBuf << std::endl;
        return false;
    }

    // Get remaining packets
    while (true) {
        AVPacket* packet = av_packet_alloc();
        ret = avcodec_receive_packet(m_codecContext, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            av_packet_free(&packet);
            break;
        } else if (ret < 0) {
            av_packet_free(&packet);
            char errBuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errBuf, AV_ERROR_MAX_STRING_SIZE);
            std::cerr << "Error receiving packet: " << errBuf << std::endl;
            return false;
        }

        // Rescale packet timestamps
        av_packet_rescale_ts(packet, m_codecContext->time_base, m_videoStream->time_base);
        packet->stream_index = m_videoStream->index;

        // Write packet to file
        ret = av_interleaved_write_frame(m_formatContext, packet);
        av_packet_free(&packet);
        if (ret < 0) {
            char errBuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errBuf, AV_ERROR_MAX_STRING_SIZE);
            std::cerr << "Error writing packet: " << errBuf << std::endl;
            return false;
        }
    }

    std::cout << "Dumping format context before finalization:" << std::endl;
    av_dump_format(m_formatContext, 0, "output.mp4", 1);

    // Write trailer
    ret = av_write_trailer(m_formatContext);
    if (ret < 0) {
        std::cerr << "Error writing trailer" << std::endl;
        return false;
    }
  if (ret >= 0) {
    std::cout << "Dumping format context after trailer:" << std::endl;
    av_dump_format(m_formatContext, 0, "output.mp4", 1);
  }


    if (m_formatContext && m_formatContext->pb) {
      avio_flush(m_formatContext->pb);
      std::cout << "File flushed to disk via avio_flush()" << std::endl;
    }

  // Close file
    avio_closep(&m_formatContext->pb);

    std::cout << "Dumping format context after file closed:" << std::endl;
    av_dump_format(m_formatContext, 0, "output_path", 1);

    // Free resources
    avcodec_free_context(&m_codecContext);
    avformat_free_context(m_formatContext);

    m_codecContext = nullptr;
    m_formatContext = nullptr;
  m_videoStream = nullptr;
    m_nextPts = 0;

    return true;
}

bool FFMpeg::copyQImageToAVFrame(const QImage& image, AVFrame* frame) {
    // Check if frame is valid
    if (!frame) return false;

    // For direct BGRA to BGRA copy (no conversion needed)
    if (m_codecContext->pix_fmt == AV_PIX_FMT_BGRA &&
        image.format() == QImage::Format_ARGB32) {

      av_frame_make_writable(frame);

      // Direct copy from QImage to AVFrame
      for (int y = 0; y < image.height(); y++) {
        memcpy(frame->data[0] + y * frame->linesize[0],
               image.constScanLine(y),
               std::min(image.bytesPerLine(), static_cast<qsizetype>(frame->linesize[0])));
      }

      return true;
    }

    // Initialize source frame if needed
    if (!m_initialized) {
        m_srcFrame = av_frame_alloc();
        if (!m_srcFrame) return false;

        m_srcFrame->width = image.width();
        m_srcFrame->height = image.height();
        m_srcFrame->format = AV_PIX_FMT_BGRA;  // QImage::Format_ARGB32 is BGRA in memory

        if (av_frame_get_buffer(m_srcFrame, 32) < 0) {
            av_frame_free(&m_srcFrame);
            m_srcFrame = nullptr;
            return false;
        }

        // Create SwsContext only once
        m_swsCtx = sws_getContext(
                image.width(), image.height(), AV_PIX_FMT_BGRA,
                image.width(), image.height(), m_codecContext->pix_fmt,
                SWS_BICUBIC, nullptr, nullptr, nullptr
        );

        if (!m_swsCtx) {
            av_frame_free(&m_srcFrame);
            m_srcFrame = nullptr;
            return false;
        }

        m_initialized = true;
    }

    // Ensure frame is writable
    av_frame_make_writable(m_srcFrame);

    // Copy data from QImage to source AVFrame
    for (int y = 0; y < image.height(); y++) {
        memcpy(m_srcFrame->data[0] + y * m_srcFrame->linesize[0],
               image.constScanLine(y),
               image.width() * 4);
    }

    // Convert pixel format to the destination frame
    int ret = sws_scale(m_swsCtx, m_srcFrame->data, m_srcFrame->linesize, 0, image.height(),
              frame->data, frame->linesize);

    return ret > 0;
}

bool FFMpeg::encodeQImageSequence(const std::vector<QImage>& images, float fps,
                                 FfmpegProgressListener& progressListener) {
    if (images.empty()) {
        std::cerr << "No images to encode" << std::endl;
        return false;
    }

    std::cout << "Starting encoding of " << images.size() << " frames..." << std::endl;
    auto startTime = std::chrono::high_resolution_clock::now();

    // Set BATCH_SIZE to min of actual batch size and images.size()
    const int BATCH_SIZE = std::min(MAX_BATCH_SIZE, static_cast<int>(images.size()));

    // Create appropriately sized frame pool
    FramePool framePool(BATCH_SIZE, m_codecContext->pix_fmt,
                       images[0].width(), images[0].height());

    for (size_t i = 0; i < images.size(); i += BATCH_SIZE) {
        size_t endIdx = std::min(i + BATCH_SIZE, images.size());
        std::vector<AVFrame*> frames(endIdx - i, nullptr);

        // Convert batch of frames in parallel
        #pragma omp parallel for if(endIdx - i > 4)
        for (size_t j = i; j < endIdx; j++) {
            // Get a frame from the pool
            AVFrame* poolFrame = framePool.getFrame();
            if (!poolFrame) {
                std::cerr << "Failed to get frame from pool for frame " << j << std::endl;
                continue;
            }

            // Copy QImage data to the frame
            if (!copyQImageToAVFrame(images[j], poolFrame)) {
                framePool.returnFrame(poolFrame);
                continue;
            }

            frames[j-i] = poolFrame;
        }

        // Send frames to encoder
        for (size_t j = 0; j < frames.size(); j++) {
            if (!frames[j]) {
                std::cerr << "Frame " << (i+j) << " is null, skipping" << std::endl;
                continue;
            }

            bool success = encodeFrame(frames[j]);

            // Return the frame to the pool
            framePool.returnFrame(frames[j]);

            if (!success) {
                std::cerr << "Failed to encode frame " << (i+j) << std::endl;
                return false;
            }

            uint64_t msEncoded = static_cast<uint64_t>(1000 * (i + j + 1) / fps);
            if (progressListener.ffmpegProgress(msEncoded)) {
                return false;
            }
        }
    }

    bool result = finalizeEncoding();

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    std::cout << "Encoded " << images.size() << " frames in " << duration << "ms ("
              << (images.size() > 0 ? duration/images.size() : 0) << "ms per frame)" << std::endl;

    return result;
}
void FFMpeg::msToTimecode(uint64_t ms, double fps, char* timecodeBuf, size_t bufSize) {
  int64_t totalSeconds = ms / 1000;
  int hours   = static_cast<int>(totalSeconds / 3600);
  int minutes = static_cast<int>((totalSeconds % 3600) / 60);
  int seconds = static_cast<int>(totalSeconds % 60);
  int frames  = static_cast<int>((ms % 1000) * fps / 1000.0);
  std::snprintf(
          timecodeBuf, bufSize,
          "%02d:%02d:%02d:%02d",
          hours, minutes, seconds, frames
  );
}

bool FFMpeg::initializeEncoder(const std::string &filename, int width, int height, float fps, uint64_t startTimeMs) {
  // Clean up any existing resources
  if (m_formatContext) {
    avformat_free_context(m_formatContext);
    m_formatContext = nullptr;
  }
  if (m_codecContext) {
    avcodec_free_context(&m_codecContext);
    m_codecContext = nullptr;
  }

  m_videoStream = nullptr;
  m_nextPts = 0;
  m_initialized = false; // Reset initialization flag for convertQImageToAVFrame

  // Create output format context for MOV (required for ProRes with alpha)
  avformat_alloc_output_context2(&m_formatContext, nullptr, "mov", filename.c_str());
  if (!m_formatContext) {
    std::cerr << "Could not create output context" << std::endl;
    return false;
  }

  // Find ProRes encoder
  const AVCodec *codec = nullptr;
  AVPixelFormat pixFmt;

  // Try hardware-accelerated ProRes encoder first
  codec = avcodec_find_encoder_by_name("prores_videotoolbox");
  if (codec) {
    std::cout << "Using hardware-accelerated ProRes encoder (VideoToolbox)" << std::endl;
    pixFmt = AV_PIX_FMT_BGRA;
  }

  // Create stream
  m_videoStream = avformat_new_stream(m_formatContext, nullptr);
  if (!m_videoStream) {
    std::cerr << "Could not create stream" << std::endl;
    return false;
  }

  // Create codec context
  m_codecContext = avcodec_alloc_context3(codec);
  if (!m_codecContext) {
    std::cerr << "Could not create codec context" << std::endl;
    return false;
  }
  int int_fps = int(fps + 0.5);

  // Set video stream parameters
  m_videoStream->time_base = m_codecContext->time_base;
  m_videoStream->avg_frame_rate = (AVRational){int_fps, 1};

  // Set codec parameters
  m_codecContext->width = width;
  m_codecContext->height = height;
  m_codecContext->time_base = (AVRational){1, int_fps};
  m_codecContext->pix_fmt = pixFmt;
  m_codecContext->thread_count = 16; // Use more threads for better performance
  m_codecContext->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE; // Use both threading models

  // Explicitly set color space and range for BGRA
  m_codecContext->color_range = AVCOL_RANGE_JPEG;    // Full range (0-255)
  m_codecContext->colorspace = AVCOL_SPC_RGB;        // RGB color space
  m_codecContext->color_primaries = AVCOL_PRI_BT709; // Standard color primaries
  m_codecContext->color_trc = AVCOL_TRC_IEC61966_2_1; // sRGB transfer characteristics

  av_opt_set_int(m_codecContext->priv_data, "profile", 4, 0); // Use profile 4 (ProRes 4444)
  av_opt_set_int(m_codecContext->priv_data, "allow_hw_accel", 1, 0);
  av_opt_set_int(m_codecContext->priv_data, "realtime", 1, 0); // Prioritize speed
  m_codecContext->flags |= AV_CODEC_FLAG_LOW_DELAY; // Lower latency

  // Open codec
  int ret = avcodec_open2(m_codecContext, codec, nullptr);

  if (ret < 0) {
    return false;
  }

  // Copy parameters to stream
  avcodec_parameters_from_context(m_videoStream->codecpar, m_codecContext);

  // Open output file
  if (!(m_formatContext->oformat->flags & AVFMT_NOFILE)) {
    if (avio_open(&m_formatContext->pb, filename.c_str(), AVIO_FLAG_WRITE) < 0) {
      std::cerr << "Could not open output file" << std::endl;
      return false;
    }
  }

  startTimeMs = startTimeMs % (24 * 60*60*1000); // Wrap around to stay within 24 hours
  char timecode[12];
  FFMpeg::msToTimecode(startTimeMs, fps, timecode, sizeof(timecode));

  if (av_dict_set(&m_videoStream->metadata, "timecode", timecode, 0) < 0) {
    std::cerr << "Failed to set timecode metadata on videostream !" << std::endl;
    avcodec_free_context(&m_codecContext);
    avio_close(m_formatContext->pb);
    avformat_free_context(m_formatContext);
    return false;
  }
  if (av_dict_set(&m_formatContext->metadata, "timecode", timecode, 0) < 0) {
    std::cerr << "Failed to set timecode metadata on videostream !" << std::endl;
    avcodec_free_context(&m_codecContext);
    avio_close(m_formatContext->pb);
    avformat_free_context(m_formatContext);
    return false;
  }


  // Write header
  if (avformat_write_header(m_formatContext, nullptr) < 0) {
    std::cerr << "Could not write header" << std::endl;
    return false;
  }

  std::cout << "Dumping format context after writing header:" << std::endl;
  av_dump_format(m_formatContext, 0, "output_path", 1);

  return true;
}

