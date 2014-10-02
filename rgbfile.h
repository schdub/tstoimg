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
