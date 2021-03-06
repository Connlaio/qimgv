#include "imageanimated.h"
#include <time.h>

ImageAnimated::ImageAnimated(QString _path) {
    timer = new QTimer(this);
    path = _path;
    loaded = false;
    movie = new QMovie(this);
    fileInfo = new FileInfo(path, this);
}

ImageAnimated::ImageAnimated(FileInfo *_info) {
    timer = new QTimer(this);
    loaded = false;
    movie = new QMovie(this);
    fileInfo = _info;
    path = fileInfo->filePath();
}

ImageAnimated::~ImageAnimated() {
    this->animationStop();
    timer->deleteLater();
    delete movie;
    delete fileInfo;
}

//load image data from disk
void ImageAnimated::load() {
    QMutexLocker locker(&mutex);
    if(isLoaded()) {
        return;
    }
    if(!fileInfo) {
        fileInfo = new FileInfo(path, this);
    }
    movie->setFileName(path);
    movie->jumpToFrame(0);
    loaded = true;
}

void ImageAnimated::save(QString destinationPath) {
    QFile file(path);
    if(file.exists()) {
        if (!file.copy(destinationPath)) {
            qDebug() << "Unable to save file.";
        }
    } else {
        qDebug() << "Unable to save file. Perhaps the source file was deleted?";
    }
}

void ImageAnimated::save() {
    //TODO
}

QPixmap *ImageAnimated::generateThumbnail(bool squared) {
    Qt::AspectRatioMode method = squared?(Qt::KeepAspectRatioByExpanding):(Qt::KeepAspectRatio);
    int size = settings->thumbnailSize();
    QPixmap *tmp;
    if(!isLoaded()) {
        tmp = new QPixmap(path, fileInfo->fileExtension());
        *tmp = tmp->scaled(size * 2,
                           size * 2,
                           method,
                           Qt::FastTransformation)
               .scaled(size,
                       size,
                       method,
                       Qt::SmoothTransformation);
    } else {
        tmp = new QPixmap();
        lock();
        *tmp = movie->currentPixmap()
               .scaled(size * 2,
                       size * 2,
                       method,
                       Qt::FastTransformation)
               .scaled(size,
                       size,
                       method,
                       Qt::SmoothTransformation);
        unlock();
    }
    if(squared) {
        QRect target(0, 0, size, size);
        target.moveCenter(tmp->rect().center());
        QPixmap *thumbnail = new QPixmap(size, size);
        *thumbnail = tmp->copy(target);
        delete tmp;
        return thumbnail;
    } else {
        return tmp;
    }
}

// in case of gif returns current frame
QPixmap *ImageAnimated::getPixmap() {
    QPixmap *pix = new QPixmap();
    if(isLoaded()) {
        *pix = movie->currentPixmap().transformed(transform, Qt::SmoothTransformation);
    }
    return pix;
}

const QImage *ImageAnimated::getImage() {
    QImage *img = new QImage();
    if(isLoaded()) {
        *img = movie->currentImage().transformed(transform, Qt::SmoothTransformation);
    }
    const QImage *cPtr = img;
    return cPtr;
}

int ImageAnimated::height() {
    if(isLoaded()) {
        return movie->currentImage().height();
    }
    return 0;
}

int ImageAnimated::width() {
    return isLoaded() ? movie->currentImage().width() : 0;
}

QSize ImageAnimated::size() {
    return isLoaded() ? movie->currentImage().size() : QSize(0, 0);
}

void ImageAnimated::animationStart() {
    if(isLoaded()) {
        animationStop();
        timer->setSingleShot(true);
        connect(timer, SIGNAL(timeout()), this, SLOT(nextFrame()));
        startAnimationTimer();
    }
}

void ImageAnimated::animationStop() {
    if(isLoaded() && timer && timer->isActive()) {
        timer->stop();
        disconnect(timer, SIGNAL(timeout()), this, SLOT(nextFrame()));
        movie->jumpToFrame(0);
    }
}

void ImageAnimated::nextFrame() {
    if(!movie->jumpToNextFrame()) {
        movie->jumpToFrame(0);
    }
    QPixmap *newFrame = new QPixmap();
    *newFrame = movie->currentPixmap().transformed(transform, Qt::SmoothTransformation);
    startAnimationTimer();
    emit frameChanged(newFrame);
}

void ImageAnimated::startAnimationTimer() {
    if(timer && movie) {
        timer->start(movie->nextFrameDelay());
    }
}

void ImageAnimated::rotate(int grad) {
    mutex.lock();
    if(isLoaded()) {
        //TODO
        transform.rotate(grad);
    }
    mutex.unlock();
}

void ImageAnimated::crop(QRect newRect) {
    Q_UNUSED(newRect)
    mutex.lock();
    if(isLoaded()) {
        // TODO
    }
    mutex.unlock();
}
