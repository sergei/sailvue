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
        av_packet_rescale_ts(packet, m_codecContext->time_base, m_stream->time_base);
        packet->stream_index = m_stream->index;

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
        av_packet_rescale_ts(packet, m_codecContext->time_base, m_stream->time_base);
        packet->stream_index = m_stream->index;

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
    m_stream = nullptr;
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

bool FFMpeg::initializeEncoder(const std::string &filename, int width, int height, float fps) {
  // Clean up any existing resources
  if (m_formatContext) {
    avformat_free_context(m_formatContext);
    m_formatContext = nullptr;
  }
  if (m_codecContext) {
    avcodec_free_context(&m_codecContext);
    m_codecContext = nullptr;
  }

  m_stream = nullptr;
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
  } else {
    // Fall back to software ProRes encoder
    codec = avcodec_find_encoder_by_name("prores_ks");
    if (!codec) {
      std::cerr << "ProRes codec not found" << std::endl;
      return false;
    }
    std::cout << "Using software ProRes encoder" << std::endl;
    pixFmt = AV_PIX_FMT_YUVA444P10LE;
  }

  // Create stream
  m_stream = avformat_new_stream(m_formatContext, nullptr);
  if (!m_stream) {
    std::cerr << "Could not create stream" << std::endl;
    return false;
  }

  // Create codec context
  m_codecContext = avcodec_alloc_context3(codec);
  if (!m_codecContext) {
    std::cerr << "Could not create codec context" << std::endl;
    return false;
  }

  // Set codec parameters
  m_codecContext->width = width;
  m_codecContext->height = height;
  m_codecContext->time_base = (AVRational){1, int(fps + 0.5)};
  m_stream->time_base = m_codecContext->time_base;
  m_codecContext->pix_fmt = pixFmt;
  m_codecContext->thread_count = 16; // Use more threads for better performance
  m_codecContext->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE; // Use both threading models

  // ProRes settings
  if (std::string(codec->name) == "prores_videotoolbox") {

    // Hardware ProRes settings for VideoToolbox
    // Explicitly set color space and range for BGRA
    m_codecContext->color_range = AVCOL_RANGE_JPEG;    // Full range (0-255)
    m_codecContext->colorspace = AVCOL_SPC_RGB;        // RGB color space
    m_codecContext->color_primaries = AVCOL_PRI_BT709; // Standard color primaries
    m_codecContext->color_trc = AVCOL_TRC_IEC61966_2_1; // sRGB transfer characteristics

    av_opt_set_int(m_codecContext->priv_data, "profile", 4, 0); // Use profile 4 (ProRes 4444)
    av_opt_set_int(m_codecContext->priv_data, "allow_hw_accel", 1, 0);
    av_opt_set_int(m_codecContext->priv_data, "realtime", 1, 0); // Prioritize speed
    m_codecContext->flags |= AV_CODEC_FLAG_LOW_DELAY; // Lower latency
  } else {
    // Software ProRes settings
    av_opt_set(m_codecContext->priv_data, "profile", "4444", 0);
    av_opt_set(m_codecContext->priv_data, "bits_per_mb", "8000", 0);
    av_opt_set(m_codecContext->priv_data, "qscale", "11", 0); // Higher value = lower quality but faster
  }

  // Open codec
  int ret = avcodec_open2(m_codecContext, codec, nullptr);
  if (ret < 0) {
    char errBuf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(ret, errBuf, AV_ERROR_MAX_STRING_SIZE);
    std::cerr << "Could not open codec: " << errBuf << std::endl;

    // If hardware encoding failed, try falling back to software
    if (std::string(codec->name) == "prores_videotoolbox") {
      std::cout << "Hardware ProRes encoding failed, falling back to software..." << std::endl;
      avcodec_free_context(&m_codecContext);

      codec = avcodec_find_encoder_by_name("prores_ks");
      if (!codec) {
        std::cerr << "ProRes software codec not found" << std::endl;
        return false;
      }

      m_codecContext = avcodec_alloc_context3(codec);
      if (!m_codecContext) {
        std::cerr << "Could not create codec context" << std::endl;
        return false;
      }

      m_codecContext->width = width;
      m_codecContext->height = height;
      m_codecContext->time_base = (AVRational){1, int(fps + 0.5)};
      m_codecContext->pix_fmt = AV_PIX_FMT_YUVA444P10LE;
      m_codecContext->thread_count = 16;
      m_codecContext->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;

      av_opt_set(m_codecContext->priv_data, "profile", "4444", 0);
      av_opt_set(m_codecContext->priv_data, "bits_per_mb", "8000", 0);
      av_opt_set(m_codecContext->priv_data, "qscale", "11", 0);

      ret = avcodec_open2(m_codecContext, codec, nullptr);
      if (ret < 0) {
        av_strerror(ret, errBuf, AV_ERROR_MAX_STRING_SIZE);
        std::cerr << "Could not open software codec: " << errBuf << std::endl;
        return false;
      }
    } else {
      return false;
    }
  }

  // Copy parameters to stream
  avcodec_parameters_from_context(m_stream->codecpar, m_codecContext);

  // Open output file
  if (!(m_formatContext->oformat->flags & AVFMT_NOFILE)) {
    if (avio_open(&m_formatContext->pb, filename.c_str(), AVIO_FLAG_WRITE) < 0) {
      std::cerr << "Could not open output file" << std::endl;
      return false;
    }
  }

  uint64_t startTimeMs = 30 * 1000; // Default start time in milliseconds (30 seconds)

  // Add timecode stream
  if (!addTimecodeStream(fps, startTimeMs)) {
    std::cerr << "Failed to add timecode stream" << std::endl;
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

bool FFMpeg::addTimecodeStream(double fps, uint64_t startTimeMs) {
  // Create timecode stream
  AVStream* timecodeStream = avformat_new_stream(m_formatContext, nullptr);
  if (!timecodeStream) {
    std::cerr << "Failed to create timecode stream" << std::endl;
    return false;
  }

  // Assign unique stream ID and configure codec parameters
  timecodeStream->id = m_formatContext->nb_streams - 1;
  timecodeStream->codecpar->codec_type = AVMEDIA_TYPE_DATA;
  timecodeStream->codecpar->codec_id = AV_CODEC_ID_NONE;
  timecodeStream->codecpar->codec_tag = 0x64636D74;
  timecodeStream->codecpar->format = 0; // Not compressed
  timecodeStream->codecpar->extradata = nullptr;
  timecodeStream->codecpar->extradata_size = 0;

  // Timecode uses the same time_base as the frame rate
  AVRational fpsTimeBase = (AVRational){1, static_cast<int>(fps)};
  timecodeStream->time_base = fpsTimeBase;

  // Prepare the timecode string in HH:MM:SS:FF format
  uint64_t totalSeconds = startTimeMs / 1000;
  uint64_t frames = (startTimeMs % 1000) * fps / 1000;
  uint64_t hours = totalSeconds / 3600;
  uint64_t minutes = (totalSeconds / 60) % 60;
  uint64_t seconds = totalSeconds % 60;

  char timecodeString[16];
  snprintf(timecodeString, sizeof(timecodeString), "%02llu:%02llu:%02llu:%02llu",
           hours, minutes, seconds, frames);

  // Attach timecode metadata to the stream
  if ( av_opt_set_int(m_formatContext->priv_data, "write_tmcd", 1, 0) ){
    std::cerr << "Failed to set write_tmcd option for timecode stream" << std::endl;
    return false;
  }
  if ( av_dict_set(&timecodeStream->metadata, "timecode", timecodeString, 0)  < 0) {
    std::cerr << "Failed to set timecode metadata" << std::endl;
    return false;
  }

  std::cout << "Added timecode stream with timecode: " << timecodeString << std::endl;
  std::cout << "tmcd stream created successfully!" << std::endl;
  std::cout << "Timecode stream codec parameters:" << std::endl;
  std::cout << "  codec_type: " << timecodeStream->codecpar->codec_type << std::endl;
  std::cout << "  codec_id: " << timecodeStream->codecpar->codec_id << std::endl;
  std::cout << "  extradata: " << (timecodeStream->codecpar->extradata ? "set" : "not set") << std::endl;
  std::cout << "  extradata_size: " << timecodeStream->codecpar->extradata_size << std::endl;
  return true;
}

int test_tmcd2() {
  AVFormatContext* fmt_ctx;

  avformat_alloc_output_context2(&fmt_ctx, nullptr, "mov", "test.mov");
  if (!fmt_ctx) {
    std::cerr << "Failed to allocate output context!" << std::endl;
    return 1;
  }

  AVStream* timecodeStream = avformat_new_stream(fmt_ctx, nullptr);
  if (!timecodeStream) {
    std::cerr << "Failed to create timecode stream!" << std::endl;
    return 1;
  }

  timecodeStream->codecpar->codec_type = AVMEDIA_TYPE_DATA;
  timecodeStream->codecpar->codec_id = AV_CODEC_ID_NONE;
  timecodeStream->codecpar->codec_tag = 0x64636D74;
  av_dump_format(fmt_ctx, 0, "test.mov", 1);

  return 0;
}


#include <iostream>
extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
}

