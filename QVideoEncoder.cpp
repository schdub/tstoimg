#include "QVideoEncoder.h"

#include <QDebug>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/avutil.h>
    #include <libavutil/time.h>
    #include <libavutil/opt.h>
    #include <libswscale/swscale.h>
}

QVideoEncoder::QVideoEncoder(std::string fileName, int width, int height, int bitrate, int fps)
    : m_FileName(std::move(fileName))
    , m_IsOk(false)
{
    m_oformat = av_guess_format(nullptr, m_FileName.data(), nullptr);
    if (!m_oformat) {
        qDebug() << "can't create output format";
        return;
    }
    //m_oformat->video_codec = AV_CODEC_ID_H265;

    int err = avformat_alloc_output_context2(&m_ofctx, m_oformat, nullptr, m_FileName.data());
    if (err) {
        qDebug() << "can't create output context";
        return;
    }

    AVCodec* codec = avcodec_find_encoder(m_oformat->video_codec);
    if (!codec) {
        qDebug() << "can't create codec";
        return;
    }

    AVStream* stream = avformat_new_stream(m_ofctx, codec);
    if (!stream) {
        qDebug() << "can't find format";
        return;
    }

    m_cctx = avcodec_alloc_context3(codec);
    if (!m_cctx) {
        qDebug() << "can't create codec context";
        return;
    }

    stream->codecpar->codec_id = m_oformat->video_codec;
    stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    stream->codecpar->width = width;
    stream->codecpar->height = height;
    stream->codecpar->format = AV_PIX_FMT_YUV420P;
    stream->codecpar->bit_rate = bitrate * 1000;
    avcodec_parameters_to_context(m_cctx, stream->codecpar);
    m_cctx->time_base = (AVRational){ 1, 1 };
    m_cctx->max_b_frames = 2;
    m_cctx->gop_size = 12;
    m_cctx->framerate = (AVRational){ fps, 1 };
    //must remove the following
    //m_cctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    if (stream->codecpar->codec_id == AV_CODEC_ID_H264) {
        av_opt_set(m_cctx, "preset", "ultrafast", 0);
    } else if (stream->codecpar->codec_id == AV_CODEC_ID_H265) {
        av_opt_set(m_cctx, "preset", "ultrafast", 0);
    }
    avcodec_parameters_from_context(stream->codecpar, m_cctx);
    if ((err = avcodec_open2(m_cctx, codec, NULL)) < 0) {
        qDebug() << "Failed to open codec" << err;
        return;
    }
    if (!(m_oformat->flags & AVFMT_NOFILE)) {
        if ((err = avio_open(&m_ofctx->pb, m_FileName.data(), AVIO_FLAG_WRITE)) < 0) {
            qDebug() << "Failed to open file" << err;
        }
    }
    if ((err = avformat_write_header(m_ofctx, NULL)) < 0) {
        qDebug() << "Failed to write header" << err;
    }
    av_dump_format(m_ofctx, 0, m_FileName.data(), 1);
    m_IsOk = true;
}

QVideoEncoder::~QVideoEncoder() {
    finish();
    if (m_videoFrame) {
        av_frame_free(&m_videoFrame);
    }
    if (m_cctx) {
        avcodec_free_context(&m_cctx);
    }
    if (m_ofctx) {
        avformat_free_context(m_ofctx);
    }
    if (m_swsCtx) {
        sws_freeContext(m_swsCtx);
    }
}

void QVideoEncoder::pushFrame(uint8_t* data) {
    int err;
    if (!m_videoFrame) {
        m_videoFrame = av_frame_alloc();
        m_videoFrame->format = AV_PIX_FMT_YUV420P;
        m_videoFrame->width = m_cctx->width;
        m_videoFrame->height = m_cctx->height;
        if ((err = av_frame_get_buffer(m_videoFrame, 32)) < 0) {
            qDebug() << "Failed to allocate picture" << err;
            return;
        }
    }
    if (!m_swsCtx) {
        m_swsCtx = sws_getContext(m_cctx->width, m_cctx->height, AV_PIX_FMT_RGB24, m_cctx->width,
            m_cctx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, 0, 0, 0);
    }

    int inLinesize[1] = { 3 * m_cctx->width };
    // From RGB to YUV
    sws_scale(m_swsCtx, (const uint8_t* const*)&data, inLinesize, 0, m_cctx->height,
        m_videoFrame->data, m_videoFrame->linesize);
    m_videoFrame->pts = (1.0 / 30.0) * 90000 * (m_frameCounter++);
    qDebug() << m_videoFrame->pts
             << m_cctx->time_base.num
             << m_cctx->time_base.den
             << m_frameCounter;
    if ((err = avcodec_send_frame(m_cctx, m_videoFrame)) < 0) {
        qDebug() << "Failed to send frame" << err;
        return;
    }
    AV_TIME_BASE;
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;
    pkt.flags |= AV_PKT_FLAG_KEY;
    if (avcodec_receive_packet(m_cctx, &pkt) == 0) {
        static int counter = 0;
#if 0
        if (counter == 0) {
            FILE* fp = fopen("dump_first_frame1.dat", "wb");
            fwrite(pkt.data, pkt.size, 1, fp);
            fclose(fp);
        }
#endif
        qDebug() << "pkt key:" << (pkt.flags & AV_PKT_FLAG_KEY)
                 << pkt.size   << (counter++);
        uint8_t* size = ((uint8_t*)pkt.data);
        qDebug() << "first:"
                 << (int)size[0] << (int)size[1]
                 << (int)size[2] << (int)size[3]
                 << (int)size[4] << (int)size[5]
                 << (int)size[6] << (int)size[7];
        av_interleaved_write_frame(m_ofctx, &pkt);
        av_packet_unref(&pkt);
    }
}

void QVideoEncoder::finish() {
    if (!isOk()) {
        return;
    }

    //DELAYED FRAMES
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    for (;;) {
        avcodec_send_frame(m_cctx, NULL);
        if (avcodec_receive_packet(m_cctx, &pkt) == 0) {
            av_interleaved_write_frame(m_ofctx, &pkt);
            av_packet_unref(&pkt);
        }
        else {
            break;
        }
    }

    av_write_trailer(m_ofctx);
    if (!(m_oformat->flags & AVFMT_NOFILE)) {
        int err = avio_close(m_ofctx->pb);
        if (err < 0) {
            qDebug() << "Failed to close file" << err;
        }
    }
}
