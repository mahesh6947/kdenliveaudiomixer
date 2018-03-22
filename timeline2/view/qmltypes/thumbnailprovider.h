/*
 * Copyright (c) 2013-2016 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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

#ifndef THUMBNAILPROVIDER_H
#define THUMBNAILPROVIDER_H

#include <KImageCache>
#include <QCache>
#include <QQuickImageProvider>
#include <mlt++/MltProducer.h>
#include <mlt++/MltProfile.h>
#include <memory>

class ThumbnailProvider : public QQuickImageProvider
{
public:
    explicit ThumbnailProvider();
    virtual ~ThumbnailProvider();
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize);
    void resetProject();

private:
    QString cacheKey(Mlt::Properties &properties, const QString &service, const QString &resource, const QString &hash, int frameNumber);
    QImage makeThumbnail(std::shared_ptr<Mlt::Producer> producer, int frameNumber, const QSize &requestedSize);
    QCache<int, Mlt::Producer> m_producers;
};

#endif // THUMBNAILPROVIDER_H
