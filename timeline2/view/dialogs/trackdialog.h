/***************************************************************************
 *   Copyright (C) 2008 by Jean-Baptiste Mardelle (jb@kdenlive.org)        *
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

#ifndef TRACKDIALOG2_H
#define TRACKDIALOG2_H

#include "timeline2/model/timelineitemmodel.hpp"
#include "ui_addtrack_ui.h"

class TrackDialog : public QDialog, public Ui::AddTrack_UI
{
    Q_OBJECT

public:
    explicit TrackDialog(std::shared_ptr<TimelineItemModel> model, int trackIndex = -1, QWidget *parent = nullptr, bool deleteMode = false);
    /** @brief: returns the selected track's trackId
     */
    int selectedTrack() const;
    /** @brief: returns true if we want to insert an audio track
     */
    bool addAudioTrack() const;
    /** @brief: returns the newly created track name
    */
    const QString trackName() const;
    
private slots:
    void updateName(bool audioTrack);
    
private:
    int m_audioCount;
    int m_videoCount;
};

#endif
