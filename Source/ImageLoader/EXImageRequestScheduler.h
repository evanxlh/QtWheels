//
//  EXImageRequestScheduler.h
//
//  Created by evanxlh on 2025/6/29.
//

#pragma once

#include "EXImageLoaderGlobal.h"
#include "EXImageRequest.h"
#include "EXImageLoaderConfiguration.h"
#include <QObject>
#include <QQueue>
#include <QHash>
#include <QThreadPool>
#include <QTimer>
#include <QReadWriteLock>
#include <QElapsedTimer>
#include <QStorageInfo>

class EXImageRequestScheduler : public QObject
{
    Q_OBJECT
public:
    explicit EXImageRequestScheduler(EXImageLoaderConfiguration *config, QObject *parent = nullptr);
    ~EXImageRequestScheduler();

    void enqueueRequest(EXImageRequest* request);
    void cancelRequest(const QString& requestId);
    void cancelAll();

    int activeRequestCount() const;
    int queuedRequestCount() const;

    EXImageLoaderConfiguration* config() const { return m_config; }

signals:
    void requestStarted(const QString& requestId);
    void requestFinished(const QString& requestId);
    void requestCancelled(const QString& requestId);
    void requestQueueOverflow();
    void concurrentCountChanged(int count);

private slots:
    void processNextRequest();
    void adjustThreadPool();
    void onConfigChanged();

private:
    void initializeThreadPool();
    double calculateSystemLoad() const;

    EXImageLoaderConfiguration* m_config;
    QThreadPool m_threadPool;
    QHash<ImageLoader::Priority, QQueue<EXImageRequest*>> m_requestQueues;
    QHash<QString, EXImageRequest*> m_activeRequests;
    mutable QReadWriteLock m_lock;
    QTimer* m_adjustTimer = nullptr;
    int m_currentConcurrent = 0;
    int m_totalQueued = 0;
};
