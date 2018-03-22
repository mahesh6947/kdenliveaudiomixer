/***************************************************************************
 *   Copyright (C) 2007 by Jean-Baptiste Mardelle (jb@kdenlive.org)        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#include "monitor.h"
#include "bin/bin.h"
#include "bin/projectclip.h"
#include "core.h"
#include "dialogs/profilesdialog.h"
#include "doc/kdenlivedoc.h"
#include "doc/kthumb.h"
#include "glwidget.h"
#include "kdenlivesettings.h"
#include "lib/audio/audioStreamInfo.h"
#include "mainwindow.h"
#include "mltcontroller/bincontroller.h"
#include "mltcontroller/clipcontroller.h"
#include "monitorcontroller.hpp"
#include "project/projectmanager.h"
#include "qmlmanager.h"
#include "recmanager.h"
#include "scopes/monitoraudiolevel.h"
#include "mltcontroller/clip.h"
#include "timeline2/model/snapmodel.hpp"
#include "transitions/transitionsrepository.hpp"
#include "utils/KoIconUtils.h"

#include "klocalizedstring.h"
#include <KDualAction>
#include <KFileWidget>
#include <KMessageBox>
#include <KMessageWidget>
#include <KRecentDirs>
#include <KSelectAction>
#include <KWindowConfig>
#include <kio_version.h>

#include "kdenlive_debug.h"
#include <QDesktopWidget>
#include <QDrag>
#include <QFileDialog>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QQuickItem>
#include <QScrollBar>
#include <QSlider>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidgetAction>

#define SEEK_INACTIVE (-1)

QuickEventEater::QuickEventEater(QObject *parent)
    : QObject(parent)
{
}

bool QuickEventEater::eventFilter(QObject *obj, QEvent *event)
{
    switch (event->type()) {
    case QEvent::DragEnter: {
        QDragEnterEvent *ev = reinterpret_cast<QDragEnterEvent *>(event);
        if (ev->mimeData()->hasFormat(QStringLiteral("kdenlive/effect"))) {
            ev->acceptProposedAction();
            return true;
        }
        break;
    }
    case QEvent::DragMove: {
        QDragEnterEvent *ev = reinterpret_cast<QDragEnterEvent *>(event);
        if (ev->mimeData()->hasFormat(QStringLiteral("kdenlive/effect"))) {
            ev->acceptProposedAction();
            return true;
        }
        break;
    }
    case QEvent::Drop: {
        QDropEvent *ev = static_cast<QDropEvent *>(event);
        if (ev) {
            QStringList effectData;
            effectData << QString::fromUtf8(ev->mimeData()->data(QStringLiteral("kdenlive/effect")));
            QStringList source = QString::fromUtf8(ev->mimeData()->data(QStringLiteral("kdenlive/effectsource"))).split(QLatin1Char('-'));
            effectData << source;
            emit addEffect(effectData);
            ev->accept();
            return true;
        }
        break;
    }
    default:
        break;
    }
    return QObject::eventFilter(obj, event);
}

QuickMonitorEventEater::QuickMonitorEventEater(QWidget *parent)
    : QObject(parent)
{
}

bool QuickMonitorEventEater::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ev = static_cast<QKeyEvent *>(event);
        if (ev) {
            emit doKeyPressEvent(ev);
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}

Monitor::Monitor(Kdenlive::MonitorId id, MonitorManager *manager, QWidget *parent)
    : AbstractMonitor(id, manager, parent)
    , m_controller(nullptr)
    , m_glMonitor(nullptr)
    , m_snaps(new SnapModel())
    , m_splitEffect(nullptr)
    , m_splitProducer(nullptr)
    , m_dragStarted(false)
    , m_recManager(nullptr)
    , m_loopClipAction(nullptr)
    , m_sceneVisibilityAction(nullptr)
    , m_multitrackView(nullptr)
    , m_contextMenu(nullptr)
    , m_loopClipTransition(true)
    , m_editMarker(nullptr)
    , m_forceSizeFactor(0)
    , m_lastMonitorSceneType(MonitorSceneDefault)
{
    auto *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Create container widget
    m_glWidget = new QWidget;
    auto *glayout = new QGridLayout(m_glWidget);
    glayout->setSpacing(0);
    glayout->setContentsMargins(0, 0, 0, 0);
    // Create QML OpenGL widget
    m_glMonitor = new GLWidget((int)id);
    connect(m_glMonitor, &GLWidget::passKeyEvent, this, &Monitor::doKeyPressEvent);
    connect(m_glMonitor, &GLWidget::panView, this, &Monitor::panView);
    connect(m_glMonitor, &GLWidget::seekPosition, this, &Monitor::slotSeekPosition, Qt::DirectConnection);
    connect(m_glMonitor, &GLWidget::activateMonitor, this, &AbstractMonitor::slotActivateMonitor, Qt::DirectConnection);
    m_monitorController = new MonitorController(m_glMonitor);
    m_videoWidget = QWidget::createWindowContainer(qobject_cast<QWindow *>(m_glMonitor));
    m_videoWidget->setAcceptDrops(true);
    auto *leventEater = new QuickEventEater(this);
    m_videoWidget->installEventFilter(leventEater);
    connect(leventEater, &QuickEventEater::addEffect, this, &Monitor::slotAddEffect);

    m_qmlManager = new QmlManager(m_glMonitor);
    connect(m_qmlManager, &QmlManager::effectChanged, this, &Monitor::effectChanged);
    connect(m_qmlManager, &QmlManager::effectPointsChanged, this, &Monitor::effectPointsChanged);

    auto *monitorEventEater = new QuickMonitorEventEater(this);
    m_glWidget->installEventFilter(monitorEventEater);
    connect(monitorEventEater, &QuickMonitorEventEater::doKeyPressEvent, this, &Monitor::doKeyPressEvent);

    glayout->addWidget(m_videoWidget, 0, 0);
    m_verticalScroll = new QScrollBar(Qt::Vertical);
    glayout->addWidget(m_verticalScroll, 0, 1);
    m_verticalScroll->hide();
    m_horizontalScroll = new QScrollBar(Qt::Horizontal);
    glayout->addWidget(m_horizontalScroll, 1, 0);
    m_horizontalScroll->hide();
    connect(m_horizontalScroll, &QAbstractSlider::valueChanged, this, &Monitor::setOffsetX);
    connect(m_verticalScroll, &QAbstractSlider::valueChanged, this, &Monitor::setOffsetY);
    connect(m_glMonitor, &GLWidget::frameDisplayed, this, &Monitor::onFrameDisplayed);
    connect(m_glMonitor, &GLWidget::mouseSeek, this, &Monitor::slotMouseSeek);
    connect(m_glMonitor, SIGNAL(monitorPlay()), this, SLOT(slotPlay()));
    connect(m_glMonitor, &GLWidget::startDrag, this, &Monitor::slotStartDrag);
    connect(m_glMonitor, SIGNAL(switchFullScreen(bool)), this, SLOT(slotSwitchFullScreen(bool)));
    connect(m_glMonitor, &GLWidget::zoomChanged, this, &Monitor::setZoom);
    connect(m_glMonitor, SIGNAL(lockMonitor(bool)), this, SLOT(slotLockMonitor(bool)), Qt::DirectConnection);
    connect(m_glMonitor, &GLWidget::showContextMenu, this, &Monitor::slotShowMenu);
    connect(m_glMonitor, &GLWidget::gpuNotSupported, this, &Monitor::gpuError);

    m_glWidget->setMinimumSize(QSize(320, 180));
    layout->addWidget(m_glWidget, 10);
    layout->addStretch();

    // Tool bar buttons
    m_toolbar = new QToolBar(this);
    QWidget *sp1 = new QWidget(this);
    sp1->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    m_toolbar->addWidget(sp1);
    if (id == Kdenlive::ClipMonitor) {
        // Add options for recording
        m_recManager = new RecManager(this);
        connect(m_recManager, &RecManager::warningMessage, this, &Monitor::warningMessage);
        connect(m_recManager, &RecManager::addClipToProject, this, &Monitor::addClipToProject);
    }

    if (id != Kdenlive::DvdMonitor) {
        m_toolbar->addAction(manager->getAction(QStringLiteral("mark_in")));
        m_toolbar->addAction(manager->getAction(QStringLiteral("mark_out")));
    }
    m_toolbar->addAction(manager->getAction(QStringLiteral("monitor_seek_backward")));

    auto *playButton = new QToolButton(m_toolbar);
    m_playMenu = new QMenu(i18n("Play..."), this);
    QAction *originalPlayAction = static_cast<KDualAction *>(manager->getAction(QStringLiteral("monitor_play")));
    m_playAction = new KDualAction(i18n("Play"), i18n("Pause"), this);
    m_playAction->setInactiveIcon(KoIconUtils::themedIcon(QStringLiteral("media-playback-start")));
    m_playAction->setActiveIcon(KoIconUtils::themedIcon(QStringLiteral("media-playback-pause")));

    QString strippedTooltip = m_playAction->toolTip().remove(QRegExp(QStringLiteral("\\s\\(.*\\)")));
    // append shortcut if it exists for action
    if (originalPlayAction->shortcut() == QKeySequence(0)) {
        m_playAction->setToolTip(strippedTooltip);
    } else {
        m_playAction->setToolTip(strippedTooltip + QStringLiteral(" (") + originalPlayAction->shortcut().toString() + QLatin1Char(')'));
    }
    m_playMenu->addAction(m_playAction);
    connect(m_playAction, &QAction::triggered, this, &Monitor::slotSwitchPlay);

    playButton->setMenu(m_playMenu);
    playButton->setPopupMode(QToolButton::MenuButtonPopup);
    m_toolbar->addWidget(playButton);
    m_toolbar->addAction(manager->getAction(QStringLiteral("monitor_seek_forward")));

    playButton->setDefaultAction(m_playAction);
    m_configMenu = new QMenu(i18n("Misc..."), this);

    if (id != Kdenlive::DvdMonitor) {
        if (id == Kdenlive::ClipMonitor) {
            m_markerMenu = new QMenu(i18n("Go to marker..."), this);
        } else {
            m_markerMenu = new QMenu(i18n("Go to guide..."), this);
        }
        m_markerMenu->setEnabled(false);
        m_configMenu->addMenu(m_markerMenu);
        connect(m_markerMenu, &QMenu::triggered, this, &Monitor::slotGoToMarker);
        m_forceSize = new KSelectAction(KoIconUtils::themedIcon(QStringLiteral("transform-scale")), i18n("Force Monitor Size"), this);
        QAction *fullAction = m_forceSize->addAction(QIcon(), i18n("Force 100%"));
        fullAction->setData(100);
        QAction *halfAction = m_forceSize->addAction(QIcon(), i18n("Force 50%"));
        halfAction->setData(50);
        QAction *freeAction = m_forceSize->addAction(QIcon(), i18n("Free Resize"));
        freeAction->setData(0);
        m_configMenu->addAction(m_forceSize);
        m_forceSize->setCurrentAction(freeAction);
        connect(m_forceSize, static_cast<void(KSelectAction::*)(QAction*)>(&KSelectAction::triggered), this, &Monitor::slotForceSize);
    }

    // Create Volume slider popup
    m_audioSlider = new QSlider(Qt::Vertical);
    m_audioSlider->setRange(0, 100);
    m_audioSlider->setValue(100);
    connect(m_audioSlider, &QSlider::valueChanged, this, &Monitor::slotSetVolume);
    auto *widgetslider = new QWidgetAction(this);
    widgetslider->setText(i18n("Audio volume"));
    widgetslider->setDefaultWidget(m_audioSlider);
    auto *menu = new QMenu(this);
    menu->addAction(widgetslider);

    m_audioButton = new QToolButton(this);
    m_audioButton->setMenu(menu);
    m_audioButton->setToolTip(i18n("Volume"));
    m_audioButton->setPopupMode(QToolButton::InstantPopup);
    QIcon icon;
    if (KdenliveSettings::volume() == 0) {
        icon = KoIconUtils::themedIcon(QStringLiteral("audio-volume-muted"));
    } else {
        icon = KoIconUtils::themedIcon(QStringLiteral("audio-volume-medium"));
    }
    m_audioButton->setIcon(icon);
    m_toolbar->addWidget(m_audioButton);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setLayout(layout);
    setMinimumHeight(200);

    connect(this, &Monitor::scopesClear, m_glMonitor, &GLWidget::releaseAnalyse, Qt::DirectConnection);
    connect(m_glMonitor, &GLWidget::analyseFrame, this, &Monitor::frameUpdated);
    connect(m_glMonitor, &GLWidget::audioSamplesSignal, this, &Monitor::audioSamplesSignal);

    if (id != Kdenlive::ClipMonitor) {
        // TODO: reimplement
        // connect(render, &Render::durationChanged, this, &Monitor::durationChanged);
        connect(m_glMonitor->getControllerProxy(), &MonitorProxy::zoneChanged, this, &Monitor::updateTimelineClipZone);
    } else {
        connect(m_glMonitor->getControllerProxy(), &MonitorProxy::zoneChanged, this, &Monitor::updateClipZone);
    }

    m_sceneVisibilityAction = new QAction(KoIconUtils::themedIcon(QStringLiteral("transform-crop")), i18n("Show/Hide edit mode"), this);
    m_sceneVisibilityAction->setCheckable(true);
    m_sceneVisibilityAction->setChecked(KdenliveSettings::showOnMonitorScene());
    connect(m_sceneVisibilityAction, &QAction::triggered, this, &Monitor::slotEnableEffectScene);
    m_toolbar->addAction(m_sceneVisibilityAction);

    m_zoomVisibilityAction = new QAction(KoIconUtils::themedIcon(QStringLiteral("zoom-in")), i18n("Zoom"), this);
    m_zoomVisibilityAction->setCheckable(true);
    connect(m_zoomVisibilityAction, &QAction::triggered, this, &Monitor::slotEnableSceneZoom);

    m_toolbar->addSeparator();
    m_timePos = new TimecodeDisplay(m_monitorManager->timecode(), this);
    m_toolbar->addWidget(m_timePos);

    auto *configButton = new QToolButton(m_toolbar);
    configButton->setIcon(KoIconUtils::themedIcon(QStringLiteral("kdenlive-menu")));
    configButton->setToolTip(i18n("Options"));
    configButton->setMenu(m_configMenu);
    configButton->setPopupMode(QToolButton::InstantPopup);
    m_toolbar->addWidget(configButton);
    if (m_recManager) {
        m_toolbar->addAction(m_recManager->switchAction());
    }
    /*QWidget *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    m_toolbar->addWidget(spacer);*/

    m_toolbar->addSeparator();
    int tm = 0;
    int bm = 0;
    m_toolbar->getContentsMargins(nullptr, &tm, nullptr, &bm);
    m_audioMeterWidget = new MonitorAudioLevel(m_glMonitor->profile(), m_toolbar->height() - tm - bm, this);
    m_toolbar->addWidget(m_audioMeterWidget);
    if (!m_audioMeterWidget->isValid) {
        KdenliveSettings::setMonitoraudio(0x01);
        m_audioMeterWidget->setVisibility(false);
    } else {
        m_audioMeterWidget->setVisibility((KdenliveSettings::monitoraudio() & m_id) != 0);
    }

    connect(m_timePos, SIGNAL(timeCodeEditingFinished()), this, SLOT(slotSeek()));
    layout->addWidget(m_toolbar);
    if (m_recManager) {
        layout->addWidget(m_recManager->toolbar());
    }

    // Load monitor overlay qml
    loadQmlScene(MonitorSceneDefault);

    // Info message widget
    m_infoMessage = new KMessageWidget(this);
    layout->addWidget(m_infoMessage);
    m_infoMessage->hide();
}

