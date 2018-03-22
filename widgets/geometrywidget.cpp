/***************************************************************************
 *   Copyright (C) 2017 by Jean-Baptiste Mardelle                                  *
 *   This file is part of Kdenlive. See www.kdenlive.org.                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) version 3 or any later version accepted by the       *
 *   membership of KDE e.V. (or its successor approved  by the membership  *
 *   of KDE e.V.), which shall act as a proxy defined in Section 14 of     *
 *   version 3 of the license.                                             *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "geometrywidget.h"
#include "doublewidget.h"
#include "dragvalue.h"
#include "core.h"
#include "monitor/monitor.h"
#include "utils/KoIconUtils.h"

#include <QGridLayout>
#include <KLocalizedString>

GeometryWidget::GeometryWidget(Monitor *monitor, QPair<int, int> range, const QRect &rect, const QSize frameSize, bool useRatioLock, bool useOpacity, QWidget *parent)
    : QWidget(parent)
    , m_min(range.first)
    , m_max(range.second)
    , m_active(false)
    , m_monitor(monitor)
    , m_opacity(nullptr)
{
    Q_UNUSED(useRatioLock)
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    m_defaultSize = pCore->getCurrentFrameSize();
    m_sourceSize = frameSize.isValid() ? frameSize : m_defaultSize;
    /*QString paramName = i18n(paramTag.toUtf8().data());
    QString comment = m_model->data(ix, AssetParameterModel::CommentRole).toString();
    if (!comment.isEmpty()) {
        comment = i18n(comment.toUtf8().data());
    }*/

    auto *horLayout = new QHBoxLayout;
    horLayout->setSpacing(2);
    m_spinX = new DragValue(i18nc("x axis position", "X"), 0, 0, -99000, 99000, -1, QString(), false, this);
    connect(m_spinX, &DragValue::valueChanged, this, &GeometryWidget::slotAdjustRectKeyframeValue);
    horLayout->addWidget(m_spinX);

    m_spinY = new DragValue(i18nc("y axis position", "Y"), 0, 0, -99000, 99000, -1, QString(), false, this);
    connect(m_spinY, &DragValue::valueChanged, this, &GeometryWidget::slotAdjustRectKeyframeValue);
    horLayout->addWidget(m_spinY);

    m_spinWidth = new DragValue(i18nc("Frame width", "W"), m_defaultSize.width(), 0, 1, 99000, -1, QString(), false, this);
    connect(m_spinWidth, &DragValue::valueChanged, this, &GeometryWidget::slotAdjustRectWidth);
    horLayout->addWidget(m_spinWidth);

    // Lock ratio stuff
    m_lockRatio = new QAction(KoIconUtils::themedIcon(QStringLiteral("link")), i18n("Lock aspect ratio"), this);
    m_lockRatio->setCheckable(true);
    connect(m_lockRatio, &QAction::triggered, this, &GeometryWidget::slotLockRatio);
    auto *ratioButton = new QToolButton;
    ratioButton->setDefaultAction(m_lockRatio);
    horLayout->addWidget(ratioButton);

    m_spinHeight = new DragValue(i18nc("Frame height", "H"), m_defaultSize.height(), 0, 1, 99000, -1, QString(), false, this);
    connect(m_spinHeight, &DragValue::valueChanged, this, &GeometryWidget::slotAdjustRectHeight);
    horLayout->addWidget(m_spinHeight);
    horLayout->addStretch(10);

    auto *horLayout2 = new QHBoxLayout;
    horLayout2->setSpacing(2);
    m_spinSize = new DragValue(i18n("Size"), 100, 2, 1, 99000, -1, i18n("%"), false, this);
    m_spinSize->setStep(5);
    connect(m_spinSize, &DragValue::valueChanged, this, &GeometryWidget::slotResize);
    horLayout2->addWidget(m_spinSize);

    if (useOpacity) {
        m_opacity = new DragValue(i18n("Opacity"), 100, 0, 0, 100, -1, i18n("%"), true, this);
        connect(m_opacity, &DragValue::valueChanged, this, &GeometryWidget::slotAdjustRectKeyframeValue);
        horLayout2->addWidget(m_opacity);
    }
    horLayout2->addStretch(10);

    // Build buttons
    m_originalSize = new QAction(KoIconUtils::themedIcon(QStringLiteral("zoom-original")), i18n("Adjust to original size"), this);
    connect(m_originalSize, &QAction::triggered, this, &GeometryWidget::slotAdjustToSource);
    m_originalSize->setCheckable(true);
    QAction *adjustSize = new QAction(KoIconUtils::themedIcon(QStringLiteral("zoom-fit-best")), i18n("Adjust and center in frame"), this);
    connect(adjustSize, &QAction::triggered, this, &GeometryWidget::slotAdjustToFrameSize);
    QAction *fitToWidth = new QAction(KoIconUtils::themedIcon(QStringLiteral("zoom-fit-width")), i18n("Fit to width"), this);
    connect(fitToWidth, &QAction::triggered, this, &GeometryWidget::slotFitToWidth);
    QAction *fitToHeight = new QAction(KoIconUtils::themedIcon(QStringLiteral("zoom-fit-height")), i18n("Fit to height"), this);
    connect(fitToHeight, &QAction::triggered, this, &GeometryWidget::slotFitToHeight);

    QAction *alignleft = new QAction(KoIconUtils::themedIcon(QStringLiteral("kdenlive-align-left")), i18n("Align left"), this);
    connect(alignleft, &QAction::triggered, this, &GeometryWidget::slotMoveLeft);
    QAction *alignhcenter = new QAction(KoIconUtils::themedIcon(QStringLiteral("kdenlive-align-hor")), i18n("Center horizontally"), this);
    connect(alignhcenter, &QAction::triggered, this, &GeometryWidget::slotCenterH);
    QAction *alignright = new QAction(KoIconUtils::themedIcon(QStringLiteral("kdenlive-align-right")), i18n("Align right"), this);
    connect(alignright, &QAction::triggered, this, &GeometryWidget::slotMoveRight);
    QAction *aligntop = new QAction(KoIconUtils::themedIcon(QStringLiteral("kdenlive-align-top")), i18n("Align top"), this);
    connect(aligntop, &QAction::triggered, this, &GeometryWidget::slotMoveTop);
    QAction *alignvcenter = new QAction(KoIconUtils::themedIcon(QStringLiteral("kdenlive-align-vert")), i18n("Center vertically"), this);
    connect(alignvcenter, &QAction::triggered, this, &GeometryWidget::slotCenterV);
    QAction *alignbottom = new QAction(KoIconUtils::themedIcon(QStringLiteral("kdenlive-align-bottom")), i18n("Align bottom"), this);
    connect(alignbottom, &QAction::triggered, this, &GeometryWidget::slotMoveBottom);

    auto *alignLayout = new QHBoxLayout;
    alignLayout->setSpacing(0);
    auto *alignButton = new QToolButton;
    alignButton->setDefaultAction(alignleft);
    alignButton->setAutoRaise(true);
    alignLayout->addWidget(alignButton);

    alignButton = new QToolButton;
    alignButton->setDefaultAction(alignhcenter);
    alignButton->setAutoRaise(true);
    alignLayout->addWidget(alignButton);

    alignButton = new QToolButton;
    alignButton->setDefaultAction(alignright);
    alignButton->setAutoRaise(true);
    alignLayout->addWidget(alignButton);

    alignButton = new QToolButton;
    alignButton->setDefaultAction(aligntop);
    alignButton->setAutoRaise(true);
    alignLayout->addWidget(alignButton);

    alignButton = new QToolButton;
    alignButton->setDefaultAction(alignvcenter);
    alignButton->setAutoRaise(true);
    alignLayout->addWidget(alignButton);

    alignButton = new QToolButton;
    alignButton->setDefaultAction(alignbottom);
    alignButton->setAutoRaise(true);
    alignLayout->addWidget(alignButton);

    alignButton = new QToolButton;
    alignButton->setDefaultAction(m_originalSize);
    alignButton->setAutoRaise(true);
    alignLayout->addWidget(alignButton);

    alignButton = new QToolButton;
    alignButton->setDefaultAction(adjustSize);
    alignButton->setAutoRaise(true);
    alignLayout->addWidget(alignButton);

    alignButton = new QToolButton;
    alignButton->setDefaultAction(fitToWidth);
    alignButton->setAutoRaise(true);
    alignLayout->addWidget(alignButton);

    alignButton = new QToolButton;
    alignButton->setDefaultAction(fitToHeight);
    alignButton->setAutoRaise(true);
    alignLayout->addWidget(alignButton);
    alignLayout->addStretch(10);

    layout->addLayout(horLayout);
    layout->addLayout(alignLayout);
    layout->addLayout(horLayout2);
    slotUpdateGeometryRect(rect);
    adjustSizeValue();
}

