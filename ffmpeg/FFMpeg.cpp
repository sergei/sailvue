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

bool FFMpeg::initializeEncoder(const std::string &filename, int width, int height, float fps, bool useAlpha = false) {
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

    // Create output format context - use QuickTime container for ProRes
    const char* format_name = useAlpha ? "mov" : nullptr;
    avformat_alloc_output_context2(&m_formatContext, nullptr, format_name, filename.c_str());
    if (!m_formatContext) {
        std::cerr << "Could not create output context" << std::endl;
        return false;
    }

    // Find encoder
    const AVCodec *codec;
    if (useAlpha) {
        codec = avcodec_find_encoder_by_name("prores_ks");
        if (!codec) {
            std::cerr << "ProRes codec not found" << std::endl;
            return false;
        }
    } else {
        codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        if (!codec) {
            std::cerr << "H.264 codec not found" << std::endl;
            return false;
        }
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

    if (useAlpha) {
        // ProRes 4444 settings for alpha support
        m_codecContext->pix_fmt = AV_PIX_FMT_YUVA444P10LE;
        av_opt_set(m_codecContext->priv_data, "profile", "4444", 0);
        // Higher quality settings
        av_opt_set(m_codecContext->priv_data, "bits_per_mb", "8000", 0);
    } else {
        m_codecContext->pix_fmt = AV_PIX_FMT_YUV420P;
        m_codecContext->gop_size = 12;
        m_codecContext->max_b_frames = 2;
        av_opt_set(m_codecContext->priv_data, "preset", "medium", 0);
    }

    // Open codec
    int ret = avcodec_open2(m_codecContext, codec, nullptr);
    if (ret < 0) {
        char errBuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errBuf, AV_ERROR_MAX_STRING_SIZE);
        std::cerr << "Could not open codec: " << errBuf << std::endl;
        return false;
    }

    // Copy parameters from codec context to stream
    avcodec_parameters_from_context(m_stream->codecpar, m_codecContext);

    // Open output file
    if (!(m_formatContext->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&m_formatContext->pb, filename.c_str(), AVIO_FLAG_WRITE) < 0) {
            std::cerr << "Could not open output file" << std::endl;
            return false;
        }
    }

    // Write header
    if (avformat_write_header(m_formatContext, nullptr) < 0) {
        std::cerr << "Could not write header" << std::endl;
        return false;
    }

    return true;
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

    // Check if frame has valid linesize
    bool validLinesize = true;
    for (int i = 0; i < AV_NUM_DATA_POINTERS && frame->data[i]; i++) {
        if (frame->linesize[i] <= 0) {
            validLinesize = false;
            break;
        }
    }

    // If linesize is invalid, create a new frame with correct linesize
    AVFrame* frameToEncode = frame;
    if (!validLinesize) {
        // Create a new frame with correct linesize
        AVFrame* newFrame = av_frame_alloc();
        newFrame->format = frame->format;
        newFrame->width = frame->width;
        newFrame->height = frame->height;

        // Allocate buffer for the new frame
        int ret = av_frame_get_buffer(newFrame, 32); // 32 for alignment
        if (ret < 0) {
            std::cerr << "Could not allocate frame data" << std::endl;
            av_frame_free(&newFrame);
            return false;
        }

        // Make the frame writable
        ret = av_frame_make_writable(newFrame);
        if (ret < 0) {
            std::cerr << "Could not make frame writable" << std::endl;
            av_frame_free(&newFrame);
            return false;
        }

        // Manually copy the pixel data
        for (int i = 0; i < AV_NUM_DATA_POINTERS && frame->data[i]; i++) {
            if (newFrame->data[i]) {
                // Calculate plane size based on format
                int planeHeight = (i == 0) ? frame->height : frame->height / 2;
                int planeWidth = (i == 0) ? frame->width : frame->width / 2;

                // Copy line by line
                for (int h = 0; h < planeHeight; h++) {
                    memcpy(newFrame->data[i] + h * newFrame->linesize[i],
                           frame->data[i] + h * (frame->linesize[i] > 0 ? frame->linesize[i] : planeWidth),
                           planeWidth);
                }
            }
        }

        frameToEncode = newFrame;
    }

    // Set frame PTS
    frameToEncode->pts = m_nextPts++;

    // Send frame to encoder
    int ret = avcodec_send_frame(m_codecContext, frameToEncode);

    // Free the new frame if we created one
    if (frameToEncode != frame) {
        av_frame_free(&frameToEncode);
    }

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

    // Write trailer
    ret = av_write_trailer(m_formatContext);
    if (ret < 0) {
        std::cerr << "Error writing trailer" << std::endl;
        return false;
    }

    // Close file
    avio_closep(&m_formatContext->pb);

    // Free resources
    avcodec_free_context(&m_codecContext);
    avformat_free_context(m_formatContext);

    m_codecContext = nullptr;
    m_formatContext = nullptr;
    m_stream = nullptr;
    m_nextPts = 0;

    return true;
}