Monitor::~Monitor()
{
    delete m_audioMeterWidget;
    delete m_glMonitor;
    delete m_videoWidget;
    delete m_glWidget;
    delete m_timePos;
}

void Monitor::setOffsetX(int x)
{
    m_glMonitor->setOffsetX(x, m_horizontalScroll->maximum());
}

void Monitor::setOffsetY(int y)
{
    m_glMonitor->setOffsetY(y, m_verticalScroll->maximum());
}

void Monitor::slotGetCurrentImage(bool request)
{
    m_glMonitor->sendFrameForAnalysis = request;
    m_monitorManager->activateMonitor(m_id);
    refreshMonitorIfActive();
    if (request) {
        // Update analysis state
        QTimer::singleShot(500, m_monitorManager, &MonitorManager::checkScopes);
    } else {
        m_glMonitor->releaseAnalyse();
    }
}

void Monitor::slotAddEffect(const QStringList &effect)
{
    if (m_id == Kdenlive::ClipMonitor) {
        if (m_controller) {
            emit addMasterEffect(m_controller->AbstractProjectItem::clipId(), effect);
        }
    } else {
        emit addEffect(effect);
    }
}

void Monitor::refreshIcons()
{
    QList<QAction *> allMenus = this->findChildren<QAction *>();
    for (int i = 0; i < allMenus.count(); i++) {
        QAction *m = allMenus.at(i);
        QIcon ic = m->icon();
        if (ic.isNull() || ic.name().isEmpty()) {
            continue;
        }
        QIcon newIcon = KoIconUtils::themedIcon(ic.name());
        m->setIcon(newIcon);
    }
    QList<KDualAction *> allButtons = this->findChildren<KDualAction *>();
    for (int i = 0; i < allButtons.count(); i++) {
        KDualAction *m = allButtons.at(i);
        QIcon ic = m->activeIcon();
        if (ic.isNull() || ic.name().isEmpty()) {
            continue;
        }
        QIcon newIcon = KoIconUtils::themedIcon(ic.name());
        m->setActiveIcon(newIcon);
        ic = m->inactiveIcon();
        if (ic.isNull() || ic.name().isEmpty()) {
            continue;
        }
        newIcon = KoIconUtils::themedIcon(ic.name());
        m->setInactiveIcon(newIcon);
    }
}

QAction *Monitor::recAction()
{
    if (m_recManager) {
        return m_recManager->switchAction();
    }
    return nullptr;
}

void Monitor::slotLockMonitor(bool lock)
{
    m_monitorManager->lockMonitor(m_id, lock);
}

