#include "loadhelper.h"

LoadHelper::LoadHelper(ImageCache *_cache, QThread *_mainThread) :
    cache(_cache),
    loadTarget(-1),
    mainThread(_mainThread) {

}

void LoadHelper::setTarget(int pos, QString _path) {
    mutex.lock();
    loadTarget = pos;
    path = _path;
    mutex.unlock();
}

int LoadHelper::target() {
    return loadTarget;
}

void LoadHelper::doLoad() {
    mutex.lock();
    int targetLocal = loadTarget;
    QString pathLocal = path;
    mutex.unlock();
    if(cache->isLoaded(targetLocal)) {
        emit finished(targetLocal);
        return;
    }
    //qDebug() << "LOADHELPER: loading! "<< targetLocal;
    ImageFactory *factory = new ImageFactory();
    Image *img = factory->createImage(pathLocal);
    delete factory;

    img->moveToThread(mainThread);
    cache->setImage(img, targetLocal);

    emit finished(targetLocal);
}
