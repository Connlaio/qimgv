#include "mainwindow.h"

MainWindow::MainWindow() :
    imageViewer(NULL),
    videoPlayer(NULL),
    panel(NULL),
    currentViewer(0),
    currentDisplay(0),
    layout(NULL),
    borderlessEnabled(false),
    desktopWidget(NULL)
{
    myTimer.start();
    resize(1100, 700);
    setMinimumSize(QSize(400, 300));
#ifdef __linux__
    setWindowIcon(QIcon(":/images/res/icons/22.png"));
#endif
    this->setMouseTracking(true);
    this->setAcceptDrops(true);
    init();
    setWindowTitle(QCoreApplication::applicationName() +
                   " " +
                   QCoreApplication::applicationVersion());
}

void MainWindow::init() {
    desktopWidget = QApplication::desktop();

    imageViewer = new ImageViewer(this);
    imageViewer->hide();
    videoPlayer = new VideoPlayer(this);
    videoPlayer->hide();

    QWidget *central = new QWidget();
    controlsOverlay = new ControlsOverlay(imageViewer);
    controlsOverlay->hide();
    infoOverlay = new textOverlay(imageViewer);

    layout = new QVBoxLayout;
    central->setAttribute(Qt::WA_MouseTracking);
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    central->setSizePolicy(sizePolicy);
    layout->setContentsMargins(0, 0, 0, 0);
    central->setLayout(layout);
    this->setCentralWidget(central);

    core = new Core();

    connect(settings, SIGNAL(settingsChanged()),
            this, SLOT(readSettings()));

    enableImageViewer();

    connect(this, SIGNAL(signalNextImage()),
            core, SLOT(slotNextImage()));

    connect(this, SIGNAL(signalPrevImage()),
            core, SLOT(slotPrevImage()));

    connect(this, SIGNAL(fileOpened(QString)),
            core, SLOT(loadImage(QString)));

    connect(this, SIGNAL(signalFullscreenEnabled(bool)),
            this, SLOT(slotShowControls(bool)));

    connect(this, SIGNAL(signalFullscreenEnabled(bool)),
            this, SLOT(slotShowInfo(bool)));

    connect(core, SIGNAL(infoStringChanged(QString)),
            this, SLOT(setInfoString(QString)));

    connect(controlsOverlay, SIGNAL(exitClicked()),
            this, SLOT(close()));

    connect(controlsOverlay, SIGNAL(exitFullscreenClicked()),
            this, SLOT(slotTriggerFullscreen()));

    connect(controlsOverlay, SIGNAL(minimizeClicked()),
            this, SLOT(slotMinimize()));



    connect(core, SIGNAL(videoChanged(Clip *)),
            this, SLOT(openVideo(Clip *)), Qt::UniqueConnection);

    connect(core, SIGNAL(stopVideo()),
            this, SLOT(disableVideoPlayer()));

    // Shortcuts

    connect(actionManager, SIGNAL(nextImage()), core, SLOT(slotNextImage()));
    connect(actionManager, SIGNAL(prevImage()), core, SLOT(slotPrevImage()));
    connect(actionManager, SIGNAL(fitAll()), this, SLOT(slotFitAll()));
    connect(actionManager, SIGNAL(fitWidth()), this, SLOT(slotFitWidth()));
    connect(actionManager, SIGNAL(fitNormal()), this, SLOT(slotFitNormal()));
    connect(actionManager, SIGNAL(toggleFitMode()), this, SLOT(switchFitMode()));
    connect(actionManager, SIGNAL(toggleFullscreen()), this, SLOT(slotTriggerFullscreen()));
    connect(actionManager, SIGNAL(toggleMenuBar()), this, SLOT(triggerMenuBar()));
    connect(actionManager, SIGNAL(zoomIn()), imageViewer, SLOT(slotZoomIn()));
    connect(actionManager, SIGNAL(zoomOut()), imageViewer, SLOT(slotZoomOut()));
    connect(actionManager, SIGNAL(rotateLeft()), this, SLOT(slotRotateLeft()));
    connect(actionManager, SIGNAL(rotateRight()), this, SLOT(slotRotateRight()));
    connect(actionManager, SIGNAL(openSettings()), this, SLOT(showSettings()));
    connect(actionManager, SIGNAL(crop()), this, SLOT(slotCrop()));
    connect(actionManager, SIGNAL(setWallpaper()), this, SLOT(slotSelectWallpaper()));
    connect(actionManager, SIGNAL(open()), this, SLOT(slotOpenDialog()));
    connect(actionManager, SIGNAL(save()), this, SLOT(slotSaveDialog()));
    connect(actionManager, SIGNAL(exit()), this, SLOT(close()));

    connect(this, SIGNAL(fileSaved(QString)), core, SLOT(saveImage(QString)));

    createActions();
    createMenus();

    readSettingsInitial();

    core->init();
}