void Monitor::setupMenu(QMenu *goMenu, QMenu *overlayMenu, QAction *playZone, QAction *loopZone, QMenu *markerMenu, QAction *loopClip)
{
    delete m_contextMenu;
    m_contextMenu = new QMenu(this);
    m_contextMenu->addMenu(m_playMenu);
    if (goMenu) {
        m_contextMenu->addMenu(goMenu);
    }

    if (markerMenu) {
        m_contextMenu->addMenu(markerMenu);
        QList<QAction *> list = markerMenu->actions();
        for (int i = 0; i < list.count(); ++i) {
            if (list.at(i)->data().toString() == QLatin1String("edit_marker")) {
                m_editMarker = list.at(i);
                break;
            }
        }
    }

    m_playMenu->addAction(playZone);
    m_playMenu->addAction(loopZone);
    if (loopClip) {
        m_loopClipAction = loopClip;
        m_playMenu->addAction(loopClip);
    }

    // TODO: add save zone to timeline monitor when fixed
    m_contextMenu->addMenu(m_markerMenu);
    if (m_id == Kdenlive::ClipMonitor) {
        m_contextMenu->addAction(KoIconUtils::themedIcon(QStringLiteral("document-save")), i18n("Save zone"), this, SLOT(slotSaveZone()));
        QAction *extractZone =
            m_configMenu->addAction(KoIconUtils::themedIcon(QStringLiteral("document-new")), i18n("Extract Zone"), this, SLOT(slotExtractCurrentZone()));
        m_contextMenu->addAction(extractZone);
    }
    m_contextMenu->addAction(m_monitorManager->getAction(QStringLiteral("extract_frame")));
    m_contextMenu->addAction(m_monitorManager->getAction(QStringLiteral("extract_frame_to_project")));

    if (m_id == Kdenlive::ProjectMonitor) {
        m_multitrackView = m_contextMenu->addAction(KoIconUtils::themedIcon(QStringLiteral("view-split-left-right")), i18n("Multitrack view"), this,
                                                    SIGNAL(multitrackView(bool)));
        m_multitrackView->setCheckable(true);
        m_configMenu->addAction(m_multitrackView);
    } else if (m_id == Kdenlive::ClipMonitor) {
        QAction *setThumbFrame = m_contextMenu->addAction(KoIconUtils::themedIcon(QStringLiteral("document-new")), i18n("Set current image as thumbnail"), this,
                                                          SLOT(slotSetThumbFrame()));
        m_configMenu->addAction(setThumbFrame);
    }

    if (overlayMenu) {
        m_contextMenu->addMenu(overlayMenu);
    }

    QAction *overlayAudio = m_contextMenu->addAction(QIcon(), i18n("Overlay audio waveform"));
    overlayAudio->setCheckable(true);
    connect(overlayAudio, &QAction::toggled, m_glMonitor, &GLWidget::slotSwitchAudioOverlay);
    overlayAudio->setChecked(KdenliveSettings::displayAudioOverlay());

    QAction *switchAudioMonitor = m_configMenu->addAction(i18n("Show Audio Levels"), this, SLOT(slotSwitchAudioMonitor()));
    switchAudioMonitor->setCheckable(true);
    switchAudioMonitor->setChecked((KdenliveSettings::monitoraudio() & m_id) != 0);
    m_configMenu->addAction(overlayAudio);
    m_configMenu->addAction(m_zoomVisibilityAction);
    m_contextMenu->addAction(m_zoomVisibilityAction);
    // For some reason, the frame in QAbstracSpinBox (base class of TimeCodeDisplay) needs to be displayed once, then hidden
    // or it will never appear (supposed to appear on hover).
    m_timePos->setFrame(false);
}

void Monitor::slotGoToMarker(QAction *action)
{
    int pos = action->data().toInt();
    slotSeek(pos);
}

void Monitor::slotForceSize(QAction *a)
{
    int resizeType = a->data().toInt();
    int profileWidth = 320;
    int profileHeight = 200;
    if (resizeType > 0) {
        // calculate size
        QRect r = QApplication::desktop()->screenGeometry();
        profileHeight = m_glMonitor->profileSize().height() * resizeType / 100;
        profileWidth = m_glMonitor->profile()->dar() * profileHeight;
        if (profileWidth > r.width() * 0.8 || profileHeight > r.height() * 0.7) {
            // reset action to free resize
            const QList<QAction *> list = m_forceSize->actions();
            for (QAction *ac : list) {
                if (ac->data().toInt() == m_forceSizeFactor) {
                    m_forceSize->setCurrentAction(ac);
                    break;
                }
            }
            warningMessage(i18n("Your screen resolution is not sufficient for this action"));
            return;
        }
    }
    switch (resizeType) {
    case 100:
    case 50:
        // resize full size
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        m_videoWidget->setMinimumSize(profileWidth, profileHeight);
        m_videoWidget->setMaximumSize(profileWidth, profileHeight);
        setMinimumSize(QSize(profileWidth, profileHeight + m_toolbar->height() + m_glMonitor->getControllerProxy()->rulerHeight()));
        break;
    default:
        // Free resize
        m_videoWidget->setMinimumSize(profileWidth, profileHeight);
        m_videoWidget->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        setMinimumSize(QSize(profileWidth, profileHeight + m_toolbar->height() + m_glMonitor->getControllerProxy()->rulerHeight()));
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        break;
    }
    m_forceSizeFactor = resizeType;
    updateGeometry();
}

QString Monitor::getTimecodeFromFrames(int pos)
{
    return m_monitorManager->timecode().getTimecodeFromFrames(pos);
}

double Monitor::fps() const
{
    return m_monitorManager->timecode().fps();
}

Timecode Monitor::timecode() const
{
    return m_monitorManager->timecode();
}

void Monitor::updateMarkers()
{
    if (m_controller) {
        m_markerMenu->clear();
        QList<CommentedTime> markers = m_controller->getMarkerModel()->getAllMarkers();
        if (!markers.isEmpty()) {
            for (int i = 0; i < markers.count(); ++i) {
                int pos = (int)markers.at(i).time().frames(m_monitorManager->timecode().fps());
                QString position = m_monitorManager->timecode().getTimecode(markers.at(i).time()) + QLatin1Char(' ') + markers.at(i).comment();
                QAction *go = m_markerMenu->addAction(position);
                go->setData(pos);
            }
        }
        m_markerMenu->setEnabled(!m_markerMenu->isEmpty());
    }
}

void Monitor::setGuides(const QMap<double, QString> &guides)
{
    // TODO: load guides model
    m_markerMenu->clear();
    QMapIterator<double, QString> i(guides);
    QList<CommentedTime> guidesList;
    while (i.hasNext()) {
        i.next();
        CommentedTime timeGuide(GenTime(i.key()), i.value());
        guidesList << timeGuide;
        int pos = (int)timeGuide.time().frames(m_monitorManager->timecode().fps());
        QString position = m_monitorManager->timecode().getTimecode(timeGuide.time()) + QLatin1Char(' ') + timeGuide.comment();
        QAction *go = m_markerMenu->addAction(position);
        go->setData(pos);
    }
    // m_ruler->setMarkers(guidesList);
    m_markerMenu->setEnabled(!m_markerMenu->isEmpty());
    checkOverlay();
}

void Monitor::slotSeekToPreviousSnap()
{
    if (m_controller) {
        m_glMonitor->seek(getSnapForPos(true).frames(m_monitorManager->timecode().fps()));
    }
}

void Monitor::slotSeekToNextSnap()
{
    if (m_controller) {
        m_glMonitor->seek(getSnapForPos(false).frames(m_monitorManager->timecode().fps()));
    }
}

int Monitor::position()
{
    return m_glMonitor->getCurrentPos();
}

GenTime Monitor::getSnapForPos(bool previous)
{
    int frame = previous ? m_snaps->getPreviousPoint(m_glMonitor->getCurrentPos()) : m_snaps->getNextPoint(m_glMonitor->getCurrentPos());
    return GenTime(frame, pCore->getCurrentFps());
}

void Monitor::slotLoadClipZone(const QPoint &zone)
{
    m_glMonitor->getControllerProxy()->setZone(zone.x(), zone.y());
    checkOverlay();
}

void Monitor::slotSetZoneStart()
{
    m_glMonitor->getControllerProxy()->setZoneIn(m_glMonitor->getCurrentPos());
    if (m_controller) {
        m_controller->setZone(m_glMonitor->getControllerProxy()->zone());
    } else {
        // timeline
        emit timelineZoneChanged();
    }
    checkOverlay();
}

void Monitor::slotSetZoneEnd(bool discardLastFrame)
{
    int pos = m_glMonitor->getCurrentPos() - (discardLastFrame ? 1 : 0);
    m_glMonitor->getControllerProxy()->setZoneOut(pos);
    if (m_controller) {
        m_controller->setZone(m_glMonitor->getControllerProxy()->zone());
    }
    checkOverlay();
}

// virtual
void Monitor::mousePressEvent(QMouseEvent *event)
{
    m_monitorManager->activateMonitor(m_id);
    if ((event->button() & Qt::RightButton) == 0u) {
        if (m_glWidget->geometry().contains(event->pos())) {
            m_DragStartPosition = event->pos();
            event->accept();
        }
    } else if (m_contextMenu) {
        slotActivateMonitor();
        m_contextMenu->popup(event->globalPos());
        event->accept();
    }
    QWidget::mousePressEvent(event);
}

void Monitor::slotShowMenu(const QPoint pos)
{
    slotActivateMonitor();
    if (m_contextMenu) {
        m_contextMenu->popup(pos);
    }
}

void Monitor::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    if (m_glMonitor->zoom() > 0.0f) {
        float horizontal = float(m_horizontalScroll->value()) / float(m_horizontalScroll->maximum());
        float vertical = float(m_verticalScroll->value()) / float(m_verticalScroll->maximum());
        adjustScrollBars(horizontal, vertical);
    } else {
        m_horizontalScroll->hide();
        m_verticalScroll->hide();
    }
}

void Monitor::adjustScrollBars(float horizontal, float vertical)
{
    if (m_glMonitor->zoom() > 1.0f) {
        m_horizontalScroll->setPageStep(m_glWidget->width());
        m_horizontalScroll->setMaximum((int)((float)m_glMonitor->profileSize().width() * m_glMonitor->zoom()) - m_horizontalScroll->pageStep());
        m_horizontalScroll->setValue(qRound(horizontal * m_horizontalScroll->maximum()));
        emit m_horizontalScroll->valueChanged(m_horizontalScroll->value());
        m_horizontalScroll->show();
    } else {
        int max = (int)((float)m_glMonitor->profileSize().width() * m_glMonitor->zoom()) - m_glWidget->width();
        emit m_horizontalScroll->valueChanged(qRound(0.5 * max));
        m_horizontalScroll->hide();
    }

    if (m_glMonitor->zoom() > 1.0f) {
        m_verticalScroll->setPageStep(m_glWidget->height());
        m_verticalScroll->setMaximum((int)((float)m_glMonitor->profileSize().height() * m_glMonitor->zoom()) - m_verticalScroll->pageStep());
        m_verticalScroll->setValue((int)((float)m_verticalScroll->maximum()*vertical));
        emit m_verticalScroll->valueChanged(m_verticalScroll->value());
        m_verticalScroll->show();
    } else {
        int max = (int)((float)m_glMonitor->profileSize().height() * m_glMonitor->zoom()) - m_glWidget->height();
        emit m_verticalScroll->valueChanged(qRound(0.5 * max));
        m_verticalScroll->hide();
    }
}

