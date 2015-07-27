// ///////////////////////////////////////////////////////////////////////// //
//                                                                           //
//   Copyright (C) 2014, 2015 by Oleg Polivets                               //
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

#include "rgbfile.h"

#include <QFile>
#include <QImage>
#include <QDebug>
#include <QPainter>
#include <QTransform>
#include <QRgb>

#define RGB_FILE_SIGNATURE (0x20000000)

namespace op {

RGBFile::RGBFile(const QString &filePath)
: mWidth(0), mHeight(0), mPixmap(0) {
    QFile file(filePath);
    loadFromIODevice(filePath, &file);
}

RGBFile::RGBFile(const QString &name, QIODevice * f, bool flip)
: mWidth(0), mHeight(0), mPixmap(0) {
    if (loadFromIODevice(name, f) && flip) {
        setImage(mPixmap->transformed(QTransform().scale(-1, 1)));
    }
}

RGBFile::~RGBFile() {
    delete mPixmap;
}

bool RGBFile::loadFromIODevice(const QString &name, QIODevice * f) {
    if (!f) {
        qDebug() << "RGBFile::loadFromIODevice() failed open" << name << "f is null";
        return false;
    }

    if (!f->open(QIODevice::ReadOnly)) {
        qDebug() << "RGBFile::loadFromIODevice() failed open" << name;
        return false;
    }

    int w = 0, h = 0, sig = 0;
    f->read((char*)&sig, 4);
    f->read((char*)&w, 2);
    f->read((char*)&h, 2);

    if (sig != RGB_FILE_SIGNATURE) {
        qWarning("ERR: invalid signature value 0x%x", sig) ;
        f->close();
        return false;
    }

    uchar bits = 16;
    int bytesPerLine = w * (bits / 8);
    int bytes        = h * bytesPerLine;

#if (0)
    qDebug() << name
             << "bits =" << bits
             << "w =" << w
             << "h =" << h
             << "bl =" << bytesPerLine
             << "bc =" << bytes;
#endif

    mImageData = f->read(bytes);
    f->close();

    if (bytes != mImageData.size()) {
        qWarning("ERR: bytes count by header calculation and readed not equal");
        return false;
    }

    if (mImageData.isEmpty()) {
        return false;
    }

    mWidth  = w;
    mHeight = h;

    // BGRA 4444 -> ABGR 4444
    Q_ASSERT (mImageData.size() == bytes);
    uchar* p = (uchar*) mImageData.data();
    uchar* pe = p + bytes;
    uchar r, g, b, a;
    for (;p < pe; p += 2) {
        a = (p[0] & 0x0f);
        r = (p[0] & 0xf0) >> 4;
        g = (p[1] & 0x0f);
        b = (p[1] & 0xf0) >> 4;
        p[0] = r | (g << 4);
        p[1] = b | (a << 4);
    }
#if (0)
    // cut empty lines from bottom
    for (p = pe - 1; (*p & 0xf0) <= 16; p -= 2);
    int diff = (pe - 1 - p) / bytesPerLine;
    h -= diff;
    // ... and top
    for (pe = (uchar*)mImageData.data(), p = pe + 1; (*p & 0xf0) <= 16; p += 2);
    diff = (p - pe) / bytesPerLine;
    p  = pe + (bytesPerLine * diff);
    h -= diff;
    mHeight = h;
#else
    p = (uchar*) mImageData.data();
#endif
    mPixmap = new QImage(p, w, h, QImage::Format_ARGB4444_Premultiplied);
    return true;
}

void RGBFile::setImage(const QImage & img) {
    QImage * pimg = new QImage(img.copy()), *prev = mPixmap;
    mPixmap = pimg;
    delete prev;
}

bool RGBFile::convertTo(const QString & srcFile, const QString & dstFile) {
    QImage img;
    if (!img.load(srcFile)) {
        qDebug() << "can't open source file" << srcFile;
        return false;
    }

    QFile out(dstFile);
    if (!out.open(QIODevice::WriteOnly)) {
        qDebug() << "cant' open dest file" << dstFile;
        return false;
    }

    // write header

    int t = 0;
    t = RGB_FILE_SIGNATURE;
    out.write((char*)&t, 4);
    t = img.width();
    out.write((char*)&t, 2);
    t = img.height();
    out.write((char*)&t, 2);

    // process and write pixels

    QByteArray data;
    data.resize(img.height() * img.width() * 2);
    uchar* p = (uchar*) data.data();
    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x, p += 2) {
            const QRgb & c = img.pixel(x, y);
            p[0] = (qAlpha(c) / 16) | (((qBlue(c)) / 16) << 4);
            p[1] = (qGreen(c) / 16) | (((qRed(c))  / 16) << 4);
        }
    }
    out.write(data);
    out.close();

    return true;
}

QImage* RGBFile::releaseImage() {
    QImage* img = NULL;
    std::swap(img, mPixmap);
    return img;
}

} // namespace op
