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

#include "effectfilter.hpp"
#include "abstractmodel/treeitem.hpp"
#include "assets/assetlist/model/assettreemodel.hpp"
#include "effecttreemodel.hpp"

EffectFilter::EffectFilter(QObject *parent)
    : AssetFilter(parent)
{
    m_type_enabled = false;
}

void EffectFilter::setFilterType(bool enabled, EffectType type)
{
    m_type_enabled = enabled;
    m_type_value = type;
    invalidateFilter();
}

bool EffectFilter::filterType(const std::shared_ptr<TreeItem> &item) const
{
    if (!m_type_enabled) {
        return true;
    }
    auto itemType = item->dataColumn(AssetTreeModel::typeCol).value<EffectType>();
    return itemType == m_type_value;
}

bool EffectFilter::applyAll(std::shared_ptr<TreeItem> item) const
{
    return filterName(item) && filterType(item);
}