void Monitor::setZoom()
{
    if (qFuzzyCompare(m_glMonitor->zoom(), 1.0f)) {
        m_horizontalScroll->hide();
        m_verticalScroll->hide();
        m_glMonitor->setOffsetX(m_horizontalScroll->value(), m_horizontalScroll->maximum());
        m_glMonitor->setOffsetY(m_verticalScroll->value(), m_verticalScroll->maximum());
    } else {
        adjustScrollBars(0.5f, 0.5f);
    }
}

void Monitor::slotSwitchFullScreen(bool minimizeOnly)
{
    // TODO: disable screensaver?
    if (!m_glWidget->isFullScreen() && !minimizeOnly) {
        // Check if we have a multiple monitor setup
        int monitors = QApplication::desktop()->screenCount();
        int screen = -1;
        if (monitors > 1) {
            QRect screenres;
            // Move monitor widget to the second screen (one screen for Kdenlive, the other one for the Monitor widget
            // int currentScreen = QApplication::desktop()->screenNumber(this);
            for (int i = 0; screen == -1 && i < QApplication::desktop()->screenCount(); i++) {
                if (i != QApplication::desktop()->screenNumber(this->parentWidget()->parentWidget())) {
                    screen = i;
                }
            }
        }
        m_qmlManager->enableAudioThumbs(false);
        m_glWidget->setParent(QApplication::desktop()->screen(screen));
        m_glWidget->move(QApplication::desktop()->screenGeometry(screen).bottomLeft());
        m_glWidget->showFullScreen();
    } else {
        m_glWidget->showNormal();
        m_qmlManager->enableAudioThumbs(true);
        QVBoxLayout *lay = (QVBoxLayout *)layout();
        lay->insertWidget(0, m_glWidget, 10);
    }
}

void Monitor::reparent()
{
    m_glWidget->setParent(nullptr);
    m_glWidget->showMinimized();
    m_glWidget->showNormal();
    QVBoxLayout *lay = (QVBoxLayout *)layout();
    lay->insertWidget(0, m_glWidget, 10);
}

// virtual
void Monitor::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_dragStarted) {
        event->ignore();
        return;
    }
    if (event->button() != Qt::RightButton) {
        if (m_glMonitor->geometry().contains(event->pos())) {
            if (isActive()) {
                slotPlay();
            } else {
                slotActivateMonitor();
            }
        } // else event->ignore(); //QWidget::mouseReleaseEvent(event);
    }
    m_dragStarted = false;
    event->accept();
    QWidget::mouseReleaseEvent(event);
}

void Monitor::slotStartDrag()
{
    if (m_id == Kdenlive::ProjectMonitor || m_controller == nullptr) {
        // dragging is only allowed for clip monitor
        return;
    }
    auto *drag = new QDrag(this);
    auto *mimeData = new QMimeData;

    QByteArray prodData;
    QPoint p = m_glMonitor->getControllerProxy()->zone();
    if (p.x() == -1 || p.y() == -1) {
        prodData = m_controller->AbstractProjectItem::clipId().toUtf8();
    } else {
        QStringList list;
        list.append(m_controller->AbstractProjectItem::clipId());
        list.append(QString::number(p.x()));
        list.append(QString::number(p.y()));
        prodData.append(list.join(QLatin1Char('/')).toUtf8());
    }
    mimeData->setData(QStringLiteral("kdenlive/producerslist"), prodData);
    drag->setMimeData(mimeData);
    /*QPixmap pix = m_currentClip->thumbnail();
    drag->setPixmap(pix);
    drag->setHotSpot(QPoint(0, 50));*/
    drag->start(Qt::MoveAction);
}

void Monitor::enterEvent(QEvent *event)
{
    m_qmlManager->enableAudioThumbs(true);
    QWidget::enterEvent(event);
}

void Monitor::leaveEvent(QEvent *event)
{
    m_qmlManager->enableAudioThumbs(false);
    QWidget::leaveEvent(event);
}

// virtual
void Monitor::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragStarted || m_controller == nullptr) {
        return;
    }

    if ((event->pos() - m_DragStartPosition).manhattanLength() < QApplication::startDragDistance()) {
        return;
    }

    {
        auto *drag = new QDrag(this);
        auto *mimeData = new QMimeData;
        m_dragStarted = true;
        QStringList list;
        list.append(m_controller->AbstractProjectItem::clipId());
        QPoint p = m_glMonitor->getControllerProxy()->zone();
        list.append(QString::number(p.x()));
        list.append(QString::number(p.y()));
        QByteArray clipData;
        clipData.append(list.join(QLatin1Char(';')).toUtf8());
        mimeData->setData(QStringLiteral("kdenlive/clip"), clipData);
        drag->setMimeData(mimeData);
        drag->start(Qt::MoveAction);
    }
    event->accept();
}

/*void Monitor::dragMoveEvent(QDragMoveEvent * event) {
    event->setDropAction(Qt::IgnoreAction);
    event->setDropAction(Qt::MoveAction);
    if (event->mimeData()->hasText()) {
        event->acceptProposedAction();
    }
}

Qt::DropActions Monitor::supportedDropActions() const {
    // returns what actions are supported when dropping
    return Qt::MoveAction;
}*/

QStringList Monitor::mimeTypes() const
{
    QStringList qstrList;
    // list of accepted MIME types for drop
    qstrList.append(QStringLiteral("kdenlive/clip"));
    return qstrList;
}

// virtual
void Monitor::wheelEvent(QWheelEvent *event)
{
    slotMouseSeek(event->delta(), event->modifiers());
    event->accept();
}

void Monitor::mouseDoubleClickEvent(QMouseEvent *event)
{
    slotSwitchFullScreen();
    event->accept();
}

void Monitor::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        slotSwitchFullScreen();
        event->accept();
        return;
    }
    if (m_glWidget->isFullScreen()) {
        event->ignore();
        emit passKeyPress(event);
        return;
    }
    QWidget::keyPressEvent(event);
}

void Monitor::slotMouseSeek(int eventDelta, uint modifiers)
{
    if ((modifiers & Qt::ControlModifier) != 0u) {
        int delta = m_monitorManager->timecode().fps();
        if (eventDelta > 0) {
            delta = 0 - delta;
        }
        m_glMonitor->seek(m_glMonitor->getCurrentPos() - delta);
    } else if ((modifiers & Qt::AltModifier) != 0u) {
        if (eventDelta >= 0) {
            emit seekToNextSnap();
        } else {
            emit seekToPreviousSnap();
        }
    } else {
        if (eventDelta >= 0) {
            slotForwardOneFrame();
        } else {
            slotRewindOneFrame();
        }
    }
}

void Monitor::slotSetThumbFrame()
{
    if (m_controller == nullptr) {
        return;
    }
    m_controller->setProducerProperty(QStringLiteral("kdenlive:thumbnailFrame"), m_glMonitor->getCurrentPos());
    emit refreshClipThumbnail(m_controller->AbstractProjectItem::clipId());
}

void Monitor::slotExtractCurrentZone()
{
    if (m_controller == nullptr) {
        return;
    }
    emit extractZone(m_controller->AbstractProjectItem::clipId());
}

std::shared_ptr<ProjectClip> Monitor::currentController() const
{
    return m_controller;
}

void Monitor::slotExtractCurrentFrame(QString frameName, bool addToProject)
{
    if (QFileInfo(frameName).fileName().isEmpty()) {
        // convenience: when extracting an image to be added to the project,
        // suggest a suitable image file name. In the project monitor, this
        // suggestion bases on the project file name; in the clip monitor,
        // the suggestion bases on the clip file name currently shown.
        // Finally, the frame number is added to this suggestion, prefixed
        // with "-f", so we get something like clip-f#.png.
        QString suggestedImageName =
            QFileInfo(currentController() ? currentController()->clipName()
                                          : pCore->currentDoc()->url().isValid() ? pCore->currentDoc()->url().fileName() : i18n("untitled"))
                .completeBaseName() +
            QStringLiteral("-f") + QString::number(m_glMonitor->getCurrentPos()).rightJustified(6, QLatin1Char('0')) + QStringLiteral(".png");
        frameName = QFileInfo(frameName, suggestedImageName).fileName();
    }

    QString framesFolder = KRecentDirs::dir(QStringLiteral(":KdenliveFramesFolder"));
    if (framesFolder.isEmpty()) {
        framesFolder = QDir::homePath();
    }
    QScopedPointer<QDialog> dlg(new QDialog(this));
    QScopedPointer<KFileWidget> fileWidget(new KFileWidget(QUrl::fromLocalFile(framesFolder), dlg.data()));
    dlg->setWindowTitle(addToProject ? i18n("Save Image") : i18n("Save Image to Project"));
    auto *layout = new QVBoxLayout;
    layout->addWidget(fileWidget.data());
    QCheckBox *b = nullptr;
    if (m_id == Kdenlive::ClipMonitor) {
        b = new QCheckBox(i18n("Export image using source resolution"), dlg.data());
        b->setChecked(KdenliveSettings::exportframe_usingsourceres());
        fileWidget->setCustomWidget(b);
    }
    fileWidget->setConfirmOverwrite(true);
    fileWidget->okButton()->show();
    fileWidget->cancelButton()->show();
    QObject::connect(fileWidget->okButton(), &QPushButton::clicked, fileWidget.data(), &KFileWidget::slotOk);
    QObject::connect(fileWidget.data(), &KFileWidget::accepted, fileWidget.data(), &KFileWidget::accept);
    QObject::connect(fileWidget.data(), &KFileWidget::accepted, dlg.data(), &QDialog::accept);
    QObject::connect(fileWidget->cancelButton(), &QPushButton::clicked, dlg.data(), &QDialog::reject);
    dlg->setLayout(layout);
    fileWidget->setMimeFilter(QStringList() << QStringLiteral("image/png"));
    fileWidget->setMode(KFile::File | KFile::LocalOnly);
    fileWidget->setOperationMode(KFileWidget::Saving);
    QUrl relativeUrl;
    relativeUrl.setPath(frameName);
#if KIO_VERSION >= QT_VERSION_CHECK(5, 33, 0)
    fileWidget->setSelectedUrl(relativeUrl);
#else
    fileWidget->setSelection(relativeUrl.toString());
#endif
    KSharedConfig::Ptr conf = KSharedConfig::openConfig();
    QWindow *handle = dlg->windowHandle();
    if ((handle != nullptr) && conf->hasGroup("FileDialogSize")) {
        KWindowConfig::restoreWindowSize(handle, conf->group("FileDialogSize"));
        dlg->resize(handle->size());
    }
    if (dlg->exec() == QDialog::Accepted) {
        QString selectedFile = fileWidget->selectedFile();
        if (!selectedFile.isEmpty()) {
            // Create Qimage with frame
            QImage frame;
            // check if we are using a proxy
            if ((m_controller != nullptr) && !m_controller->getProducerProperty(QStringLiteral("kdenlive:proxy")).isEmpty() &&
                m_controller->getProducerProperty(QStringLiteral("kdenlive:proxy")) != QLatin1String("-")) {
                // using proxy, use original clip url to get frame
                frame =
                    m_monitorController->extractFrame(m_glMonitor->getCurrentPos(), m_controller->getProducerProperty(QStringLiteral("kdenlive:originalurl")),
                                                      -1, -1, b != nullptr ? b->isChecked() : false);
            } else {
                frame = m_monitorController->extractFrame(m_glMonitor->getCurrentPos(), QString(), -1, -1, b != nullptr ? b->isChecked() : false);
            }
            frame.save(selectedFile);
            if (b != nullptr) {
                KdenliveSettings::setExportframe_usingsourceres(b->isChecked());
            }
            KRecentDirs::add(QStringLiteral(":KdenliveFramesFolder"), QUrl::fromLocalFile(selectedFile).adjusted(QUrl::RemoveFilename).toLocalFile());

            if (addToProject) {
                QStringList folderInfo = pCore->bin()->getFolderInfo();
                pCore->bin()->droppedUrls(QList<QUrl>() << QUrl::fromLocalFile(selectedFile), folderInfo);
            }
        }
    }
}

