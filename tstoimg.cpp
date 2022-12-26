// ///////////////////////////////////////////////////////////////////////// //
//                                                                           //
//   Copyright (C) 2014-2022 by Oleg Polivets                                //
//   jsbot@ya.ru                                                             //
//                                                                           //
//   This program is free software; you can redistribute it and/or modify    //
//   it under the terms of the GNU General Public License as published by    //
//   the Free Software Foundation; either version 2 of the License, or       //
//   (at your option) any later version.                                     //
//                                                                           //
//   This program is distributed in the hope that it will be useful,         //
//   but WITHOUT ANY WARRANTY; without even the implied warranty of          //
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           //
//   GNU General Public License for more details.                            //
//                                                                           //
// ///////////////////////////////////////////////////////////////////////// //

#include <QCoreApplication>
#include <QImage>
#include <QFile>
#include <QDebug>
#include <iostream>

#include "rgbfile.h"
#include "bsv3file.h"
#include "bcellfile.h"

#include <QPainter>
#include <math.h>

typedef bool (*convertFunction)(const char * path);

#if (DISABLE_VIDEO_OUTPUT != 1)

#include "QVideoEncoder.h"

bool dumpFramesIntoVideo(const QString & outFilePath, const QVector<QImage> & frames) {
    int width=0;
    int height=0;
    int bitrate = 2000;
    int fps = 25;

    foreach (const QImage & img, frames) {
        Q_ASSERT(!img.isNull());
        if (img.width() > width) width = img.width();
        if (img.height() > height) height = img.height();
    }

    while (width % 8 != 0) ++width;
    while (height % 8 != 0) ++height;

    QVideoEncoder encoder(outFilePath.toStdString(), width, height, bitrate, fps);
    QImage frame(width,height,QImage::Format_RGB888);
    QPainter painter(&frame);
//    Q_ASSERT(encoder.isOk());
    if (!encoder.isOk()) {
        return false;
    }
    foreach (const QImage & img, frames) {
        painter.fillRect(frame.rect(), QColor(102, 187, 102));
        painter.drawImage(0, 0, img);
        encoder.pushFrame(frame.bits());
    }
    return true;
}

#endif

bool rgbToPng(const char * fp) {
    QString filePath(fp);
    op::RGBFile rgb(filePath);
    QImage * img = rgb.image();
    bool ok = (img != NULL);
    if (!ok) {
        qWarning() << "ERR: can't load '" << fp << "'.";
    } else {
        filePath += ".png";
        ok = img->save(filePath, "PNG");
        if (!ok) {
            qWarning() << "ERR: can't save '" << fp << "'.";
        }
    }
    return ok;
}

bool pngToRgb(const char * fp) {
    QString filePath(fp);
    return op::RGBFile::convertTo(filePath, filePath + ".rgb");
}

bool bsv3ToPng(const char * fp) {
    QString filePath(fp);
    filePath.remove(filePath.length() - 5, filePath.length());
    op::RGBFile rgb(filePath + ".rgb");
    QImage * img = rgb.image();
    if (!img) {
        qWarning() << "ERR: can't load RGB '" << fp << "'.";
        return false;
    }
    filePath = QString(fp);
    QFile ffp(filePath);
    op::Bsv3File file(ffp, *img);
    const QImage & bsvImage = file.transImage();
    if (bsvImage.isNull()) return false;
    return bsvImage.save(filePath + ".png");
}

bool bsv3ToMp4(const char * fp) {
#if (DISABLE_VIDEO_OUTPUT == 1)
    Q_UNUSED(fp);
    qWarning() << "ERR: video generation is disabled";
#else
    QString filePath(fp);
    filePath.remove(filePath.length() - 5, filePath.length());
    op::RGBFile rgb(filePath + ".rgb");
    QImage * img = rgb.image();
    if (!img) {
        qWarning() << "ERR: can't load RGB '" << fp << "'.";
        return false;
    }
    filePath = QString(fp);
    QFile ffp(filePath);
    op::Bsv3File bsvFile(ffp, *img);
    bool ok = true;
    foreach (const QString & name, bsvFile.animationNames()) {
        const QVector<QImage> & frames = bsvFile.getFrames(name);
        if (frames.count() <= 1) continue;
        ok &= dumpFramesIntoVideo(filePath + "." + name + ".mp4", frames);
    }
#endif
    return ok;
}

bool bcellToMp4(const char * fp) {
#if (DISABLE_VIDEO_OUTPUT == 1)
    Q_UNUSED(fp);
    qWarning() << "ERR: video generation is disabled";
    return false;
#else
    QString filePath(fp);
    op::BcellFile bcell(filePath);
    return dumpFramesIntoVideo(filePath + ".mp4", bcell.frames());
#endif
}

int main(int argc, char ** argv) {
    int ret = EXIT_SUCCESS;

    if (argc < 2) {
        std::cout
            << "USAGE: " << argv[0] << " --rgb2png file.rgb [fileN.rgb]\n"
            << "   or  " << argv[0] << " --png2rgb file.png [fileN.png]\n"
            << "   or  " << argv[0] << " --bsv2png file.bsv3 [fileN.bsv3]\n"
            << "   or  " << argv[0] << " --bsv2mp4 file.bsv3 [fileN.bsv3]\n"
            << "   or  " << argv[0] << " --bcell2mp4 file.bcell [fileN.bcell]\n";
        ret = EXIT_FAILURE;
    } else {
        convertFunction functions[] = {
            rgbToPng,
            pngToRgb,
            bsv3ToPng,
            bsv3ToMp4,
            bcellToMp4
        };

        const char* args[] = {
            "--rgb2png",
            "--png2rgb",
            "--bsv2png",
            "--bsv2mp4",
            "--bcell2mp4"
        };

        Q_ASSERT(sizeof(functions)/sizeof(*functions) == sizeof(args)/sizeof(*args));

        ret = EXIT_FAILURE;
        unsigned index = 0;
        for (; index < sizeof(args)/sizeof(*args); ++index) {
            const char * a = argv[1];
            const char * b = args[index];
            if (!strcmp(a, b)) {
                ret = EXIT_SUCCESS;
                break;
            }
        }

        if (ret == EXIT_SUCCESS) {
            for (int i = 2; i < argc; ++i) {
                std::cout << argv[i] << '\n';
                if (!functions[index](argv[i])) {
                    ret = EXIT_FAILURE;
                }
            }
        }
    }
    return ret;
}
