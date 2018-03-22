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

#ifndef ENCODINGPROFILESDIALOG_H
#define ENCODINGPROFILESDIALOG_H

#include <KConfigGroup>

#include "definitions.h"
#include "ui_manageencodingprofile_ui.h"

class EncodingProfilesDialog : public QDialog, Ui::ManageEncodingProfile_UI
{
    Q_OBJECT

public:
    explicit EncodingProfilesDialog(int profileType, QWidget *parent = nullptr);
    ~EncodingProfilesDialog();

private slots:
    void slotLoadProfiles();
    void slotShowParams();
    void slotDeleteProfile();
    void slotAddProfile();
    void slotEditProfile();

private:
    KConfig *m_configFile;
    KConfigGroup *m_configGroup;
};

#endif