void Monitor::setTimePos(const QString &pos)
{
    m_timePos->setValue(pos);
    slotSeek();
}

void Monitor::slotSeek()
{
    slotSeek(m_timePos->getValue());
}

void Monitor::slotSeek(int pos)
{
    slotActivateMonitor();
    m_glMonitor->seek(pos);
}

void Monitor::checkOverlay(int pos)
{
    if (m_qmlManager->sceneType() != MonitorSceneDefault) {
        // we are not in main view, ignore
        return;
    }
    QString overlayText;
    if (pos == -1) {
        pos = m_timePos->getValue();
    }
    QPoint zone = m_glMonitor->getControllerProxy()->zone();
    std::shared_ptr<MarkerListModel> model;
    if (m_id == Kdenlive::ClipMonitor && m_controller) {
        model = m_controller->getMarkerModel();
    } else if (m_id == Kdenlive::ProjectMonitor && pCore->currentDoc()) {
        model = pCore->currentDoc()->getGuideModel();
    }

    if (model) {
        bool found = false;
        CommentedTime marker = model->getMarker(GenTime(pos, m_monitorManager->timecode().fps()), &found);
        if (!found) {
            if (pos == zone.x()) {
                overlayText = i18n("In Point");
            } else if (pos == zone.y()) {
                overlayText = i18n("Out Point");
            }
        } else {
            overlayText = marker.comment();
        }
    }
    m_glMonitor->getControllerProxy()->setMarkerComment(overlayText);
}

int Monitor::getZoneStart()
{
    return m_glMonitor->getControllerProxy()->zoneIn();
}

int Monitor::getZoneEnd()
{
    return m_glMonitor->getControllerProxy()->zoneOut();
}

void Monitor::slotZoneStart()
{
    slotActivateMonitor();
    m_glMonitor->getControllerProxy()->pauseAndSeek(m_glMonitor->getControllerProxy()->zoneIn());
}

void Monitor::slotZoneEnd()
{
    slotActivateMonitor();
    m_glMonitor->getControllerProxy()->pauseAndSeek(m_glMonitor->getControllerProxy()->zoneOut());
}

void Monitor::slotRewind(double speed)
{
    slotActivateMonitor();
    if (qFuzzyIsNull(speed)) {
        double currentspeed = m_glMonitor->playSpeed();
        if (currentspeed >= 0) {
            speed = -1;
        } else
            switch ((int)currentspeed) {
            case -1:
                speed = -2;
                break;
            case -2:
                speed = -3;
                break;
            case -3:
                speed = -5;
                break;
            default:
                speed = -8;
            }
    }
    m_glMonitor->switchPlay(true, speed);
    m_playAction->setActive(true);
}

void Monitor::slotForward(double speed)
{
    slotActivateMonitor();
    if (qFuzzyIsNull(speed)) {
        double currentspeed = m_glMonitor->playSpeed();
        if (currentspeed <= 0) {
            speed = 1;
        } else
            switch ((int)currentspeed) {
            case 1:
                speed = 2;
                break;
            case 2:
                speed = 3;
                break;
            case 3:
                speed = 5;
                break;
            default:
                speed = 8;
            }
    }
    m_glMonitor->switchPlay(true, speed);
    m_playAction->setActive(true);
}

void Monitor::slotRewindOneFrame(int diff)
{
    slotActivateMonitor();
    m_glMonitor->seek(m_glMonitor->getCurrentPos() - diff);
}

void Monitor::slotForwardOneFrame(int diff)
{
    slotActivateMonitor();
    m_glMonitor->seek(m_glMonitor->getCurrentPos() + diff);
}

void Monitor::seekCursor(int pos)
{
    Q_UNUSED(pos)
    // Deprecated shoud not be used, instead requestSeek
    /*if (m_ruler->slotNewValue(pos)) {
        m_timePos->setValue(pos);
        checkOverlay(pos);
        if (m_id != Kdenlive::ClipMonitor) {
            emit renderPosition(pos);
        }
    }*/
}

void Monitor::adjustRulerSize(int length, std::shared_ptr<MarkerListModel> markerModel)
{
    if (m_controller != nullptr) {
        m_glMonitor->setRulerInfo(length);
    } else {
        m_glMonitor->setRulerInfo(length, markerModel);
    }
    m_timePos->setRange(0, length);
    if (markerModel) {
        connect(markerModel.get(), SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &, const QVector<int> &)), this, SLOT(checkOverlay()));
        connect(markerModel.get(), SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SLOT(checkOverlay()));
        connect(markerModel.get(), SIGNAL(rowsRemoved(const QModelIndex &, int, int)), this, SLOT(checkOverlay()));
        if (m_controller) {
            markerModel->registerSnapModel(m_snaps);
        }
    }
}

void Monitor::stop()
{
    m_playAction->setActive(false);
    m_glMonitor->stop();
}

void Monitor::mute(bool mute, bool updateIconOnly)
{
    // TODO: we should set the "audio_off" property to 1 to mute the consumer instead of changing volume
    QIcon icon;
    if (mute || KdenliveSettings::volume() == 0) {
        icon = KoIconUtils::themedIcon(QStringLiteral("audio-volume-muted"));
    } else {
        icon = KoIconUtils::themedIcon(QStringLiteral("audio-volume-medium"));
    }
    m_audioButton->setIcon(icon);
    if (!updateIconOnly) {
        m_glMonitor->setVolume(mute ? 0 : (double)KdenliveSettings::volume() / 100.0);
    }
}

void Monitor::start()
{
    if (!isVisible() || !isActive()) {
        return;
    }
    m_glMonitor->startConsumer();
}

void Monitor::slotRefreshMonitor(bool visible)
{
    if (visible) {
        if (slotActivateMonitor()) {
            start();
        }
    }
}

void Monitor::refreshMonitorIfActive()
{
    if (isActive()) {
        m_glMonitor->requestRefresh();
    }
}

void Monitor::pause()
{
    if (!m_playAction->isActive()) {
        return;
    }
    slotActivateMonitor();
    m_glMonitor->switchPlay(false);
    m_playAction->setActive(false);
}

void Monitor::switchPlay(bool play)
{
    m_playAction->setActive(play);
    m_glMonitor->switchPlay(play);
}

void Monitor::slotSwitchPlay()
{
    slotActivateMonitor();
    m_glMonitor->switchPlay(m_playAction->isActive());
}

void Monitor::slotPlay()
{
    m_playAction->trigger();
}

void Monitor::slotPlayZone()
{
    slotActivateMonitor();
    bool ok = m_glMonitor->playZone();
    if (ok) {
        m_playAction->setActive(true);
    }
}

void Monitor::slotLoopZone()
{
    slotActivateMonitor();
    bool ok = m_glMonitor->playZone(true);
    if (ok) {
        m_playAction->setActive(true);
    }
}

void Monitor::slotLoopClip()
{
    slotActivateMonitor();
    bool ok = m_glMonitor->loopClip();
    if (ok) {
        m_playAction->setActive(true);
    }
}

void Monitor::updateClipProducer(Mlt::Producer *prod)
{
    if (m_glMonitor->setProducer(prod, isActive(), -1)) {
        prod->set_speed(1.0);
    }
}

void Monitor::updateClipProducer(const QString &playlist)
{
    Q_UNUSED(playlist)
    // TODO
    // Mlt::Producer *prod = new Mlt::Producer(*m_glMonitor->profile(), playlist.toUtf8().constData());
    // m_glMonitor->setProducer(prod, isActive(), render->seekFramePosition());
    m_glMonitor->switchPlay(true);
}

