#include <iostream>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
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

int main(){
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

  if (av_dict_set(&videoStream->metadata, "timecode", "00:00:30:00", 0) < 0) {
    std::cerr << "Failed to set timecode metadata on videostream !" << std::endl;
    avcodec_free_context(&videoCodecContext);
    avio_close(formatContext->pb);
    avformat_free_context(formatContext);
    return -1;
  }
  if (av_dict_set(&formatContext->metadata, "timecode", "00:00:30:00", 0) < 0) {
    std::cerr << "Failed to set timecode metadata on videostream !" << std::endl;
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

  // Write trailer
  if (av_write_trailer(formatContext) < 0) {
    std::cerr << "Failed to write file trailer!" << std::endl;
  }

  av_dump_format(formatContext, 0, "test.mov", 1);

  // Cleanup
  avcodec_free_context(&videoCodecContext);
  avio_close(formatContext->pb);
  avformat_free_context(formatContext);

  std::cout << "Test file with timecode created successfully: " << outputFileName << std::endl;

  return 0;
}