void GeometryWidget::slotAdjustToSource()
{
    m_spinWidth->blockSignals(true);
    m_spinHeight->blockSignals(true);
    m_spinWidth->setValue((int)(m_sourceSize.width() / pCore->getCurrentSar() + 0.5), false);
    m_spinHeight->setValue(m_sourceSize.height(), false);
    m_spinWidth->blockSignals(false);
    m_spinHeight->blockSignals(false);
    slotAdjustRectKeyframeValue();
    if (m_lockRatio->isChecked()) {
        m_monitor->setEffectSceneProperty(QStringLiteral("lockratio"),
                                          m_originalSize->isChecked() ? (double)m_sourceSize.width() / m_sourceSize.height()
                                                                      : (double)m_defaultSize.width() / m_defaultSize.height());
    }
}
void GeometryWidget::slotAdjustToFrameSize()
{
    double monitorDar = pCore->getCurrentDar();
    double sourceDar = m_sourceSize.width() / m_sourceSize.height();
    m_spinWidth->blockSignals(true);
    m_spinHeight->blockSignals(true);
    if (sourceDar > monitorDar) {
        // Fit to width
        double factor = (double)m_defaultSize.width() / m_sourceSize.width() * pCore->getCurrentSar();
        m_spinHeight->setValue((int)(m_sourceSize.height() * factor + 0.5));
        m_spinWidth->setValue(m_defaultSize.width());
        // Center
        m_spinY->blockSignals(true);
        m_spinY->setValue((m_defaultSize.height() - m_spinHeight->value()) / 2);
        m_spinY->blockSignals(false);
    } else {
        // Fit to height
        double factor = (double)m_defaultSize.height() / m_sourceSize.height();
        m_spinHeight->setValue(m_defaultSize.height());
        m_spinWidth->setValue((int)(m_sourceSize.width() / pCore->getCurrentSar() * factor + 0.5));
        // Center
        m_spinX->blockSignals(true);
        m_spinX->setValue((m_defaultSize.width() - m_spinWidth->value()) / 2);
        m_spinX->blockSignals(false);
    }
    m_spinWidth->blockSignals(false);
    m_spinHeight->blockSignals(false);
    slotAdjustRectKeyframeValue();
}