void Monitor::slotOpenClip(std::shared_ptr<ProjectClip> controller, int in, int out)
{
    Q_UNUSED(out)
    if (m_controller) {
        disconnect(m_controller->getMarkerModel().get(), SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &, const QVector<int> &)), this,
                   SLOT(checkOverlay()));
        disconnect(m_controller->getMarkerModel().get(), SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SLOT(checkOverlay()));
        disconnect(m_controller->getMarkerModel().get(), SIGNAL(rowsRemoved(const QModelIndex &, int, int)), this, SLOT(checkOverlay()));
    }
    m_controller = controller;
    loadQmlScene(MonitorSceneDefault);
    m_snaps.reset(new SnapModel());
    if (controller) {
        connect(m_controller->getMarkerModel().get(), SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &, const QVector<int> &)), this,
                SLOT(checkOverlay()));
        connect(m_controller->getMarkerModel().get(), SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SLOT(checkOverlay()));
        connect(m_controller->getMarkerModel().get(), SIGNAL(rowsRemoved(const QModelIndex &, int, int)), this, SLOT(checkOverlay()));

        if (m_recManager->toolbar()->isVisible()) {
            // we are in record mode, don't display clip
            return;
        }
        m_glMonitor->setRulerInfo(m_controller->frameDuration(), controller->getMarkerModel());
        m_timePos->setRange(0, m_controller->frameDuration());
        updateMarkers();
        connect(m_glMonitor->getControllerProxy(), &MonitorProxy::addSnap, this, &Monitor::addSnapPoint, Qt::DirectConnection);
        connect(m_glMonitor->getControllerProxy(), &MonitorProxy::removeSnap, this, &Monitor::removeSnapPoint, Qt::DirectConnection);
        m_glMonitor->getControllerProxy()->setZone(m_controller->zone());
        m_snaps->addPoint(m_controller->frameDuration());
        // Loading new clip / zone, stop if playing
        if (m_playAction->isActive()) {
            m_playAction->setActive(false);
        }
        m_glMonitor->setProducer(m_controller->originalProducer().get(), isActive(), in);
        m_audioMeterWidget->audioChannels = controller->audioInfo() ? controller->audioInfo()->channels() : 0;
        // hasEffects =  controller->hasEffects();
    } else {
        m_glMonitor->setProducer(nullptr, isActive());
        m_glMonitor->setAudioThumb();
        m_audioMeterWidget->audioChannels = 0;
    }
    if (slotActivateMonitor()) {
        start();
    }
    checkOverlay();
}

const QString Monitor::activeClipId()
{
    if (m_controller) {
        return m_controller->AbstractProjectItem::clipId();
    }
    return QString();
}

void Monitor::slotOpenDvdFile(const QString &file)
{
    // TODO
    Q_UNUSED(file)
    m_glMonitor->initializeGL();
    // render->loadUrl(file);
}

void Monitor::slotSaveZone()
{
    // TODO? or deprecate
    // render->saveZone(pCore->currentDoc()->projectDataFolder(), m_ruler->zone());
}

void Monitor::setCustomProfile(const QString &profile, const Timecode &tc)
{
    // TODO or deprecate
    Q_UNUSED(profile)
    m_timePos->updateTimeCode(tc);
    if (true) {
        return;
    }
    slotActivateMonitor();
    // render->prepareProfileReset(tc.fps());
    if (m_multitrackView) {
        m_multitrackView->setChecked(false);
    }
    // TODO: this is a temporary profile for DVD preview, it should not alter project profile
    // pCore->setCurrentProfile(profile);
    m_glMonitor->reloadProfile();
}

void Monitor::resetProfile()
{
    m_timePos->updateTimeCode(m_monitorManager->timecode());
    m_glMonitor->reloadProfile();
    m_glMonitor->rootObject()->setProperty("framesize", QRect(0, 0, m_glMonitor->profileSize().width(), m_glMonitor->profileSize().height()));
    double fps = m_monitorManager->timecode().fps();
    // Update drop frame info
    m_qmlManager->setProperty(QStringLiteral("dropped"), false);
    m_qmlManager->setProperty(QStringLiteral("fps"), QString::number(fps, 'g', 2));
}

/*void Monitor::saveSceneList(const QString &path, const QDomElement &info)
{
    if (render == nullptr) return;
    render->saveSceneList(path, info);
}*/

const QString Monitor::sceneList(const QString &root, const QString &fullPath)
{
    return m_glMonitor->sceneList(root, fullPath);
}

void Monitor::updateClipZone()
{
    if (m_controller == nullptr) {
        return;
    }
    m_controller->setZone(m_glMonitor->getControllerProxy()->zone());
}

void Monitor::updateTimelineClipZone()
{
    emit zoneUpdated(m_glMonitor->getControllerProxy()->zone());
}

void Monitor::switchDropFrames(bool drop)
{
    m_glMonitor->setDropFrames(drop);
}

void Monitor::switchMonitorInfo(int code)
{
    int currentOverlay;
    if (m_id == Kdenlive::ClipMonitor) {
        currentOverlay = KdenliveSettings::displayClipMonitorInfo();
        currentOverlay ^= code;
        KdenliveSettings::setDisplayClipMonitorInfo(currentOverlay);
    } else {
        currentOverlay = KdenliveSettings::displayProjectMonitorInfo();
        currentOverlay ^= code;
        KdenliveSettings::setDisplayProjectMonitorInfo(currentOverlay);
    }
    updateQmlDisplay(currentOverlay);
}

void Monitor::updateMonitorGamma()
{
    if (isActive()) {
        stop();
        m_glMonitor->updateGamma();
        start();
    } else {
        m_glMonitor->updateGamma();
    }
}

void Monitor::slotEditMarker()
{
    if (m_editMarker) {
        m_editMarker->trigger();
    }
}

void Monitor::updateTimecodeFormat()
{
    m_timePos->slotUpdateTimeCodeFormat();
    m_glMonitor->rootObject()->setProperty("timecode", m_timePos->displayText());
}

QPoint Monitor::getZoneInfo() const
{
    if (m_controller == nullptr) {
        return QPoint();
    }
    return m_controller->zone();
}

void Monitor::slotEnableSceneZoom(bool enable)
{
    m_qmlManager->setProperty(QStringLiteral("showToolbar"), enable);
}

void Monitor::slotEnableEffectScene(bool enable)
{
    KdenliveSettings::setShowOnMonitorScene(enable);
    MonitorSceneType sceneType = enable ? m_lastMonitorSceneType : MonitorSceneDefault;
    slotShowEffectScene(sceneType, true);
    if (enable) {
        emit seekPosition(m_glMonitor->getCurrentPos());
    }
}

void Monitor::slotShowEffectScene(MonitorSceneType sceneType, bool temporary)
{
    if (sceneType == MonitorSceneNone) {
        // We just want to revert to normal scene
        if (m_qmlManager->sceneType() == MonitorSceneSplit || m_qmlManager->sceneType() == MonitorSceneDefault) {
            // Ok, nothing to do
            return;
        }
        sceneType = MonitorSceneDefault;
    }
    if (!temporary) {
        m_lastMonitorSceneType = sceneType;
    }
    loadQmlScene(sceneType);
}

void Monitor::slotSeekToKeyFrame()
{
    if (m_qmlManager->sceneType() == MonitorSceneGeometry) {
        // Adjust splitter pos
        int kfr = m_glMonitor->rootObject()->property("requestedKeyFrame").toInt();
        emit seekToKeyframe(kfr);
    }
}

void Monitor::setUpEffectGeometry(const QRect &r, const QVariantList &list, const QVariantList &types)
{
    QQuickItem *root = m_glMonitor->rootObject();
    if (!root) {
        return;
    }
    if (!list.isEmpty()) {
        root->setProperty("centerPointsTypes", types);
        root->setProperty("centerPoints", list);
    }
    if (!r.isEmpty()) {
        root->setProperty("framesize", r);
    }
}

void Monitor::setEffectSceneProperty(const QString &name, const QVariant &value)
{
    QQuickItem *root = m_glMonitor->rootObject();
    if (!root) {
        return;
    }
    root->setProperty(name.toUtf8().constData(), value);
}

QRect Monitor::effectRect() const
{
    QQuickItem *root = m_glMonitor->rootObject();
    if (!root) {
        return QRect();
    }
    return root->property("framesize").toRect();
}

QVariantList Monitor::effectPolygon() const
{
    QQuickItem *root = m_glMonitor->rootObject();
    if (!root) {
        return QVariantList();
    }
    return root->property("centerPoints").toList();
}

QVariantList Monitor::effectRoto() const
{
    QQuickItem *root = m_glMonitor->rootObject();
    if (!root) {
        return QVariantList();
    }
    QVariantList points = root->property("centerPoints").toList();
    QVariantList controlPoints = root->property("centerPointsTypes").toList();
    // rotoscoping effect needs a list of
    QVariantList mix;
    mix.reserve(points.count() * 3);
    for (int i = 0; i < points.count(); i++) {
        mix << controlPoints.at(2 * i);
        mix << points.at(i);
        mix << controlPoints.at(2 * i + 1);
    }
    return mix;
}

void Monitor::setEffectKeyframe(bool enable)
{
    QQuickItem *root = m_glMonitor->rootObject();
    if (root) {
        root->setProperty("iskeyframe", enable);
    }
}

bool Monitor::effectSceneDisplayed(MonitorSceneType effectType)
{
    return m_qmlManager->sceneType() == effectType;
}

void Monitor::slotSetVolume(int volume)
{
    KdenliveSettings::setVolume(volume);
    QIcon icon;
    double renderVolume = m_glMonitor->volume();
    m_glMonitor->setVolume((double)volume / 100.0);
    if (renderVolume > 0 && volume > 0) {
        return;
    }
    if (volume == 0) {
        icon = KoIconUtils::themedIcon(QStringLiteral("audio-volume-muted"));
    } else {
        icon = KoIconUtils::themedIcon(QStringLiteral("audio-volume-medium"));
    }
    m_audioButton->setIcon(icon);
}

void Monitor::sendFrameForAnalysis(bool analyse)
{
    m_glMonitor->sendFrameForAnalysis = analyse;
}

