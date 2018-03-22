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

#include "assettreemodel.hpp"
#include "abstractmodel/treeitem.hpp"
#include "effects/effectsrepository.hpp"
#include "transitions/transitionsrepository.hpp"

int AssetTreeModel::nameCol = 0;
int AssetTreeModel::idCol = 1;
int AssetTreeModel::typeCol = 2;
int AssetTreeModel::favCol = 3;

AssetTreeModel::AssetTreeModel(QObject *parent)
    : AbstractTreeModel(parent)
{
}

QHash<int, QByteArray> AssetTreeModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "identifier";
    roles[NameRole] = "name";
    return roles;
}

QString AssetTreeModel::getName(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QString();
    }
    std::shared_ptr<TreeItem> item = getItemById((int)index.internalId());
    if (item->depth() == 1) {
        return item->dataColumn(0).toString();
    }
    return item->dataColumn(AssetTreeModel::nameCol).toString();
}

QString AssetTreeModel::getDescription(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QString();
    }
    std::shared_ptr<TreeItem> item = getItemById((int)index.internalId());
    if (item->depth() == 1) {
        return QString();
    }
    auto id = item->dataColumn(AssetTreeModel::idCol).toString();
    if (EffectsRepository::get()->exists(id)) {
        return EffectsRepository::get()->getDescription(id);
    }
    if (TransitionsRepository::get()->exists(id)) {
        return TransitionsRepository::get()->getDescription(id);
    }
    return QString();
}

QVariant AssetTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    std::shared_ptr<TreeItem> item = getItemById((int)index.internalId());
    if (role == IdRole) {
        return item->dataColumn(AssetTreeModel::idCol);
    }

    if (role != NameRole) {
        return QVariant();
    }
    return item->dataColumn(index.column());
}