void GeometryWidget::slotFitToWidth()
{
    double factor = (double)m_defaultSize.width() / m_sourceSize.width() * pCore->getCurrentSar();
    m_spinWidth->blockSignals(true);
    m_spinHeight->blockSignals(true);
    m_spinHeight->setValue((int)(m_sourceSize.height() * factor + 0.5));
    m_spinWidth->setValue(m_defaultSize.width());
    m_spinWidth->blockSignals(false);
    m_spinHeight->blockSignals(false);
    slotAdjustRectKeyframeValue();
}
void GeometryWidget::slotFitToHeight()
{
    double factor = (double)m_defaultSize.height() / m_sourceSize.height();
    m_spinWidth->blockSignals(true);
    m_spinHeight->blockSignals(true);
    m_spinHeight->setValue(m_defaultSize.height());
    m_spinWidth->setValue((int)(m_sourceSize.width() / pCore->getCurrentSar() * factor + 0.5));
    m_spinWidth->blockSignals(false);
    m_spinHeight->blockSignals(false);
    slotAdjustRectKeyframeValue();
}
void GeometryWidget::slotResize(double value)
{
    m_spinWidth->blockSignals(true);
    m_spinHeight->blockSignals(true);
    int w = m_originalSize->isChecked() ? m_sourceSize.width() : m_defaultSize.width();
    int h = m_originalSize->isChecked() ? m_sourceSize.height() : m_defaultSize.height();
    m_spinWidth->setValue(w * value / 100.0);
    m_spinHeight->setValue(h * value / 100.0);
    m_spinWidth->blockSignals(false);
    m_spinHeight->blockSignals(false);
    slotAdjustRectKeyframeValue();
}

