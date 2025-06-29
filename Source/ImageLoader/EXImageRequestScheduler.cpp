//
//  EXImageRequestScheduler.cpp
//
//  Created by evanxlh on 2025/6/29.
//

#include "EXImageRequestScheduler.h"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QStorageInfo>
#include <QDebug>
#include <algorithm>

EXImageRequestScheduler::EXImageRequestScheduler(EXImageLoaderConfiguration *config, QObject *parent)
    : QObject(parent), m_config(config)
{
    initializeThreadPool();

    connect(m_config, &EXImageLoaderConfiguration::maxConcurrentChanged,
            this, &EXImageRequestScheduler::onConfigChanged);
    connect(m_config, &EXImageLoaderConfiguration::adaptiveScalingChanged,
            this, &EXImageRequestScheduler::onConfigChanged);

    m_adjustTimer = new QTimer(this);
    connect(m_adjustTimer, &QTimer::timeout, this, &EXImageRequestScheduler::adjustThreadPool);
    m_adjustTimer->start(5000);
}

EXImageRequestScheduler::~EXImageRequestScheduler()
{
    cancelAll();
    m_threadPool.waitForDone();
}

void EXImageRequestScheduler::initializeThreadPool()
{
    int maxThreads = m_config->maxConcurrent();

    if (m_config->adaptiveScaling()) {
        int cpuCores = QThread::idealThreadCount();
        if (cpuCores > 0) {
            maxThreads = qMin(maxThreads, cpuCores * 2);
        }
    }

    maxThreads = qMax(2, maxThreads);
    m_threadPool.setMaxThreadCount(maxThreads);
    qDebug() << "EXImageRequestScheduler initialized with max threads:" << maxThreads;
}

void EXImageRequestScheduler::enqueueRequest(EXImageRequest* request)
{
    QWriteLocker locker(&m_lock);

    if (m_totalQueued >= m_config->queueCapacity()) {
        qWarning() << "Request queue overflow! Max capacity:" << m_config->queueCapacity();
        delete request;
        emit requestQueueOverflow();
        return;
    }

    auto& queue = m_requestQueues[request->priority()];
    for (EXImageRequest* existing : queue) {
        if (existing->isSameRequest(request)) {
            qDebug() << "Duplicate request skipped:" << request->requestId();
            delete request;
            return;
        }
    }

    queue.enqueue(request);
    m_totalQueued++;

    QMetaObject::invokeMethod(this, "processNextRequest", Qt::QueuedConnection);
}

void EXImageRequestScheduler::cancelRequest(const QString& requestId)
{
    QWriteLocker locker(&m_lock);

    if (EXImageRequest* request = m_activeRequests.value(requestId)) {
        request->cancel();
        m_activeRequests.remove(requestId);
        m_currentConcurrent = m_activeRequests.size();
        emit requestCancelled(requestId);
        emit concurrentCountChanged(m_currentConcurrent);
        return;
    }

    for (auto& queue : m_requestQueues) {
        for (auto it = queue.begin(); it != queue.end(); ++it) {
            if ((*it)->requestId() == requestId) {
                delete *it;
                queue.erase(it);
                m_totalQueued--;
                emit requestCancelled(requestId);
                return;
            }
        }
    }
}

void EXImageRequestScheduler::cancelAll()
{
    QWriteLocker locker(&m_lock);

    for (EXImageRequest* request : m_activeRequests) {
        request->cancel();
    }
    m_activeRequests.clear();
    m_currentConcurrent = 0;
    emit concurrentCountChanged(0);

    for (auto& queue : m_requestQueues) {
        while (!queue.isEmpty()) {
            delete queue.dequeue();
        }
    }
    m_totalQueued = 0;
}

int EXImageRequestScheduler::activeRequestCount() const
{
    QReadLocker locker(&m_lock);
    return m_currentConcurrent;
}

int EXImageRequestScheduler::queuedRequestCount() const
{
    QReadLocker locker(&m_lock);
    return m_totalQueued;
}

void EXImageRequestScheduler::processNextRequest()
{
    // QWriteLocker locker(&m_lock);

    if (m_currentConcurrent >= m_threadPool.maxThreadCount()) {
        return;
    }

    for (int p = static_cast<int>(ImageLoader::Priority::VeryHigh);
         p >= static_cast<int>(ImageLoader::Priority::VeryLow); --p) {

        auto priority = static_cast<ImageLoader::Priority>(p);
        auto& queue = m_requestQueues[priority];

        if (!queue.isEmpty()) {
            EXImageRequest* request = queue.dequeue();
            m_totalQueued--;

            m_activeRequests.insert(request->requestId(), request);
            m_currentConcurrent = m_activeRequests.size();

            connect(request, &EXImageRequest::finished, this, [this, request] {
                // QWriteLocker lock(&m_lock);
                m_activeRequests.remove(request->requestId());
                m_currentConcurrent = m_activeRequests.size();
                emit requestFinished(request->requestId());
                emit concurrentCountChanged(m_currentConcurrent);
                request->deleteLater();
                processNextRequest();
            });

            m_threadPool.start(request);
            emit requestStarted(request->requestId());
            emit concurrentCountChanged(m_currentConcurrent);

            if (m_currentConcurrent < m_threadPool.maxThreadCount()) {
                QMetaObject::invokeMethod(this, "processNextRequest", Qt::QueuedConnection);
            }

            return;
        }
    }
}

void EXImageRequestScheduler::adjustThreadPool()
{
    if (!m_config->adaptiveScaling()) return;

    static QElapsedTimer timer;
    if (!timer.isValid()) timer.start();

    if (timer.elapsed() < 30000) return;
    timer.restart();

    double load = calculateSystemLoad();
    int currentMax = m_threadPool.maxThreadCount();
    int newMax = currentMax;

    if (load < 0.3) {
        newMax = qMin(currentMax + 1, m_config->maxConcurrent());
    }
    else if (load > 0.7) {
        newMax = qMax(2, currentMax - 1);
    }

    if (newMax != currentMax) {
        qDebug() << "Adjusting thread pool size from" << currentMax << "to" << newMax
                 << "(System load:" << load * 100 << "%)";
        m_threadPool.setMaxThreadCount(newMax);

        if (m_currentConcurrent < newMax && m_totalQueued > 0) {
            QMetaObject::invokeMethod(this, "processNextRequest", Qt::QueuedConnection);
        }
    }
}

double EXImageRequestScheduler::calculateSystemLoad() const
{
    return 0.5;
}

void EXImageRequestScheduler::onConfigChanged()
{
    initializeThreadPool();
    if (m_currentConcurrent < m_threadPool.maxThreadCount() && m_totalQueued > 0) {
        QMetaObject::invokeMethod(this, "processNextRequest", Qt::QueuedConnection);
    }
}
