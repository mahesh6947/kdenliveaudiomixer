/***************************************************************************
 *   Copyright (C) 2012 by Simon Andreas Eugster (simon.eu@gmail.com)      *
 *   This file is part of kdenlive. See www.kdenlive.org.                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "audioCorrelationInfo.h"

AudioCorrelationInfo::AudioCorrelationInfo(int mainSize, int subSize)
    : m_mainSize(mainSize)
    , m_subSize(subSize)
    , m_max(-1)
{
    m_correlationVector = new qint64[m_mainSize + m_subSize + 1];
}

AudioCorrelationInfo::~AudioCorrelationInfo()
{
    delete[] m_correlationVector;
}

int AudioCorrelationInfo::size() const
{
    return m_mainSize + m_subSize + 1;
}

void AudioCorrelationInfo::setMax(qint64 max)
{
    m_max = max;
}

qint64 AudioCorrelationInfo::max() const
{
    if (m_max <= 0) {
        int width = size();
        qint64 max = 0;
        for (int i = 0; i < width; ++i) {
            if (m_correlationVector[i] > max) {
                max = m_correlationVector[i];
            }
        }
        Q_ASSERT(max > 0);
        return max;
    }
    return m_max;
}

int AudioCorrelationInfo::maxIndex() const
{
    qint64 max = 0;
    int index = 0;
    int width = size();

    for (int i = 0; i < width; ++i) {
        if (m_correlationVector[i] > max) {
            max = m_correlationVector[i];
            index = i;
        }
    }

    return index;
}

qint64 *AudioCorrelationInfo::correlationVector()
{
    return m_correlationVector;
}

QImage AudioCorrelationInfo::toImage(int height) const
{
    int width = size();
    qint64 maxVal = max();

    QImage img(width, height, QImage::Format_ARGB32);
    img.fill(qRgb(255, 255, 255));

    if (maxVal == 0) {
        return img;
    }

    for (int x = 0; x < width; ++x) {
        int val = img.height() * m_correlationVector[x] / maxVal;
        for (int y = img.height() - 1; y > img.height() - val - 1; --y) {
            img.setPixel(x, y, qRgb(50, 50, 50));
        }
    }

    return img;
}