void MainWindow::enablePanel() {
    if(!panel) {
        panel = new ThumbnailStrip(this);
    }

    connect(this, SIGNAL(resized(QSize)), panel, SLOT(parentResized(QSize)));

    connect(core, SIGNAL(imageChanged(int)),
            panel, SLOT(selectThumbnail(int)), Qt::UniqueConnection);

    connect(panel, SIGNAL(thumbnailClicked(int)),
            core, SLOT(loadImageByPos(int)), Qt::UniqueConnection);

    connect(panel, SIGNAL(thumbnailRequested(int)),
            core, SIGNAL(thumbnailRequested(int)), Qt::UniqueConnection);

    connect(core, SIGNAL(thumbnailReady(int, Thumbnail *)),
            panel, SLOT(setThumbnail(int, Thumbnail *)), Qt::UniqueConnection);

    connect(core, SIGNAL(cacheInitialized(int)),
            panel, SLOT(fillPanel(int)), static_cast<Qt::ConnectionType>(Qt::DirectConnection | Qt::UniqueConnection));

    connect(panel, SIGNAL(panelSizeChanged()),
               this, SLOT(calculatePanelTriggerArea()), Qt::UniqueConnection);

    connect(this, SIGNAL(signalFullscreenEnabled(bool)),
            panel, SLOT(enableWindowControls(bool)), Qt::UniqueConnection);

    connect(panel, SIGNAL(openClicked()), this, SLOT(slotOpenDialog()), Qt::UniqueConnection);
    connect(panel, SIGNAL(saveClicked()), this, SLOT(slotSaveDialog()), Qt::UniqueConnection);
    connect(panel, SIGNAL(settingsClicked()), this, SLOT(showSettings()), Qt::UniqueConnection);
    connect(panel, SIGNAL(exitClicked()), this, SLOT(close()), Qt::UniqueConnection);

    panel->parentResized(size());

    panel->fillPanel(core->imageCount());
}

void MainWindow::disablePanel() {
    if(!panel) {
        return;
    }
    disconnect(core, SIGNAL(imageChanged(int)),
            panel, SLOT(selectThumbnail(int)));

    disconnect(panel, SIGNAL(thumbnailClicked(int)),
            core, SLOT(loadImageByPos(int)));

    disconnect(panel, SIGNAL(thumbnailRequested(int)),
            core, SIGNAL(thumbnailRequested(int)));

    disconnect(core, SIGNAL(thumbnailReady(int, Thumbnail *)),
            panel, SLOT(setThumbnail(int, Thumbnail *)));

    disconnect(core, SIGNAL(cacheInitialized(int)),
            panel, SLOT(fillPanel(int)));

    disconnect(panel, SIGNAL(panelSizeChanged()),
               this, SLOT(calculatePanelTriggerArea()));

    disconnect(this, SIGNAL(signalFullscreenEnabled(bool)),
            panel, SLOT(enableWindowControls(bool)));

    disconnect(panel, SIGNAL(openClicked()), this, SLOT(slotOpenDialog()));
    disconnect(panel, SIGNAL(saveClicked()), this, SLOT(slotSaveDialog()));
    disconnect(panel, SIGNAL(settingsClicked()), this, SLOT(showSettings()));
    disconnect(panel, SIGNAL(exitClicked()), this, SLOT(close()));

}

