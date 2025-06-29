//
//  EXImageLoaderConfiguration.cpp
//
//  Created by evanxlh on 2025/6/29.
//

#include "EXImageLoaderConfiguration.h"


EXImageLoaderConfiguration::EXImageLoaderConfiguration(QObject *parent)
    : QObject(parent)
{
}

int EXImageLoaderConfiguration::maxConcurrent() const
{
    return m_maxConcurrent;
}

void EXImageLoaderConfiguration::setMaxConcurrent(int count)
{
    if (m_maxConcurrent != count) {
        m_maxConcurrent = count;
        emit maxConcurrentChanged(count);
    }
}

int EXImageLoaderConfiguration::queueCapacity() const
{
    return m_queueCapacity;
}

void EXImageLoaderConfiguration::setQueueCapacity(int capacity)
{
    if (m_queueCapacity != capacity) {
        m_queueCapacity = capacity;
        emit queueCapacityChanged(capacity);
    }
}

bool EXImageLoaderConfiguration::adaptiveScaling() const
{
    return m_adaptiveScaling;
}

void EXImageLoaderConfiguration::setAdaptiveScaling(bool enabled)
{
    if (m_adaptiveScaling != enabled) {
        m_adaptiveScaling = enabled;
        emit adaptiveScalingChanged(enabled);
    }
}
