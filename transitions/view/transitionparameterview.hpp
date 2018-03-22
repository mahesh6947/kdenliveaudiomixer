/***************************************************************************
 *   Copyright (C) 2017 by Nicolas Carion                                  *
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

#ifndef TRANSITIONPARAMETERVIEW_H
#define TRANSITIONPARAMETERVIEW_H

#include "assets/model/assetparametermodel.hpp"
#include <QQuickWidget>

/* @brief This class is the view of the properties of a transition

 */

class TransitionParameterView : public QQuickWidget
{
    Q_OBJECT

public:
    TransitionParameterView(QWidget *parent = nullptr);

    /* @brief Set the current model to be displayed */
    void setModel(const std::shared_ptr<AssetParameterModel> &model);

protected:
    std::shared_ptr<AssetParameterModel> m_model;
};

#endif