void Monitor::updateAudioForAnalysis()
{
    m_glMonitor->updateAudioForAnalysis();
}

void Monitor::onFrameDisplayed(const SharedFrame &frame)
{
    m_monitorManager->frameDisplayed(frame);
    int position = frame.get_position();
    if (!m_glMonitor->checkFrameNumber(position)) {
        m_playAction->setActive(false);
    } /* else if (position >= m_length) {
         m_playAction->setActive(false);
     }*/
}

void Monitor::checkDrops(int dropped)
{
    if (m_droppedTimer.isValid()) {
        if (m_droppedTimer.hasExpired(1000)) {
            m_droppedTimer.invalidate();
            double fps = m_monitorManager->timecode().fps();
            if (dropped == 0) {
                // No dropped frames since last check
                m_qmlManager->setProperty(QStringLiteral("dropped"), false);
                m_qmlManager->setProperty(QStringLiteral("fps"), QString::number(fps, 'g', 2));
            } else {
                m_glMonitor->resetDrops();
                fps -= dropped;
                m_qmlManager->setProperty(QStringLiteral("dropped"), true);
                m_qmlManager->setProperty(QStringLiteral("fps"), QString::number(fps, 'g', 2));
                m_droppedTimer.start();
            }
        }
    } else if (dropped > 0) {
        // Start m_dropTimer
        m_glMonitor->resetDrops();
        m_droppedTimer.start();
    }
}

void Monitor::reloadProducer(const QString &id)
{
    if (!m_controller) {
        return;
    }
    if (m_controller->AbstractProjectItem::clipId() == id) {
        slotOpenClip(m_controller);
    }
}

QString Monitor::getMarkerThumb(GenTime pos)
{
    if (!m_controller) {
        return QString();
    }
    if (!m_controller->getClipHash().isEmpty()) {
        QString url = m_monitorManager->getCacheFolder(CacheThumbs)
                          .absoluteFilePath(m_controller->getClipHash() + QLatin1Char('#') +
                                            QString::number((int)pos.frames(m_monitorManager->timecode().fps())) + QStringLiteral(".png"));
        if (QFile::exists(url)) {
            return url;
        }
    }
    return QString();
}

const QString Monitor::projectFolder() const
{
    return m_monitorManager->getProjectFolder();
}

void Monitor::setPalette(const QPalette &p)
{
    QWidget::setPalette(p);
    QList<QToolButton *> allButtons = this->findChildren<QToolButton *>();
    for (int i = 0; i < allButtons.count(); i++) {
        QToolButton *m = allButtons.at(i);
        QIcon ic = m->icon();
        if (ic.isNull() || ic.name().isEmpty()) {
            continue;
        }
        QIcon newIcon = KoIconUtils::themedIcon(ic.name());
        m->setIcon(newIcon);
    }
    m_audioMeterWidget->refreshPixmap();
}

void Monitor::gpuError()
{
    qCWarning(KDENLIVE_LOG) << " + + + + Error initializing Movit GLSL manager";
    warningMessage(i18n("Cannot initialize Movit's GLSL manager, please disable Movit"), -1);
}

void Monitor::warningMessage(const QString &text, int timeout, const QList<QAction *> &actions)
{
    m_infoMessage->setMessageType(KMessageWidget::Warning);
    m_infoMessage->setText(text);
    for (QAction *action : actions) {
        m_infoMessage->addAction(action);
    }
    m_infoMessage->setCloseButtonVisible(true);
    m_infoMessage->animatedShow();
    if (timeout > 0) {
        QTimer::singleShot(timeout, m_infoMessage, &KMessageWidget::animatedHide);
    }
}

void Monitor::activateSplit()
{
    loadQmlScene(MonitorSceneSplit);
    if (slotActivateMonitor()) {
        start();
    }
}

void Monitor::slotSwitchCompare(bool enable)
{
    if (m_id == Kdenlive::ProjectMonitor) {
        if (enable) {
            if (m_qmlManager->sceneType() == MonitorSceneSplit) {
                // Split scene is already active
                return;
            }
            m_splitEffect = new Mlt::Filter(*profile(), "frei0r.alphagrad");
            if ((m_splitEffect != nullptr) && m_splitEffect->is_valid()) {
                m_splitEffect->set("0", 0.5);    // 0 is the Clip left parameter
                m_splitEffect->set("1", 0);      // 1 is gradient width
                m_splitEffect->set("2", -0.747); // 2 is tilt
            } else {
                // frei0r.scal0tilt is not available
                warningMessage(i18n("The alphagrad filter is required for that feature, please install frei0r and restart Kdenlive"));
                return;
            }
            emit createSplitOverlay(m_splitEffect);
            return;
        }
        // Delete temp track
        emit removeSplitOverlay();
        delete m_splitEffect;
        m_splitEffect = nullptr;
        loadQmlScene(MonitorSceneDefault);
        if (slotActivateMonitor()) {
            start();
        }

        return;
    }
    if (m_controller == nullptr || !m_controller->hasEffects()) {
        // disable split effect
        if (m_controller) {
            pCore->displayMessage(i18n("Clip has no effects"), InformationMessage);
        } else {
            pCore->displayMessage(i18n("Select a clip in project bin to compare effect"), InformationMessage);
        }
        return;
    }
    if (enable) {
        if (m_qmlManager->sceneType() == MonitorSceneSplit) {
            // Split scene is already active
            return;
        }
        buildSplitEffect(m_controller->masterProducer());
    } else if (m_splitEffect) {
        // TODO
        m_glMonitor->setProducer(m_controller->originalProducer().get(), isActive(), position());
        delete m_splitEffect;
        m_splitProducer = nullptr;
        m_splitEffect = nullptr;
        loadQmlScene(MonitorSceneDefault);
    }
    slotActivateMonitor();
}

void Monitor::buildSplitEffect(Mlt::Producer *original)
{
    qDebug() << "// BUILDING SPLIT EFFECT!!!";
    m_splitEffect = new Mlt::Filter(*profile(), "frei0r.alphagrad");
    if ((m_splitEffect != nullptr) && m_splitEffect->is_valid()) {
        m_splitEffect->set("0", 0.5);    // 0 is the Clip left parameter
        m_splitEffect->set("1", 0);      // 1 is gradient width
        m_splitEffect->set("2", -0.747); // 2 is tilt
    } else {
        // frei0r.scal0tilt is not available
        pCore->displayMessage(i18n("The alphagrad filter is required for that feature, please install frei0r and restart Kdenlive"), ErrorMessage);
        return;
    }
    QString splitTransition = TransitionsRepository::get()->getCompositingTransition();
    Mlt::Transition t(*profile(), splitTransition.toUtf8().constData());
    if (!t.is_valid()) {
        delete m_splitEffect;
        pCore->displayMessage(i18n("The cairoblend transition is required for that feature, please install frei0r and restart Kdenlive"), ErrorMessage);
        return;
    }
    Mlt::Tractor trac(*profile());
    // TODO: remove usage of Clip class
    Clip clp(*original);
    Mlt::Producer *clone = clp.clone();
    Clip clp2(*clone);
    clp2.deleteEffects();
    trac.set_track(*original, 0);
    trac.set_track(*clone, 1);
    clone->attach(*m_splitEffect);
    t.set("always_active", 1);
    trac.plant_transition(t, 0, 1);
    delete clone;
    delete original;
    m_splitProducer = new Mlt::Producer(trac.get_producer());
    m_glMonitor->setProducer(m_splitProducer, isActive(), position());
    loadQmlScene(MonitorSceneSplit);
}

QSize Monitor::profileSize() const
{
    return m_glMonitor->profileSize();
}

void Monitor::loadQmlScene(MonitorSceneType type)
{
    if (m_id == Kdenlive::DvdMonitor || type == m_qmlManager->sceneType()) {
        return;
    }
    bool sceneWithEdit = type == MonitorSceneGeometry || type == MonitorSceneCorners || type == MonitorSceneRoto;
    if ((m_sceneVisibilityAction != nullptr) && !m_sceneVisibilityAction->isChecked() && sceneWithEdit) {
        // User doesn't want effect scenes
        type = MonitorSceneDefault;
    }
    double ratio = (double)m_glMonitor->profileSize().width() / (int)(m_glMonitor->profileSize().height() * m_glMonitor->profile()->dar() + 0.5);
    m_qmlManager->setScene(m_id, type, m_glMonitor->profileSize(), ratio, m_glMonitor->displayRect(), m_glMonitor->zoom(), m_timePos->maximum());
    QQuickItem *root = m_glMonitor->rootObject();
    root->setProperty("showToolbar", m_zoomVisibilityAction->isChecked());
    connectQmlToolbar(root);
    switch (type) {
    case MonitorSceneSplit:
        QObject::connect(root, SIGNAL(qmlMoveSplit()), this, SLOT(slotAdjustEffectCompare()), Qt::UniqueConnection);
        break;
    case MonitorSceneGeometry:
    case MonitorSceneCorners:
    case MonitorSceneRoto:
        QObject::connect(root, SIGNAL(addKeyframe()), this, SIGNAL(addKeyframe()), Qt::UniqueConnection);
        QObject::connect(root, SIGNAL(seekToKeyframe()), this, SLOT(slotSeekToKeyFrame()), Qt::UniqueConnection);
        QObject::connect(root, SIGNAL(toolBarChanged(bool)), m_zoomVisibilityAction, SLOT(setChecked(bool)), Qt::UniqueConnection);
        break;
    case MonitorSceneRipple:
        QObject::connect(root, SIGNAL(doAcceptRipple(bool)), this, SIGNAL(acceptRipple(bool)), Qt::UniqueConnection);
        QObject::connect(root, SIGNAL(switchTrimMode(int)), this, SIGNAL(switchTrimMode(int)), Qt::UniqueConnection);
        break;
    case MonitorSceneDefault:
        QObject::connect(root, SIGNAL(editCurrentMarker()), this, SLOT(slotEditInlineMarker()), Qt::UniqueConnection);
        m_qmlManager->setProperty(QStringLiteral("timecode"), m_timePos->displayText());
        if (m_id == Kdenlive::ClipMonitor) {
            updateQmlDisplay(KdenliveSettings::displayClipMonitorInfo());
        } else if (m_id == Kdenlive::ProjectMonitor) {
            updateQmlDisplay(KdenliveSettings::displayProjectMonitorInfo());
        }
        break;
    default:
        break;
    }
    m_qmlManager->setProperty(QStringLiteral("fps"), QString::number(m_monitorManager->timecode().fps(), 'g', 2));
}