AVFrame* FFMpeg::convertQImageToAVFrame(const QImage& image) {
    // First create a frame in the source format
    AVFrame* srcFrame = av_frame_alloc();
    if (!srcFrame) {
        return nullptr;
    }

    // Create a frame in the destination format
    AVFrame* dstFrame = av_frame_alloc();
    if (!dstFrame) {
        av_frame_free(&srcFrame);
        return nullptr;
    }

    // Set source frame properties
    srcFrame->width = image.width();
    srcFrame->height = image.height();
    srcFrame->format = AV_PIX_FMT_BGRA; // QImage::Format_ARGB32 is actually BGRA in memory

    // Set destination frame properties based on codec context
    dstFrame->width = image.width();
    dstFrame->height = image.height();
    dstFrame->format = m_codecContext->pix_fmt;

    // Allocate source frame buffer
    if (av_frame_get_buffer(srcFrame, 0) < 0) {
        av_frame_free(&srcFrame);
        av_frame_free(&dstFrame);
        return nullptr;
    }

    // Allocate destination frame buffer
    if (av_frame_get_buffer(dstFrame, 0) < 0) {
        av_frame_free(&srcFrame);
        av_frame_free(&dstFrame);
        return nullptr;
    }

    // Make source frame writable
    if (av_frame_make_writable(srcFrame) < 0) {
        av_frame_free(&srcFrame);
        av_frame_free(&dstFrame);
        return nullptr;
    }

    // Copy data from QImage to source AVFrame
    for (int y = 0; y < srcFrame->height; y++) {
        memcpy(srcFrame->data[0] + y * srcFrame->linesize[0],
               image.constScanLine(y),
               srcFrame->width * 4);
    }

    // Create SwsContext for pixel format conversion
    SwsContext* swsCtx = sws_getContext(
        srcFrame->width, srcFrame->height, (AVPixelFormat)srcFrame->format,
        dstFrame->width, dstFrame->height, (AVPixelFormat)dstFrame->format,
        SWS_BICUBIC, nullptr, nullptr, nullptr
    );

    if (!swsCtx) {
        av_frame_free(&srcFrame);
        av_frame_free(&dstFrame);
        return nullptr;
    }

    // Set proper colorspace conversion parameters
    int srcRange = 1; // Full range source
    int dstRange = 1; // Full range destination
    sws_setColorspaceDetails(
        swsCtx,
        sws_getCoefficients(SWS_CS_DEFAULT), srcRange,
        sws_getCoefficients(SWS_CS_ITU709), dstRange,
        0, 1 << 16, 1 << 16
    );

    // Convert pixel format
    sws_scale(swsCtx, srcFrame->data, srcFrame->linesize, 0, srcFrame->height,
              dstFrame->data, dstFrame->linesize);

    // Free SwsContext
    sws_freeContext(swsCtx);

    // Free source frame
    av_frame_free(&srcFrame);

    return dstFrame;
}

bool FFMpeg::encodeQImageSequence(const std::vector<QImage>& images, float fps,
                                 FfmpegProgressListener& progressListener) {
    if (!m_codecContext || !m_formatContext) {
        std::cerr << "Encoder not initialized" << std::endl;
        return false;
    }

    uint64_t totalFrames = images.size();

    for (size_t i = 0; i < images.size(); i++) {
        // Convert QImage to AVFrame
        AVFrame* frame = convertQImageToAVFrame(images[i]);
        if (!frame) {
            std::cerr << "Failed to convert QImage to AVFrame" << std::endl;
            return false;
        }

        // Encode the frame
        bool success = encodeFrame(frame);

        // Free the frame
        av_frame_free(&frame);

        if (!success) {
            std::cerr << "Failed to encode frame " << i << std::endl;
            return false;
        }

        // Report progress
        uint64_t msEncoded = static_cast<uint64_t>(1000 * (i + 1) / fps);
        if (progressListener.ffmpegProgress(msEncoded)) {
            return false; // Stop requested
        }
    }

    // Finalize encoding
    return finalizeEncoding();
}