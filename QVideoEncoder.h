#pragma once

#include <QIODevice>
#include <QFile>
#include <QImage>

class AVFrame;
class AVCodecContext;
class SwsContext;
class AVFormatContext;
class AVOutputFormat;

class QVideoEncoder {
    std::string m_FileName;
    AVFrame* m_videoFrame = nullptr;
    AVCodecContext* m_cctx = nullptr;
    SwsContext* m_swsCtx = nullptr;
    int m_frameCounter = 0;
    AVFormatContext* m_ofctx = nullptr;
    AVOutputFormat* m_oformat = nullptr;
    bool m_IsOk;

    void finish();

public:
    QVideoEncoder(std::string fileName, int width, int height, int bitrate, int fps);
    ~QVideoEncoder();

    bool isOk() const {
        return m_IsOk;
    }

    void pushFrame(uint8_t* data);
};