void MainWindow::enableImageViewer() {
    if(currentViewer != 1) {
        disableVideoPlayer();
        controlsOverlay->setParent(imageViewer);
        infoOverlay->setParent(imageViewer);
        layout->addWidget(imageViewer);

        imageViewer->show();

        connect(imageViewer, SIGNAL(scalingRequested(QSize)),
                core, SLOT(rescaleForZoom(QSize)), Qt::UniqueConnection);

        connect(core, SIGNAL(scalingFinished(QPixmap *)),
                imageViewer, SLOT(updateImage(QPixmap *)), Qt::UniqueConnection);

        connect(core, SIGNAL(frameChanged(QPixmap *)),
                imageViewer, SLOT(updateImage(QPixmap *)),
                static_cast<Qt::ConnectionType>(Qt::DirectConnection | Qt::UniqueConnection));
        // reload after image edits
        connect(core, SIGNAL(imageAltered(QPixmap *)),
                imageViewer, SLOT(displayImage(QPixmap *)), Qt::UniqueConnection);

        connect(core, SIGNAL(videoAltered(Clip *)),
                videoPlayer, SLOT(displayVideo(Clip *)), Qt::UniqueConnection);

        connect(imageViewer, SIGNAL(cropSelected(QRect)),
                core, SLOT(crop(QRect)), Qt::UniqueConnection);

        connect(imageViewer, SIGNAL(wallpaperSelected(QRect)),
                core, SLOT(setWallpaper(QRect)), Qt::UniqueConnection);

        connect(core, SIGNAL(signalSetImage(QPixmap *)),
                this, SLOT(openImage(QPixmap *)), Qt::UniqueConnection);

        connect(this, SIGNAL(signalFitAll()),
                imageViewer, SLOT(slotFitAll()), Qt::UniqueConnection);

        connect(this, SIGNAL(signalFitWidth()),
                imageViewer, SLOT(slotFitWidth()), Qt::UniqueConnection);

        connect(this, SIGNAL(signalFitNormal()),
                imageViewer, SLOT(slotFitNormal()), Qt::UniqueConnection);

        connect(this, SIGNAL(signalZoomIn()),
                imageViewer, SLOT(slotZoomIn()), Qt::UniqueConnection);

        connect(this, SIGNAL(signalZoomOut()),
                imageViewer, SLOT(slotZoomOut()), Qt::UniqueConnection);

        updateOverlays();
        currentViewer = 1;
    }
}

void MainWindow::disableImageViewer() {
    layout->removeWidget(imageViewer);

    disconnect(imageViewer, SIGNAL(scalingRequested(QSize)),
               core, SLOT(rescaleForZoom(QSize)));

    disconnect(imageViewer, SIGNAL(cropSelected(QRect)),
               core, SLOT(crop(QRect)));

    disconnect(imageViewer, SIGNAL(wallpaperSelected(QRect)),
               core, SLOT(setWallpaper(QRect)));

    disconnect(core, SIGNAL(signalSetImage(QPixmap *)),
               imageViewer, SLOT(displayImage(QPixmap *)));

    disconnect(this, SIGNAL(signalZoomIn()),
               imageViewer, SLOT(slotZoomIn()));

    disconnect(this, SIGNAL(signalZoomOut()),
               imageViewer, SLOT(slotZoomOut()));

    currentViewer = 0;
    imageViewer->hide();
}

void MainWindow::enableVideoPlayer() {
    if(currentViewer != 2) {
        connect(this, SIGNAL(resized(QSize)),
                videoPlayer, SIGNAL(parentResized(QSize)), Qt::UniqueConnection);
        disableImageViewer();
        controlsOverlay->setParent(videoPlayer);
        infoOverlay->setParent(videoPlayer);
        layout->addWidget(videoPlayer);
        currentViewer = 2;
        videoPlayer->show();
        updateOverlays();
    }
}

