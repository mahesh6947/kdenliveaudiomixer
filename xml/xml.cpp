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

#include "xml.hpp"
#include <QDebug>

// static
QString Xml::getSubTagContent(const QDomElement &element, const QString &tagName)
{
    QVector<QDomNode> nodeList = getDirectChildrenByTagName(element, tagName);
    if (!nodeList.isEmpty()) {
        if (nodeList.size() > 1) {
            QString str;
            QTextStream stream(&str);
            element.save(stream, 4);
            qDebug() << "Warning: " << str << "provides several " << tagName << ". We keep only first one.";
        }
        QString content = nodeList.at(0).toElement().text();
        return content;
    }
    return QString();
}

QVector<QDomNode> Xml::getDirectChildrenByTagName(const QDomElement &element, const QString &tagName)
{
    auto children = element.childNodes();
    QVector<QDomNode> result;
    for (int i = 0; i < children.count(); ++i) {
        if (children.item(i).isNull() || !children.item(i).isElement()) {
            continue;
        }
        QDomElement child = children.item(i).toElement();
        if (child.tagName() == tagName) {
            result.push_back(child);
        }
    }
    return result;
}

QString Xml::getTagContentByAttribute(const QDomElement &element, const QString &tagName, const QString &attribute, const QString &value,
                                      const QString &defaultReturn, bool directChildren)
{
    QDomNodeList nodes;
    if (directChildren) {
        nodes = element.childNodes();
    } else {
        nodes = element.elementsByTagName(tagName);
    }
    for (int i = 0; i < nodes.count(); ++i) {
        auto current = nodes.item(i);
        if (current.isNull() || !current.isElement()) {
            continue;
        }
        auto elem = current.toElement();
        if (elem.tagName() == tagName && elem.hasAttribute(attribute)) {
            if (elem.attribute(attribute) == value) {
                return elem.text();
            }
        }
    }
    return defaultReturn;
}

void Xml::addXmlProperties(QDomElement &element, const std::unordered_map<QString, QString> &properties)
{
    for (const auto &p : properties) {
        QDomElement prop = element.ownerDocument().createElement(QStringLiteral("property"));
        prop.setAttribute(QStringLiteral("name"), p.first);
        QDomText value = element.ownerDocument().createTextNode(p.second);
        prop.appendChild(value);
        element.appendChild(prop);
    }
}

void Xml::addXmlProperties(QDomElement &element, const QMap<QString, QString> &properties)
{
    QMapIterator<QString, QString> i(properties);
    while (i.hasNext()) {
        i.next();
        QDomElement prop = element.ownerDocument().createElement(QStringLiteral("property"));
        prop.setAttribute(QStringLiteral("name"), i.key());
        QDomText value = element.ownerDocument().createTextNode(i.value());
        prop.appendChild(value);
        element.appendChild(prop);
    }
}

QString Xml::getXmlProperty(const QDomElement &element, const QString &propertyName, const QString &defaultReturn)
{
    return Xml::getTagContentByAttribute(element, QStringLiteral("property"), QStringLiteral("name"), propertyName, defaultReturn, false);
}