void Monitor::connectQmlToolbar(QQuickItem *root)
{
    QObject *button = root->findChild<QObject *>(QStringLiteral("fullScreen"));
    if (button) {
        QObject::connect(button, SIGNAL(clicked()), this, SLOT(slotSwitchFullScreen()), Qt::UniqueConnection);
    }
    // Normal monitor toolbar
    button = root->findChild<QObject *>(QStringLiteral("nextSnap"));
    if (button) {
        QObject::connect(button, SIGNAL(clicked()), this, SIGNAL(seekToNextSnap()), Qt::UniqueConnection);
    }
    button = root->findChild<QObject *>(QStringLiteral("prevSnap"));
    if (button) {
        QObject::connect(button, SIGNAL(clicked()), this, SIGNAL(seekToPreviousSnap()), Qt::UniqueConnection);
    }
    button = root->findChild<QObject *>(QStringLiteral("addMarker"));
    if (button) {
        QObject::connect(button, SIGNAL(clicked()), this, SIGNAL(addMarker()), Qt::UniqueConnection);
    }
    button = root->findChild<QObject *>(QStringLiteral("removeMarker"));
    if (button) {
        QObject::connect(button, SIGNAL(clicked()), this, SIGNAL(deleteMarker()), Qt::UniqueConnection);
    }

    // Effect monitor toolbar
    button = root->findChild<QObject *>(QStringLiteral("nextKeyframe"));
    if (button) {
        QObject::connect(button, SIGNAL(clicked()), this, SIGNAL(seekToNextKeyframe()), Qt::UniqueConnection);
    }
    button = root->findChild<QObject *>(QStringLiteral("prevKeyframe"));
    if (button) {
        QObject::connect(button, SIGNAL(clicked()), this, SIGNAL(seekToPreviousKeyframe()), Qt::UniqueConnection);
    }
    button = root->findChild<QObject *>(QStringLiteral("addKeyframe"));
    if (button) {
        QObject::connect(button, SIGNAL(clicked()), this, SIGNAL(addKeyframe()), Qt::UniqueConnection);
    }
    button = root->findChild<QObject *>(QStringLiteral("removeKeyframe"));
    if (button) {
        QObject::connect(button, SIGNAL(clicked()), this, SIGNAL(deleteKeyframe()), Qt::UniqueConnection);
    }
    button = root->findChild<QObject *>(QStringLiteral("zoomSlider"));
    if (button) {
        QObject::connect(button, SIGNAL(zoomChanged(double)), m_glMonitor, SLOT(slotZoomScene(double)), Qt::UniqueConnection);
    }
}

void Monitor::setQmlProperty(const QString &name, const QVariant &value)
{
    m_qmlManager->setProperty(name, value);
}

void Monitor::slotAdjustEffectCompare()
{
    QRect r = m_glMonitor->rect();
    double percent = 0.5;
    if (m_qmlManager->sceneType() == MonitorSceneSplit) {
        // Adjust splitter pos
        QQuickItem *root = m_glMonitor->rootObject();
        percent = 0.5 - ((root->property("splitterPos").toInt() - r.left() - r.width() / 2.0) / (double)r.width() / 2.0) / 0.75;
        // Store real frame percentage for resize events
        root->setProperty("realpercent", percent);
    }
    if (m_splitEffect) {
        m_splitEffect->set("0", percent);
    }
    m_glMonitor->refresh();
}

Mlt::Profile *Monitor::profile()
{
    return m_glMonitor->profile();
}

void Monitor::slotSwitchRec(bool enable)
{
    if (!m_recManager) {
        return;
    }
    if (enable) {
        m_toolbar->setVisible(false);
        m_recManager->toolbar()->setVisible(true);
    } else if (m_recManager->toolbar()->isVisible()) {
        m_recManager->stop();
        m_toolbar->setVisible(true);
        emit refreshCurrentClip();
    }
}

bool Monitor::startCapture(const QString &params, const QString &path, Mlt::Producer *p)
{
    // TODO
    m_controller = nullptr;
    if (false) { // render->updateProducer(p)) {
        m_glMonitor->reconfigureMulti(params, path, p->profile());
        return true;
    }
    return false;
}

bool Monitor::stopCapture()
{
    m_glMonitor->stopCapture();
    slotOpenClip(nullptr);
    m_glMonitor->reconfigure(profile());
    return true;
}

void Monitor::doKeyPressEvent(QKeyEvent *ev)
{
    keyPressEvent(ev);
}

void Monitor::slotEditInlineMarker()
{
    QQuickItem *root = m_glMonitor->rootObject();
    if (root) {
        std::shared_ptr<MarkerListModel> model;
        if (m_controller) {
            // We are editing a clip marker
            model = m_controller->getMarkerModel();
        } else {
            model = pCore->currentDoc()->getGuideModel();
        }
        QString newComment = root->property("markerText").toString();
        bool found = false;
        CommentedTime oldMarker = model->getMarker(m_timePos->gentime(), &found);
        if (!found || newComment == oldMarker.comment()) {
            // No change
            return;
        }
        oldMarker.setComment(newComment);
        model->addMarker(oldMarker.time(), oldMarker.comment(), oldMarker.markerType());
    }
}

void Monitor::prepareAudioThumb(int channels, QVariantList &audioCache)
{
    m_glMonitor->setAudioThumb(channels, audioCache);
}

void Monitor::slotUpdateQmlTimecode(const QString &tc)
{
    checkDrops(m_glMonitor->droppedFrames());
    m_glMonitor->rootObject()->setProperty("timecode", tc);
}

void Monitor::slotSwitchAudioMonitor()
{
    if (!m_audioMeterWidget->isValid) {
        KdenliveSettings::setMonitoraudio(0x01);
        m_audioMeterWidget->setVisibility(false);
        return;
    }
    int currentOverlay = KdenliveSettings::monitoraudio();
    currentOverlay ^= m_id;
    KdenliveSettings::setMonitoraudio(currentOverlay);
    if ((KdenliveSettings::monitoraudio() & m_id) != 0) {
        // We want to enable this audio monitor, so make monitor active
        slotActivateMonitor();
    }
    displayAudioMonitor(isActive());
}

void Monitor::displayAudioMonitor(bool isActive)
{
    bool enable = isActive && ((KdenliveSettings::monitoraudio() & m_id) != 0);
    if (enable) {
        connect(m_monitorManager, &MonitorManager::frameDisplayed, m_audioMeterWidget, &ScopeWidget::onNewFrame, Qt::UniqueConnection);
    } else {
        disconnect(m_monitorManager, &MonitorManager::frameDisplayed, m_audioMeterWidget, &ScopeWidget::onNewFrame);
    }
    m_audioMeterWidget->setVisibility((KdenliveSettings::monitoraudio() & m_id) != 0);
}

void Monitor::updateQmlDisplay(int currentOverlay)
{
    m_glMonitor->rootObject()->setVisible((currentOverlay & 0x01) != 0);
    m_glMonitor->rootObject()->setProperty("showMarkers", currentOverlay & 0x04);
    m_glMonitor->rootObject()->setProperty("showFps", currentOverlay & 0x20);
    m_glMonitor->rootObject()->setProperty("showTimecode", currentOverlay & 0x02);
    bool showTimecodeRelatedInfo = ((currentOverlay & 0x02) != 0) || ((currentOverlay & 0x20) != 0);
    m_timePos->sendTimecode(showTimecodeRelatedInfo);
    if (showTimecodeRelatedInfo) {
        connect(m_timePos, &TimecodeDisplay::emitTimeCode, this, &Monitor::slotUpdateQmlTimecode, Qt::UniqueConnection);
    } else {
        disconnect(m_timePos, &TimecodeDisplay::emitTimeCode, this, &Monitor::slotUpdateQmlTimecode);
    }
    m_glMonitor->rootObject()->setProperty("showSafezone", currentOverlay & 0x08);
    m_glMonitor->rootObject()->setProperty("showAudiothumb", currentOverlay & 0x10);
}

void Monitor::clearDisplay()
{
    m_glMonitor->clear();
}

void Monitor::panView(QPoint diff)
{
    // Only pan if scrollbars are visible
    if (m_horizontalScroll->isVisible()) {
        m_horizontalScroll->setValue(m_horizontalScroll->value() + diff.x());
    }
    if (m_verticalScroll->isVisible()) {
        m_verticalScroll->setValue(m_verticalScroll->value() + diff.y());
    }
}

void Monitor::requestSeek(int pos)
{
    m_glMonitor->seek(pos);
}

void Monitor::setProducer(Mlt::Producer *producer, int pos)
{
    m_glMonitor->setProducer(producer, isActive(), pos);
}

void Monitor::reconfigure()
{
    m_glMonitor->reconfigure();
}

void Monitor::slotSeekPosition(int pos)
{
    emit seekPosition(pos);
    m_timePos->setValue(pos);
    checkOverlay();
}

void Monitor::slotStart()
{
    slotActivateMonitor();
    m_glMonitor->switchPlay(false);
    m_glMonitor->seek(0);
}

void Monitor::slotEnd()
{
    slotActivateMonitor();
    m_glMonitor->switchPlay(false);
    m_glMonitor->seek(m_glMonitor->duration());
}

void Monitor::addSnapPoint(int pos)
{
    m_snaps->addPoint(pos);
}

void Monitor::removeSnapPoint(int pos)
{
    m_snaps->removePoint(pos);
}

