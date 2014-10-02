#include <QImage>
#include <iostream>
#include "rgbfile.h"

bool workRgbFile(const char * fp) {
    QString filePath(fp);
    RGBFile rgb(filePath);
    QImage * img = rgb.image();
    bool ok = (img != NULL);
    if (!ok) {
        std::cout << "ERR: can't load '" << filePath.toStdString() << "'.\n";
    } else {
        filePath += ".png";
        ok = img->save(filePath, "PNG");
        if (!ok){
            std::cout << "ERR: can't save '" << filePath.toStdString() << "'.\n";
        }
    }
    return ok;
}

bool workImgFile(const char * fp) {
    QString filePath(fp);
    return RGBFile::convertTo(filePath, filePath + ".rgb");
}

int main(int argc, char ** argv) {
    int ret = 0;
    if (argc < 2) {
        std::cout << "USAGE: " << argv[0] << " file.rgb\n"
                  << "   or  " << argv[0] << " -b file.png\n"
        ;
        ret = 1;
    } else {
        if (argv[1][0] == '-' && argv[1][1] == 'b') {
            for (int i = 2; i < argc; ++i) {
                if (!workImgFile(argv[i])) {
                    ret = 1;
                }
            }
        } else for (int i = 1; i < argc; ++i) {
            if (!workRgbFile(argv[i])) {
                ret = 1;
            }
        }
    }
    return ret;
}