void MainWindow::disableVideoPlayer() {
    layout->removeWidget(videoPlayer);
    videoPlayer->stop();
    disconnect(this, SIGNAL(resized(QSize)),
               videoPlayer, SIGNAL(parentResized(QSize)));
    currentViewer = 0;
    videoPlayer->hide();
}

void MainWindow::openVideo(Clip *clip) {
    enableVideoPlayer();
    videoPlayer->displayVideo(clip);
}

void MainWindow::open(QString path) {
    int nMilliseconds = myTimer.elapsed();
    qDebug() << "init time:" << nMilliseconds;
    core->loadImageBlocking(path);
    nMilliseconds = myTimer.elapsed();
    qDebug() << "open time: " << nMilliseconds;
}

void MainWindow::openImage(QPixmap *pixmap) {
    enableImageViewer();
    imageViewer->displayImage(pixmap);
}

void MainWindow::readSettingsInitial() {
    readSettings();
    currentDisplay = settings->lastDisplay();
    if(!settings->fullscreenMode()) {
        restoreWindowGeometry();
    }
}

void MainWindow::readSettings() {
    borderlessEnabled = settings->fullscreenTaskbarShown();
    menuBar()->setHidden(settings->menuBarHidden());
    panelPosition = settings->panelPosition();
    int fitMode = settings->imageFitMode();
    if(fitMode == 1) {
        slotFitWidth();
    } else if(fitMode == 2) {
        slotFitNormal();
    } else {
        slotFitAll();
    }
    emit resized(size());
    settings->panelEnabled()?enablePanel():disablePanel();
    calculatePanelTriggerArea();
}

void MainWindow::calculatePanelTriggerArea() {
    if(!settings->panelEnabled()) {
        panelArea.setRect(0,0,0,0);
        return;
    }
    switch(panelPosition) {
        case LEFT:
            panelArea.setRect(0, 0, panel->width() - 1, height());
            break;
        case RIGHT:
            panelArea.setRect(width() - panel->width() + 1, 0, width(), height());
            break;
        case BOTTOM:
            panelArea.setRect(0, height() - panel->height() + 1, width() - 180, height());
            break;
        case TOP:
            panelArea.setRect(0, 0, width(), panel->height() - 1);
            break;
        }
}

void MainWindow::updateOverlays() {
    controlsOverlay->updateSize(this->centralWidget()->size());
    infoOverlay->updateWidth(this->centralWidget()->width());
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    Q_UNUSED(event)
    if(panel) {
        emit resized(size());
    }
    updateOverlays();
}