/** @brief Moves the rect to the left frame border (x position = 0). */
void GeometryWidget::slotMoveLeft()
{
    m_spinX->setValue(0);
}
/** @brief Centers the rect horizontally. */
void GeometryWidget::slotCenterH()
{
    m_spinX->setValue((m_defaultSize.width() - m_spinWidth->value()) / 2);
}
/** @brief Moves the rect to the right frame border (x position = frame width - rect width). */
void GeometryWidget::slotMoveRight()
{
    m_spinX->setValue(m_defaultSize.width() - m_spinWidth->value());
}

/** @brief Moves the rect to the top frame border (y position = 0). */
void GeometryWidget::slotMoveTop()
{
    m_spinY->setValue(0);
}

/** @brief Centers the rect vertically. */
void GeometryWidget::slotCenterV()
{
    m_spinY->setValue((m_defaultSize.height() - m_spinHeight->value()) / 2);
}

/** @brief Moves the rect to the bottom frame border (y position = frame height - rect height). */
void GeometryWidget::slotMoveBottom()
{
    m_spinY->setValue(m_defaultSize.height() - m_spinHeight->value());
}

/** @brief Un/Lock aspect ratio for size in effect parameter. */
void GeometryWidget::slotLockRatio()
{
    QAction *lockRatio = qobject_cast<QAction *>(QObject::sender());
    if (lockRatio->isChecked()) {
        m_monitor->setEffectSceneProperty(QStringLiteral("lockratio"),
                                          m_originalSize->isChecked() ? (double)m_sourceSize.width() / m_sourceSize.height()
                                                                      : (double)m_defaultSize.width() / m_defaultSize.height());
    } else {
        m_monitor->setEffectSceneProperty(QStringLiteral("lockratio"), -1);
    }
}
void GeometryWidget::slotAdjustRectHeight()
{
    if (m_lockRatio->isChecked()) {
        m_spinWidth->blockSignals(true);
        if (m_originalSize->isChecked()) {
            m_spinWidth->setValue((int)(m_spinHeight->value() * m_sourceSize.width() / m_sourceSize.height() + 0.5));
        } else {
            m_spinWidth->setValue((int)(m_spinHeight->value() * m_defaultSize.width() / m_defaultSize.height() + 0.5));
        }
        m_spinWidth->blockSignals(false);
    }
    adjustSizeValue();
    slotAdjustRectKeyframeValue();
}

void GeometryWidget::slotAdjustRectWidth()
{
    if (m_lockRatio->isChecked()) {
        m_spinHeight->blockSignals(true);
        if (m_originalSize->isChecked()) {
            m_spinHeight->setValue((int)(m_spinWidth->value() * m_sourceSize.height() / m_sourceSize.width() + 0.5));
        } else {
            m_spinHeight->setValue((int)(m_spinWidth->value() * m_defaultSize.height() / m_defaultSize.width() + 0.5));
        }
        m_spinHeight->blockSignals(false);
    }
    adjustSizeValue();
    slotAdjustRectKeyframeValue();
}

