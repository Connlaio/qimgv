#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtWidgets>
#include <QFileDialog>
#include <QAction>
#include <QBoxLayout>
#include "core.h"
#include "overlays/infooverlay.h"
#include "overlays/controlsoverlay.h"
#include "settingsdialog.h"
#include "resizedialog.h"
#include "viewers/imageviewer.h"
#include "viewers/videoplayer.h"
#include "settings.h"
#include "actionmanager.h"
#include "thumbnailPanel/thumbnailstrip.h"
#include <time.h>

QT_BEGIN_NAMESPACE
    class QLabel;
    class QMenu;
    class QScrollArea;
    class QScrollBar;
QT_END_NAMESPACE

class Core;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public slots:
    void slotTriggerFullscreen();
    void slotMinimize();
    void setInfoString(QString);
    void readSettings();
    void enableImageViewer();
    void enableVideoPlayer();
    void enablePanel();
    void disablePanel();
    void disableImageViewer();
    void disableVideoPlayer();
    void slotCrop();

signals:
    void signalFitAll();
    void signalFitWidth();
    void signalFitNormal();
    void signalNextImage();
    void signalPrevImage();
    void signalZoomIn();
    void signalZoomOut();
    void signalFullscreenEnabled(bool);
    void fileOpened(QString);
    void fileSaved(QString);
    void resized(QSize);

public:
    MainWindow();
    ~MainWindow();    
    ImageViewer *imageViewer;
    VideoPlayer *videoPlayer;

    void open(QString path);
    void updateOverlays();

private slots:
    void slotOpenDialog();
    void slotFitAll();
    void slotFitWidth();
    void slotFitNormal();
    void switchFitMode();
    void slotResize(QSize);
    void slotResizeDialog();
    void slotRotateLeft();
    void slotRotateRight();
    void slotSaveDialog();


    void slotShowControls(bool);
    void slotShowInfo(bool x);
    void openVideo(Clip *clip);
    void openImage(QPixmap *pixmap);
    void showSettings();

    void slotSelectWallpaper();
    void calculatePanelTriggerArea();    

private:
    Core *core;
    textOverlay *infoOverlay, *messageOverlay;
    ControlsOverlay *controlsOverlay;
    ThumbnailStrip *panel;
    int currentViewer; // 0 = none; 1 = imageViewer; 2 = VideoPlayer;
    int currentDisplay;
    int fitMode;
    bool fullscreen; // separate flag because "borderless fullscreen" is actually a maximized window
    QBoxLayout *layout;
    PanelPosition panelPosition;
    QRect panelArea;
    QPoint lastMouseMovePos;
    void init();
    void saveWindowGeometry();
    void restoreWindowGeometry();

    void readSettingsInitial();
    bool borderlessEnabled;
    QDesktopWidget *desktopWidget;

    void adjustWindowPosLastScreen();
    QRect lastScreenGeometry();
    void saveDisplay();
protected:
    bool event(QEvent *event);
    void resizeEvent(QResizeEvent *event);
    void closeEvent(QCloseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *event);
};

#endif // MAINWINDOW_H
