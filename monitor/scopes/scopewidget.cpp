/*
 * Copyright (c) 2015 Meltytech, LLC
 * Author: Brian Matherly <code@brianmatherly.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "scopewidget.h"
#include "kdenlive_debug.h"
#include <QtConcurrent>

ScopeWidget::ScopeWidget(QWidget *parent)
    : QWidget(parent)
    , m_queue(3, DataQueue<SharedFrame>::OverflowModeDiscardOldest)
    , m_future()
    , m_refreshPending(false)
    , m_mutex(QMutex::NonRecursive)
    , m_forceRefresh(false)
    , m_size(0, 0)
{
    // qCDebug(KDENLIVE_LOG) << "begin" << m_future.isFinished();
    // qCDebug(KDENLIVE_LOG) << "end";
}

ScopeWidget::~ScopeWidget() = default;

void ScopeWidget::onNewFrame(const SharedFrame &frame)
{
    m_queue.push(frame);
    requestRefresh();
}

void ScopeWidget::requestRefresh()
{
    if (m_future.isFinished()) {
        m_future = QtConcurrent::run(this, &ScopeWidget::refreshInThread);
    } else {
        m_refreshPending = true;
    }
}

void ScopeWidget::refreshInThread()
{
    if (m_size.isEmpty()) {
        return;
    }

    m_mutex.lock();
    QSize size = m_size;
    bool full = m_forceRefresh;
    m_forceRefresh = false;
    m_mutex.unlock();

    m_refreshPending = false;
    refreshScope(size, full);
    // Tell the GUI thread that the refresh is complete.
    QMetaObject::invokeMethod(this, "onRefreshThreadComplete", Qt::QueuedConnection);
}

void ScopeWidget::onRefreshThreadComplete()
{
    update();
    if (m_refreshPending) {
        requestRefresh();
    }
}

void ScopeWidget::resizeEvent(QResizeEvent *)
{
    m_mutex.lock();
    m_size = size();
    m_mutex.unlock();
    requestRefresh();
}

void ScopeWidget::changeEvent(QEvent *)
{
    m_mutex.lock();
    m_forceRefresh = true;
    m_mutex.unlock();
    requestRefresh();
}