void GeometryWidget::adjustSizeValue()
{
    double size;
    if ((double) m_spinWidth->value() / m_spinHeight->value() < pCore->getCurrentDar()) {
        if (m_originalSize->isChecked()) {
            size = m_spinWidth->value() * 100.0 / m_sourceSize.width();
        } else {
            size = m_spinWidth->value() * 100.0 / m_defaultSize.width();
        }
    } else {
        if (m_originalSize->isChecked()) {
            size = m_spinHeight->value() * 100.0 / m_sourceSize.height();
        } else {
            size = m_spinHeight->value() * 100.0 / m_defaultSize.height();
        }
    }
    m_spinSize->blockSignals(true);
    m_spinSize->setValue(size);
    m_spinSize->blockSignals(false);
}

void GeometryWidget::slotAdjustRectKeyframeValue()
{
    QRect rect(m_spinX->value(), m_spinY->value(), m_spinWidth->value(), m_spinHeight->value());
    m_monitor->setUpEffectGeometry(rect);
    emit valueChanged(getValue());
}

void GeometryWidget::slotUpdateGeometryRect(const QRect r)
{
    if (!r.isValid()) {
        return;
    }
    m_spinX->blockSignals(true);
    m_spinY->blockSignals(true);
    m_spinWidth->blockSignals(true);
    m_spinHeight->blockSignals(true);
    m_spinX->setValue(r.x());
    m_spinY->setValue(r.y());
    m_spinWidth->setValue(r.width());
    m_spinHeight->setValue(r.height());
    m_spinX->blockSignals(false);
    m_spinY->blockSignals(false);
    m_spinWidth->blockSignals(false);
    m_spinHeight->blockSignals(false);
    m_monitor->setUpEffectGeometry(r);
    //slotAdjustRectKeyframeValue();
    emit valueChanged(getValue());
    //setupMonitor();
}

void GeometryWidget::setValue(const QRect r, double opacity)
{
    if (!r.isValid()) {
        return;
    }
    m_spinX->blockSignals(true);
    m_spinY->blockSignals(true);
    m_spinWidth->blockSignals(true);
    m_spinHeight->blockSignals(true);
    m_spinX->setValue(r.x());
    m_spinY->setValue(r.y());
    m_spinWidth->setValue(r.width());
    m_spinHeight->setValue(r.height());
    if (m_opacity) {
        m_opacity->blockSignals(true);
        m_opacity->setValue((int) (opacity * 100));
        m_opacity->blockSignals(false);
    }
    m_spinX->blockSignals(false);
    m_spinY->blockSignals(false);
    m_spinWidth->blockSignals(false);
    m_spinHeight->blockSignals(false);
    m_monitor->setUpEffectGeometry(r);
}


const QString GeometryWidget::getValue() const
{
    if (m_opacity) {
        return QStringLiteral("%1 %2 %3 %4 %5").arg(m_spinX->value()).arg(m_spinY->value()).arg(m_spinWidth->value()).arg( m_spinHeight->value()).arg(m_opacity->value() / 100.0);
    }
    return QStringLiteral("%1 %2 %3 %4").arg(m_spinX->value()).arg(m_spinY->value()).arg(m_spinWidth->value()).arg( m_spinHeight->value());
}

void GeometryWidget::connectMonitor(bool activate)
{
    if (m_active == activate) {
        return;
    }
    m_active = activate;
    if (activate) {
        connect(m_monitor, &Monitor::effectChanged, this, &GeometryWidget::slotUpdateGeometryRect, Qt::UniqueConnection);
        QRect rect(m_spinX->value(), m_spinY->value(), m_spinWidth->value(), m_spinHeight->value());
        m_monitor->setUpEffectGeometry(rect);
    } else {
        m_monitor->setEffectKeyframe(false);
        disconnect(m_monitor, &Monitor::effectChanged, this, &GeometryWidget::slotUpdateGeometryRect);
    }
}

void GeometryWidget::slotSetRange(QPair <int, int> range)
{
    m_min = range.first;
    m_max = range.second;
}