//#############################################################
//######################### EVENTS ############################
//#############################################################

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    if(event->buttons() != Qt::RightButton && event->buttons() != Qt::LeftButton) {
        if(panelArea.contains(event->pos()) &&
           panel &&
           !panelArea.contains(lastMouseMovePos))
        {
            panel->show();
        }
        event->ignore();
    }
    lastMouseMovePos = event->pos();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e) {
    if(e->mimeData()->hasUrls()) {
        e->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event) {
    const QMimeData *mimeData = event->mimeData();
    // check for our needed mime type, here a file or a list of files
    if(mimeData->hasUrls()) {
        QStringList pathList;
        QList<QUrl> urlList = mimeData->urls();
        // extract the local paths of the files
        for(int i = 0; i < urlList.size() && i < 32; ++i) {
            pathList.append(urlList.at(i).toLocalFile());
        }
        // try to open first file in the list
        open(pathList.first());
    }
}

bool MainWindow::event(QEvent *event) {
    return (actionManager->processEvent(event)) ? true : QMainWindow::event(event);
}


void MainWindow::closeEvent(QCloseEvent *event) {
    this->hide();
    if(QThreadPool::globalInstance()->activeThreadCount()) {
        QThreadPool::globalInstance()->waitForDone();
    }
    if(!isMaximized() && !isFullScreen()) {
        saveWindowGeometry();
    }
    saveDisplay();
    QMainWindow::closeEvent(event);
}

void MainWindow::saveDisplay() {
    if(settings->fullscreenMode() || isFullScreen()) {
        settings->setLastDisplay(desktopWidget->screenNumber(this));
    }
}

void MainWindow::saveWindowGeometry() {
    settings->setWindowGeometry(QRect(pos(), size()));
}

void MainWindow::restoreWindowGeometry() {
    QRect geometry = settings->windowGeometry();
    this->resize(geometry.size());
    this->move(geometry.x(), geometry.y());
}

MainWindow::~MainWindow() {
}

//#############################################################
//######################## OTHERS #############################
//#############################################################

void MainWindow::setInfoString(QString text) {
    infoOverlay->setText(text);
    setWindowTitle(text);
}

void MainWindow::slotShowControls(bool x) {
    x ? controlsOverlay->show() : controlsOverlay->hide();
}

void MainWindow::slotShowInfo(bool x) {
    x ? infoOverlay->show() : infoOverlay->hide();
}

void MainWindow::triggerMenuBar() {
    if(this->menuBar()->isHidden()) {
        this->menuBar()->show();
    } else {
        this->menuBar()->hide();
    }
    settings->setMenuBarHidden(this->menuBar()->isHidden());
}

void MainWindow::showMenuBar() {
    if(!settings->menuBarHidden()) {
        menuBar()->show();
    }
}

//#############################################################
//###################### ACTION SLOTS #########################
//#############################################################

void MainWindow::switchFitMode() {
    if(modeFitAll->isChecked()) {
        this->slotFitNormal();
    } else {
        this->slotFitAll();
    }
}

void MainWindow::slotFitAll() {
    modeFitWidth->setChecked(false);
    modeFitNormal->setChecked(false);
    modeFitAll->setChecked(true);
    emit signalFitAll();
}

void MainWindow::slotFitWidth() {
    modeFitAll->setChecked(false);
    modeFitNormal->setChecked(false);
    modeFitWidth->setChecked(true);
    emit signalFitWidth();
}

void MainWindow::slotFitNormal() {
    modeFitAll->setChecked(false);
    modeFitWidth->setChecked(false);
    modeFitNormal->setChecked(true);
    emit signalFitNormal();
}

void MainWindow::slotTriggerFullscreen() {
    this->fullscreenEnabledAct->trigger();
}

void MainWindow::slotFullscreen() {
    if(fullscreenEnabledAct->isChecked()) {
        this->menuBar()->hide();
        // do not save immediately on application start
        if(!this->isHidden())
            saveWindowGeometry();

        //this->hide();
        //move to target screen
        int display = settings->lastDisplay();
        if(desktopWidget->screenCount() > display &&
           display != desktopWidget->screenNumber(this))
        {
            this->move(desktopWidget->screenGeometry(display).x(),
                       desktopWidget->screenGeometry(display).y());
        }

        if(borderlessEnabled) {
            // there is some stupid bullshit with this mode under X11
            this->setWindowFlags(Qt::FramelessWindowHint);
            this->showMaximized();;
        } else {
            this->showFullScreen();
        }
        emit signalFullscreenEnabled(true);

    } else {
        showMenuBar();
        this->setWindowFlags(windowFlags() & ~Qt::FramelessWindowHint);
        this->showNormal();
        restoreWindowGeometry();
        this->activateWindow();
        this->raise();
        emit signalFullscreenEnabled(false);
    }
}

void MainWindow::slotMinimize() {
    this->setWindowState(Qt::WindowMinimized);
}

void MainWindow::slotRotateLeft() {
    core->rotateImage(-90);
}

void MainWindow::slotRotateRight() {
    core->rotateImage(90);
}

void MainWindow::slotCrop() {
    this->slotFitAll();
    imageViewer->crop();
}

void MainWindow::slotSelectWallpaper() {
    this->slotFitAll();
    imageViewer->selectWallpaper();
}

void MainWindow::slotSaveDialog() {
    const QString imagesFilter = settings->supportedFormatsString();
    QString fileName = core->getCurrentFilePath();
    fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
                                            fileName,
                                            imagesFilter);
    emit fileSaved(fileName);
}

