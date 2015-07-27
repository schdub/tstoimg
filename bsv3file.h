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

#pragma once

#include <QString>
#include <QRect>
#include <QVector>
#include <QMap>
#include <QPair>
#include <QIODevice>
#include <QImage>

namespace  op {

class RGBFile;

class Bsv3File {
public:
    Bsv3File(QIODevice & bsvFile, const QImage & rgbImage);
    ~Bsv3File();
    QImage transImage() const;
    QVector< QImage > getFrames(const QString & framesName) const;
    QList<QString> animationNames() const;

private:
    struct Scrap {
        QString mName;
        QRect mRect;
        Scrap()
        {}
        Scrap(const QString & name, const QRect & rect)
            : mName(name), mRect(rect)
        {}
    };

    struct ScrapTransform {
        int mIndex;
        float mX;
        float mY;
        float mFactor;
        float mRX;
        float mRY;
        float mFactor2;
        ScrapTransform()
        {}
        ScrapTransform(int index, float data[6])
            : mIndex(index),
              mX(data[0]),  mY(data[1]),  mFactor(data[2]),
              mRX(data[3]), mRY(data[4]), mFactor2(data[5])
        {}
    };

    struct ScrapAnimation {
        QString mName;
        int mIndexStart;
        int mIndexStop;
        ScrapAnimation()
        {}
        ScrapAnimation(const QString & name, int start, int stop)
            : mName(name), mIndexStart(start), mIndexStop(stop)
        {}
    };

    bool loadFromIODevice(QIODevice & bsvFile);
//    float                                mWidth, mHeigth, mBaseX;
    QImage                               mPict;
    QVector< Scrap >                     mRects;
    QVector< QVector< ScrapTransform > > mTransformations;
    QMap< QString, QPair<int, int> >     mAnimations;
};

} // namespace op
