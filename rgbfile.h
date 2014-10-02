// ///////////////////////////////////////////////////////////////////////// //
//                                                                           //
//   Copyright (C) 2014 by jsbot@ya.ru                                       //
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

#ifndef RGBFILE_H
#define RGBFILE_H

#include <QByteArray>

class QString;
class QImage;
class QIODevice;

class RGBFile {
public:
    RGBFile(const QString &filePath);
    RGBFile(const QString &name, QIODevice *, bool flip = false);
    ~RGBFile();

    void setImage(const QImage &);
    QImage * image() { return mPixmap; }
    int width()  const { return  mWidth; }
    int height() const { return mHeight; }

    static bool convertTo(const QString & srcFile, const QString & dstFile);

private:
    int mWidth;
    int mHeight;
    QImage * mPixmap;
    QByteArray mImageData;

    RGBFile(const RGBFile &);
    RGBFile& operator=(const RGBFile &);
    bool loadFromIODevice(const QString & name, QIODevice *);
};

#endif // RGBFILE_H