void MainWindow::slotOpenDialog() {
    QFileDialog dialog;
    QStringList imageFilter;
    imageFilter.append(settings->supportedFormatsString());
    imageFilter.append("All Files (*)");
    QString lastDir = settings->lastDirectory();
    dialog.setDirectory(lastDir);
    dialog.setNameFilters(imageFilter);
    dialog.setWindowTitle("Open image");
    //dialog.setParent(this);
    dialog.setWindowModality(Qt::ApplicationModal);
    connect(&dialog, SIGNAL(fileSelected(QString)), this, SIGNAL(fileOpened(QString)));
    dialog.exec();
}

void MainWindow::showSettings() {
    SettingsDialog settingsDialog;
    settingsDialog.exec();
}

void MainWindow::slotAbout() {
    QMessageBox msgBox;
    msgBox.setIconPixmap(QPixmap(":/images/res/icons/pepper.png"));
    QSpacerItem *horizontalSpacer =
        new QSpacerItem(250,
                        0,
                        QSizePolicy::Minimum,
                        QSizePolicy::Expanding);
    msgBox.setWindowTitle("About " +
                          QCoreApplication::applicationName() +
                          " " +
                          QCoreApplication::applicationVersion());
    QString message;
    message = "qimgv is a simple image viewer written in qt.";
    message.append("<br>This program is licensed under GNU GPL Version 3.");
    message.append("<br><br>Website: <a href='https://github.com/easymodo/qimgv'>github.com/easymodo/qimgv</a>");
    message.append("<br><br>Main developer: <br>Easymodo (easymodofrf@gmail.com)");
    message.append("<br><br><a href='https://github.com/easymodo/qimgv/graphs/contributors'>Contributors</a>");
    message.append("<br><br> This is a pre-release software.");
    message.append("<br> Expect bugs.");
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setText(message);
    QGridLayout *layout = (QGridLayout *) msgBox.layout();
    layout->addItem(horizontalSpacer,
                    layout->rowCount(),
                    0,
                    1,
                    layout->columnCount());
    msgBox.exec();
}

//#############################################################
//#################### MENU BAR & ACTIONS #####################
//#############################################################

