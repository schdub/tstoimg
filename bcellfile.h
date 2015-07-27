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

#include <QVector>
#include <QString>
#include <QImage>

namespace op {

class BcellFile {
public:
    BcellFile(const QString & filePath);
    virtual ~BcellFile();
    const QVector<QImage> & frames() const { return mFrames; }
    int maximumHeight() const { return maxH; }
    int maximumWidth()  const { return maxW; }

private:
    QVector<QImage> mFrames;
    int maxH;
    int maxW;
};

} // namespace
