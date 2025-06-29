//
//  EXImageLoaderConfiguration.h
//
//  Created by evanxlh on 2025/6/29.
//

#pragma once

#include <QObject>

class EXImageLoaderConfiguration : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int maxConcurrent READ maxConcurrent WRITE setMaxConcurrent NOTIFY maxConcurrentChanged)
    Q_PROPERTY(int queueCapacity READ queueCapacity WRITE setQueueCapacity NOTIFY queueCapacityChanged)
    Q_PROPERTY(bool adaptiveScaling READ adaptiveScaling WRITE setAdaptiveScaling NOTIFY adaptiveScalingChanged)

public:
    explicit EXImageLoaderConfiguration(QObject *parent = nullptr);

    int maxConcurrent() const;
    void setMaxConcurrent(int count);

    int queueCapacity() const;
    void setQueueCapacity(int capacity);

    bool adaptiveScaling() const;
    void setAdaptiveScaling(bool enabled);

signals:
    void maxConcurrentChanged(int count);
    void queueCapacityChanged(int capacity);
    void adaptiveScalingChanged(bool enabled);

private:
    int m_maxConcurrent = 8;
    int m_queueCapacity = 100;
    bool m_adaptiveScaling = true;
};
