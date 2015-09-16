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

#include "bsv3file.h"
#include "rgbfile.h"

#include <QFile>
#include <QImage>
#include <QPainter>
#include <QTransform>
#include <QDebug>

#include <limits>
#include <iostream>

//#define ENABLE_LOGS

namespace op {

Bsv3File::~Bsv3File()
{}

Bsv3File::Bsv3File(QIODevice & bsvFile, const QImage & rgbImage)
    : mPict(rgbImage)
{
    loadFromIODevice(bsvFile);
}

bool Bsv3File::loadFromIODevice(QIODevice & file) {
    if (!file.open(QIODevice::ReadOnly)) {
        std::cout << "Bsv3: can't open file\n";
        return false;
    }

    int sign = 0;
    file.read((char*)&sign, 2);

    if (sign == 0x0104) {
        std::cout << file.read(5).toHex().data() << "\n";
    }

#ifdef ENABLE_LOGS
    std::cout << "signature=" << sign << "\n";
#endif

    int regionsCount = 0;
    file.read((char*)&regionsCount, 2);

#ifdef ENABLE_LOGS
    std::cout << "regionsCount=0x" << std::hex << regionsCount << "\n";
#endif

    // read flags

    uchar len=0;
    file.read((char*)&len, 1);
    bool hasOpacity = ((len & 1) == 1);
    int transCnt = 24 + (hasOpacity ? 1 : 0);

    if (sign == 0x0104) {
        transCnt += 1;
    }

    // set of image regions

    int x,y,w,h;
    char name[256];
    for (int i = 0; i < regionsCount; ++i) {
        len=0;
        file.read((char*)&len, 1);
        file.read(name, len);
        name[len] = '\0';
        x=y=w=h=0;
        file.read((char*)&x, 2);
        file.read((char*)&y, 2);
        file.read((char*)&w, 2);
        file.read((char*)&h, 2);
        mRects.push_back(Scrap(QString(name), QRect(x, y, w, h)));
#ifdef ENABLE_LOGS
        std::cout << name << ":"
                  << x << ":"
                  << y << ":"
                  << w << ":"
                  << h << "\n";
#endif
    }

    if (sign == 0x0104) {
        std::cout << file.read(2).toHex().data() << "\n";
    }

    // set of transformations

    int count, sub;
    count = 0;
    file.read((char*)&count, 2);

#ifdef ENABLE_LOGS
    std::cout << "transformationsCount=" << count << "\n";
#endif

    for (int i = 0; i < count; ++i) {
#ifdef ENABLE_LOGS
        std::cout << "offset=0x" << std::hex << file.pos() << std::endl;
#endif
        sub = 0;
        file.read((char*)&sub, 2);
        len = 0;
        file.read((char*)&len, 1);
#ifdef ENABLE_LOGS
        std::cout << "[" << i << "]" << sub << "\n";
#endif

        mTransformations.push_back(QVector<ScrapTransform>());

        int regId = 0;
        for (int j = 0; j < sub; ++j) {
            regId = 0;
            file.read((char*)&regId, 2);
            QByteArray data(file.read(transCnt));
            Q_ASSERT(data.size() == transCnt);
            ScrapDrawData * sdd = (ScrapDrawData *) data.data();
#ifdef ENABLE_LOGS
            std::cout << " "
               << regId << " "
               << sdd->x << " "
               << sdd->y << " "
               << sdd->xscale << " "
               << sdd->skew_h << " "
               << sdd->skew_v << " "
               << sdd->yscale << std::endl;
#endif
            mTransformations[i].push_back(ScrapTransform(regId, sdd, hasOpacity ? sdd->opacity : 0xff));
        }
    }

    // animation indexes

    int ib, ie;
    count = 0;
    file.read((char*)&count, 2);
#ifdef ENABLE_LOGS
    std::cout << "animsCount = " << count << std::endl;
#endif
    for (int i = 0; i < count; ++i) {
        len = 0;
        file.read((char*)&len, 1);
        file.read(name, len);
        name[len] = '\0';
        ib = ie = 0;
        file.read((char*)&ib, 2);
        file.read((char*)&ie, 2);
#ifdef ENABLE_LOGS
        std::cout << name << ":" << ib << ":" << ie << "\n";
#endif
        mAnimations.insert(name, QPair<int, int>(ib, ie));
    }
    file.close();

    return true;
}

QImage createSubImage(const QImage & image, const QRect & rect) {
    size_t offset = rect.x() * image.depth() / 8
                  + rect.y() * image.bytesPerLine();
    return QImage(image.bits() + offset,
                  rect.width(),
                  rect.height(),
                  image.bytesPerLine(),
                  image.format());
}

QVector< QImage > Bsv3File::getFrames(const QString & framesName) const {
    Q_ASSERT(!mPict.isNull());
    QVector< QImage > frames;
    if (mAnimations.contains(framesName)) {
        const int ib = mAnimations[framesName].first;
        const int ie = mAnimations[framesName].second;
        Q_ASSERT(ib >= 0 && ib < mTransformations.size());
        Q_ASSERT(ie >= 0 && ie < mTransformations.size());

        // calculate width and height for set of frames
        float baseX, w, h;
        float minX = std::numeric_limits<float>::max();
        float minY = std::numeric_limits<float>::max();
        float maxX = -400;
        float maxY = -400;
        for (int index = ib; index <= ie; ++index) {
            for (int i = mTransformations[index].size()-1; i >= 0; --i) {
                const ScrapTransform & ts = mTransformations[index][i];
                const QRect & scrapRect = mRects[ ts.mIndex ].mRect;

                if (ts.mX < minX) minX = ts.mX;
                if (ts.mY < minY) minY = ts.mY;

                float t = ts.mX + scrapRect.width();
                if (t > maxX) {
                    maxX = t;
                }
                if (ts.mY > maxY) {
                    maxY = ts.mY;
                }
            }
        }

        baseX = minX * -1;
        w = abs(maxX) + abs(minX);
        h = abs(maxY) + abs(minY);

        qDebug() << framesName
                 << "baseX =" << baseX
                 << "w =" << w
                 << "h =" << h;

        // make frames
        for (int index = ib; index <= ie; ++index) {
            // build single frame with transfomations data
            QImage img(w, h, mPict.format());
            Q_ASSERT(!img.isNull());
            img.fill(Qt::transparent);
            QPainter painter(&img);
            for (int i = mTransformations[index].size()-1; i >= 0; --i) {
                const ScrapTransform & ts = mTransformations[index][i];
                const QRect & scrapRect = mRects[ts.mIndex].mRect;
                QImage scrap(createSubImage(mPict, scrapRect));
                QTransform transform;
                transform.scale(ts.mFactor, ts.mFactor2);
                transform.rotate(ts.mRX, Qt::XAxis);
                transform.rotate(ts.mRY, Qt::YAxis);
                scrap = scrap.transformed(transform);
                float x = ts.mX + baseX;
                float y = ts.mY + h;
                Q_ASSERT( x >= 0 && y >= 0 );
                qDebug() << "x =" << x << "y =" << y;
                painter.setOpacity(ts.mOpacity);
                painter.drawImage(QPointF(x, y), scrap);
            }
            painter.end();
            frames.push_back(img);
        }
    }
    return frames;
}


QImage Bsv3File::transImage() const {
    const QVector< QImage > & frames = getFrames("Neutral");
    return frames.size() == 0 ? QImage() : frames[0];
}

QList<QString> Bsv3File::animationNames() const {
    return mAnimations.keys();
}

} // namespace op