void MainWindow::createActions() {
    openAct = new QAction(tr("&Open..."), this);
    this->addAction(openAct);
    connect(openAct, SIGNAL(triggered()), this, SLOT(slotOpenDialog()));

    saveAct = new QAction(tr("Save"), this);
    this->addAction(saveAct);
    connect(saveAct, SIGNAL(triggered()), this, SLOT(slotSaveDialog()));

    settingsAct = new QAction(tr("&Preferences"), this);
    this->addAction(settingsAct);
    connect(settingsAct, SIGNAL(triggered()), this, SLOT(showSettings()));

    exitAct = new QAction(tr("E&xit"), this);
    this->addAction(exitAct);
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

    rotateLeftAct = new QAction(tr("Rotate L&eft"), this);
    this->addAction(rotateLeftAct);
    connect(rotateLeftAct, SIGNAL(triggered()), this, SLOT(slotRotateLeft()));

    cropAct = new QAction(tr("C&rop"), this);
    this->addAction(cropAct);
    connect(cropAct, SIGNAL(triggered()), this, SLOT(slotCrop()));

    selectWallpaperAct = new QAction(tr("Set wallpaper"), this);
    this->addAction(selectWallpaperAct);
    connect(selectWallpaperAct, SIGNAL(triggered()), this, SLOT(slotSelectWallpaper()));

    rotateRightAct = new QAction(tr("Rotate R&ight"), this);
    this->addAction(rotateRightAct);
    connect(rotateRightAct, SIGNAL(triggered()), this, SLOT(slotRotateRight()));

    nextAct = new QAction(tr("N&ext"), this);
    nextAct->setEnabled(true);
    this->addAction(nextAct);
    connect(nextAct, SIGNAL(triggered()), core, SLOT(slotNextImage()));

    prevAct = new QAction(tr("P&rev"), this);
    this->addAction(prevAct);
    connect(prevAct, SIGNAL(triggered()), core, SLOT(slotPrevImage()));

    zoomInAct = new QAction(tr("Zoom &In (10%)"), this);
    this->addAction(zoomInAct);
    connect(zoomInAct, SIGNAL(triggered()), imageViewer, SLOT(slotZoomIn()));

    zoomOutAct = new QAction(tr("Zoom &Out (10%)"), this);
    this->addAction(zoomOutAct);
    connect(zoomOutAct, SIGNAL(triggered()), imageViewer, SLOT(slotZoomOut()));

    modeFitNormal = new QAction(tr("&Normal Size"), this);
    modeFitNormal->setEnabled(false);
    modeFitNormal->setCheckable(true);
    this->addAction(modeFitNormal);
    connect(modeFitNormal, SIGNAL(triggered()), this, SLOT(slotFitNormal()));

    modeFitAll = new QAction(tr("Fit all"), this);
    modeFitAll->setEnabled(false);
    modeFitAll->setCheckable(true);
    this->addAction(modeFitAll);
    connect(modeFitAll, SIGNAL(triggered()), this, SLOT(slotFitAll()));

    modeFitWidth = new QAction(tr("Fit &width"), this);
    modeFitWidth->setEnabled(false);
    modeFitWidth->setCheckable(true);
    this->addAction(modeFitWidth);
    connect(modeFitWidth, SIGNAL(triggered()), this, SLOT(slotFitWidth()));

    fullscreenEnabledAct = new QAction(tr("&Fullscreen"), this);
    fullscreenEnabledAct->setEnabled(true);
    fullscreenEnabledAct->setCheckable(true);
    this->addAction(fullscreenEnabledAct);
    connect(fullscreenEnabledAct, SIGNAL(triggered()),
            this, SLOT(slotFullscreen()));

    aboutAct = new QAction(tr("&About"), this);
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(slotAbout()));

    aboutQtAct = new QAction(tr("About &Qt"), this);
    connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

    modeFitNormal->setEnabled(true);
    modeFitAll->setEnabled(true);
    modeFitWidth->setEnabled(true);
}

void MainWindow::createMenus() {
    fileMenu = new QMenu(tr("&File"), this);
    fileMenu->addAction(openAct);
    fileMenu->addAction(saveAct);
    fileMenu->addAction(settingsAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    editMenu = new QMenu(tr("&Edit"), this);
    editMenu->addAction(cropAct);
    editMenu->addSeparator();
    editMenu->addAction(rotateLeftAct);
    editMenu->addAction(rotateRightAct);
    editMenu->addSeparator();
    editMenu->addAction(selectWallpaperAct);

    viewMenu = new QMenu(tr("&View"), this);
    viewMenu->addAction(fullscreenEnabledAct);
    viewMenu->addAction(zoomInAct);
    viewMenu->addAction(zoomOutAct);
    viewMenu->addSeparator();
    viewMenu->addAction(modeFitNormal);
    viewMenu->addAction(modeFitAll);
    viewMenu->addAction(modeFitWidth);

    navigationMenu = new QMenu(tr("&Navigate"), this);
    navigationMenu->addAction(nextAct);
    navigationMenu->addAction(prevAct);

    helpMenu = new QMenu(tr("&Help"), this);
    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);

    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(editMenu);
    menuBar()->addMenu(viewMenu);
    menuBar()->addMenu(navigationMenu);
    menuBar()->addMenu(helpMenu);
}