bool checkProResAvailability() {
  const AVCodec* codec = avcodec_find_encoder_by_name("prores_ks");
  if (!codec) {
    std::cerr << "ProRes codec not available. Please install FFmpeg with ProRes support." << std::endl;
    return false;
  }
  return true;
}

int test_tmcd() {
  av_log_set_level(AV_LOG_INFO);

  const char* outputFileName = "test_tmcd.mov";
  AVFormatContext* formatContext = nullptr;
  AVCodecContext* videoCodecContext = nullptr;

  if (!checkProResAvailability()) {
    return -1;
  }

  std::cout << "Creating test file with tmcd stream: " << outputFileName << std::endl;

  // Allocate the output format context
  if (avformat_alloc_output_context2(&formatContext, nullptr, "mov", outputFileName) < 0) {
    std::cerr << "Failed to allocate output context!" << std::endl;
    return -1;
  }

  if (avio_open(&formatContext->pb, outputFileName, AVIO_FLAG_WRITE) < 0) {
    std::cerr << "Failed to open output file!" << std::endl;
    avformat_free_context(formatContext);
    return -1;
  }

  // Find ProRes encoder
  const AVCodec* videoCodec = avcodec_find_encoder_by_name("prores_ks");
  if (!videoCodec) {
    std::cerr << "Failed to find the ProRes encoder!" << std::endl;
    avio_close(formatContext->pb);
    avformat_free_context(formatContext);
    return -1;
  }

  // Video stream setup
  AVStream* videoStream = avformat_new_stream(formatContext, nullptr);
  if (!videoStream) {
    std::cerr << "Failed to create video stream!" << std::endl;
    avio_close(formatContext->pb);
    avformat_free_context(formatContext);
    return -1;
  }

  videoCodecContext = avcodec_alloc_context3(videoCodec);
  if (!videoCodecContext) {
    std::cerr << "Failed to allocate video codec context!" << std::endl;
    avio_close(formatContext->pb);
    avformat_free_context(formatContext);
    return -1;
  }

  videoCodecContext->width = 1920;
  videoCodecContext->height = 1080;
  videoCodecContext->pix_fmt = AV_PIX_FMT_YUV422P10;
  videoCodecContext->time_base = (AVRational){1, 30}; // Set FPS: 30
  videoCodecContext->bit_rate = 2000000;

  if (avcodec_open2(videoCodecContext, videoCodec, nullptr) < 0) {
    std::cerr << "Failed to open ProRes codec!" << std::endl;
    avcodec_free_context(&videoCodecContext);
    avio_close(formatContext->pb);
    avformat_free_context(formatContext);
    return -1;
  }

  if (avcodec_parameters_from_context(videoStream->codecpar, videoCodecContext) < 0) {
    std::cerr << "Failed to copy codec parameters to video stream!" << std::endl;
    avcodec_free_context(&videoCodecContext);
    avio_close(formatContext->pb);
    avformat_free_context(formatContext);
    return -1;
  }

  videoStream->time_base = videoCodecContext->time_base;

  // Timecode stream setup
  AVStream* timecodeStream = avformat_new_stream(formatContext, nullptr);
  if (!timecodeStream) {
    std::cerr << "Failed to create timecode stream!" << std::endl;
    avcodec_free_context(&videoCodecContext);
    avio_close(formatContext->pb);
    avformat_free_context(formatContext);
    return -1;
  }

  timecodeStream->codecpar->codec_type = AVMEDIA_TYPE_DATA;
  timecodeStream->codecpar->codec_id = AV_CODEC_ID_TIMED_ID3;
  timecodeStream->codecpar->codec_tag = MKTAG('t', 'm', 'c', 'd'); // Timecode tag
  timecodeStream->time_base = (AVRational){1, 30}; // FPS: 30

  if (av_dict_set(&timecodeStream->metadata, "timecode", "00:00:30:00", 0) < 0) {
    std::cerr << "Failed to set timecode metadata!" << std::endl;
    avcodec_free_context(&videoCodecContext);
    avio_close(formatContext->pb);
    avformat_free_context(formatContext);
    return -1;
  }

  // Write container header
  if (avformat_write_header(formatContext, nullptr) < 0) {
    std::cerr << "Failed to write file header!" << std::endl;
    avcodec_free_context(&videoCodecContext);
    avio_close(formatContext->pb);
    avformat_free_context(formatContext);
    return -1;
  }

  // Encode a dummy video frame
  AVFrame* frame = av_frame_alloc();
  if (!frame) {
    std::cerr << "Failed to allocate video frame!" << std::endl;
    avcodec_free_context(&videoCodecContext);
    avio_close(formatContext->pb);
    avformat_free_context(formatContext);
    return -1;
  }

  frame->format = videoCodecContext->pix_fmt;
  frame->width = videoCodecContext->width;
  frame->height = videoCodecContext->height;

  if (av_image_alloc(frame->data, frame->linesize, frame->width, frame->height, videoCodecContext->pix_fmt, 32) < 0) {
    std::cerr << "Failed to allocate frame buffer!" << std::endl;
    av_frame_free(&frame);
    avcodec_free_context(&videoCodecContext);
    avio_close(formatContext->pb);
    avformat_free_context(formatContext);
    return -1;
  }

  // Fill frame with black
  memset(frame->data[0], 0, frame->linesize[0] * frame->height); // Y plane
  memset(frame->data[1], 128, frame->linesize[1] * frame->height / 2); // U plane
  memset(frame->data[2], 128, frame->linesize[2] * frame->height / 2); // V plane

  // Encode the frame
  AVPacket packet;
  av_init_packet(&packet);
  packet.data = nullptr;
  packet.size = 0;

  if (avcodec_send_frame(videoCodecContext, frame) == 0) {
    if (avcodec_receive_packet(videoCodecContext, &packet) == 0) {
      packet.stream_index = videoStream->index;
      av_interleaved_write_frame(formatContext, &packet);
      av_packet_unref(&packet);
    }
  }

  av_frame_free(&frame);

  // Write a dummy packet for the timecode stream
  AVPacket tmcdPacket;
  av_init_packet(&tmcdPacket);
  tmcdPacket.stream_index = timecodeStream->index;
  tmcdPacket.flags |= AV_PKT_FLAG_KEY;
  tmcdPacket.data = nullptr; // Empty packet for timecode
  tmcdPacket.size = 0;
  tmcdPacket.pts = 0; // Set necessary PTS
  tmcdPacket.dts = 0;
  av_interleaved_write_frame(formatContext, &tmcdPacket);

  // Write trailer
  if (av_write_trailer(formatContext) < 0) {
    std::cerr << "Failed to write file trailer!" << std::endl;
  }

  // Cleanup
  avcodec_free_context(&videoCodecContext);
  avio_close(formatContext->pb);
  avformat_free_context(formatContext);

  std::cout << "Test file with timecode created successfully: " << outputFileName << std::endl;

  return 0;
}