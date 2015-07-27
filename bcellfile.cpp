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

#include "bcellfile.h"
#include "rgbfile.h"

#include <QDir>
#include <QDebug>
#include <QPainter>

#include <cstring>

namespace op {

BcellFile::BcellFile(const QString &filePath)
    : maxH(0) , maxW(0)
{
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "BcellFile: can't open file" << filePath;
        return;
    }

    // check signature here
    uchar len;
    int last = 0;
    int count = 0;
    char buff[512], * pTo = NULL;
    file.read(buff, 8);

    if (strncmp(buff, "bcell13", 7) != 0) {
        file.close();
        qDebug() << "BcellFile: invalid signature " << QString(buff);
        return;
    }

    count = 0;
    file.read((char*)&count, 2);

    last = filePath.lastIndexOf(QDir::separator());
    Q_ASSERT(last > 0);

    for (int i = 0; i <= last; ++i) {
        buff[i] = filePath[i].toLatin1();
    }
    last += 1;
    pTo = buff + last;

    int maxH = 0;
    int maxW = 0;

    QVector<RGBFile*> frames;
    QVector<float> xdiffs;
    for (int i = 0; i < count; ++i) {
        len = 0;
        file.read((char*)&len, 1);
        file.read(pTo, len);
        pTo[len] = '\0';

        if (len == 0) {
            qDebug() << "BcellFile: wrong" << filePath;
            break;
        }
        for (char * p = pTo; (*p = tolower(*p)); ++p);

        RGBFile * rgb = new RGBFile(buff);
        frames.push_back(rgb);

        if (rgb->image()->height() > maxH) maxH = rgb->image()->height();
        if (rgb->image()->width()  > maxW) maxW = rgb->image()->width();

        file.read(pTo, 0x0A);
        xdiffs.push_back(((float*)pTo)[0]);

        for (int i = 0, ie = pTo[0x09]; i < ie; ++i) {
            len = 0; file.read((char*)&len, 1); file.read(pTo, len);
            len = 0; file.read((char*)&len, 1); file.read(pTo, len);
            file.read(pTo, 0x1C);
        }
    }
    file.close();

    for (int i = 0; i < frames.size(); ++i) {
        QImage & frame = *(frames[i]->image());
        QImage img(maxW, maxH, frame.format());
        img.fill(Qt::transparent);
        QPainter p(&img);
        p.drawImage(maxW - xdiffs[i], maxH - frame.height(), frame);
        mFrames.push_back(img.copy());
        delete frames[i];
    }
}

BcellFile::~BcellFile() {}

} // namespace op
