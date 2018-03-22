/***************************************************************************
 *   Copyright (C) 2011 by Till Theato (root@ttill.de)                     *
 *   Copyright (C) 2017 by Nicolas Carion                                  *
 *   This file is part of Kdenlive (www.kdenlive.org).                     *
 *                                                                         *
 *   Kdenlive is free software: you can redistribute it and/or modify      *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation, either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Kdenlive is distributed in the hope that it will be useful,           *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with Kdenlive.  If not, see <http://www.gnu.org/licenses/>.     *
 ***************************************************************************/

#ifndef KEYFRAMEWIDGET_H
#define KEYFRAMEWIDGET_H

#include "abstractparamwidget.hpp"

#include "definitions.h"
#include <QPersistentModelIndex>
#include <memory>
#include <unordered_map>

class AssetParameterModel;
class DoubleWidget;
class KeyframeView;
class KeyframeModelList;
class QVBoxLayout;
class QToolButton;
class TimecodeDisplay;
class KSelectAction;

class KeyframeWidget : public AbstractParamWidget
{
    Q_OBJECT

public:
    explicit KeyframeWidget(std::shared_ptr<AssetParameterModel> model, QModelIndex index, QWidget *parent = nullptr);
    ~KeyframeWidget();

    /* @brief Add a new parameter to be managed using the same keyframe viewer */
    void addParameter(const QPersistentModelIndex &index);
    int getPosition() const;
    void addKeyframe(int pos = -1);

    void updateTimecodeFormat();

public slots:
    void slotSetRange(QPair<int, int> range) override;
    void slotRefresh() override;
    /** @brief intialize qml overlay
     */
    void slotInitMonitor(bool active) override;

public slots:
    void slotSetPosition(int pos = -1, bool update = true);

private slots:
    /* brief Update the value of the widgets to reflect keyframe change */
    void slotRefreshParams();
    void slotAtKeyframe(bool atKeyframe, bool singleKeyframe);
    void monitorSeek(int pos);
    void slotEditKeyframeType(QAction *action);

private:
    QVBoxLayout *m_lay;
    std::shared_ptr<KeyframeModelList> m_keyframes;

    KeyframeView *m_keyframeview;
    QToolButton *m_buttonAddDelete;
    QToolButton *m_buttonPrevious;
    QToolButton *m_buttonNext;
    KSelectAction *m_selectType;
    TimecodeDisplay *m_time;
    void connectMonitor(bool active);
    std::unordered_map<QPersistentModelIndex, QWidget *> m_parameters;

};

#endif
