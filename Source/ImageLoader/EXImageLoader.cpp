//
//  EXImageLoader.cpp
//
//  Created by evanxlh on 2025/6/29.
//

#include "EXImageLoader.h"
#include "EXImageLoader.h"
#include "EXImageLoaderPrivate.h"
#include "EXImageRequestScheduler.h"
#include "EXImageProcessor.h"
#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QThread>
#include <QDebug>
#include <QtMinMax>

EXImageLoaderPrivate::EXImageLoaderPrivate(EXImageLoader* q)
    : q_ptr(q),
    downloader(nullptr),
    memoryCache(new EXMemoryCache<QString, QPixmap>({50 * 1024 * 1024})),
    diskCachePath(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/image_cache"),
    diskCacheMaxSize(200 * 1024 * 1024)
{
    QDir dir;
    dir.mkpath(diskCachePath);

    m_diskMonitorTimer = new QTimer(q);
    q->connect(m_diskMonitorTimer, &QTimer::timeout, [this]() {
        monitorDiskSpace();
    });
    m_diskMonitorTimer->start(5 * 60 * 1000);

    QMetaObject::invokeMethod(q, [this] {
            m_storageInfo = QStorageInfo(diskCachePath);
            checkDiskSpace();
        }, Qt::QueuedConnection);
}

EXImageLoaderPrivate::~EXImageLoaderPrivate()
{
    if (m_diskMonitorTimer) {
        m_diskMonitorTimer->stop();
        delete m_diskMonitorTimer;
    }
    delete memoryCache;
}

void EXImageLoaderPrivate::loadImage(const QUrl& url,
                                  const std::function<void (const QPixmap&)>& callback,
                                  ImageLoader::Priority priority,
                                  const QSize& thumbnailSize,
                                  const EXImageProcessingChain& processingChain)
{
    EXImageProcessingChain effectiveChain =
        EXImageProcessingChain::merge(EXImageProcessingChain::globalChain(), processingChain);

    if (!thumbnailSize.isEmpty()) {
        bool hasScaling = false;
        for (const auto& step : effectiveChain.m_steps) {
            if (dynamic_cast<EXScaleImageProcessor*>(step.data())) {
                hasScaling = true;
                break;
            }
        }

        if (!hasScaling) {
            effectiveChain.addStep(QSharedPointer<EXImageProcessing>(
                new EXScaleImageProcessor(thumbnailSize, Qt::KeepAspectRatio, 5)));
        }
    }

    const QString processingId = effectiveChain.chainIdentifier();
    const QString cacheKey = makeCacheKey(url, thumbnailSize, processingId);

    auto item = memoryCache->get(cacheKey);
    if (item.has_value()) {
        callback(item.value());
        return;
    }

    if (auto pixmap = loadFromDiskCache(cacheKey)) {
        memoryCache->put(cacheKey, *pixmap);
        callback(*pixmap);
        return;
    }

    auto request = new EXImageRequest(url, [=](const QPixmap& result, bool fromNetwork) {
            if (!result.isNull()) {
                memoryCache->put(cacheKey, result);
                if (fromNetwork) {
                    saveToDiskCache(cacheKey, result);
                }
            }
            callback(result);
        }, priority, thumbnailSize, effectiveChain);

    downloader->enqueueRequest(request);
}

std::optional<QPixmap> EXImageLoaderPrivate::loadFromDiskCache(const QString& key)
{
    QString filePath = diskCachePath + "/" + QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Md5).toHex();
    QImage image(filePath);
    auto pixmap = QPixmap::fromImage(image);
    return pixmap.isNull() ? std::nullopt : std::make_optional(pixmap);
}

void EXImageLoaderPrivate::saveToDiskCache(const QString& key, const QPixmap& pixmap)
{
    if (pixmap.isNull()) return;

    checkDiskSpace();

    QString filePath = diskCachePath + "/" + QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Md5).toHex();
    if (!pixmap.save(filePath))
    {
        qDebug() << "save image failed";
    }
}

void EXImageLoaderPrivate::checkDiskSpace()
{
    QDir dir(diskCachePath);
    quint64 totalSize = 0;
    const auto files = dir.entryInfoList(QDir::Files);
    for (const auto& info : files) {
        totalSize += info.size();
    }

    bool needCleanup = (totalSize > diskCacheMaxSize * 0.9);

    m_storageInfo.refresh();
    if (m_storageInfo.isValid()) {
        const qint64 freeSpace = m_storageInfo.bytesAvailable();
        const qint64 threshold = qMax(minFreeSpace, m_storageInfo.bytesTotal() / 10);
        needCleanup = needCleanup || (freeSpace < threshold);
    }

    if (needCleanup) {
        cleanDiskCache();
    }
}

void EXImageLoaderPrivate::cleanDiskCache()
{
    QDir dir(diskCachePath);
    auto files = dir.entryInfoList(QDir::Files, QDir::Time | QDir::Reversed);

    quint64 totalSize = 0;
    for (const auto& file : files) {
        totalSize += file.size();
    }

    m_storageInfo.refresh();
    qint64 freeSpace = m_storageInfo.bytesAvailable();
    const qint64 targetFreeSpace = qMax(minFreeSpace, m_storageInfo.bytesTotal() / 10);

    qDebug() << "Starting disk cache cleanup. Current:"
             << "Cache size:" << totalSize / (1024 * 1024) << "MB,"
             << "Free space:" << freeSpace / (1024 * 1024) << "MB,"
             << "Target free space:" << targetFreeSpace / (1024 * 1024) << "MB";

    int removedCount = 0;
    while (!files.isEmpty() && (freeSpace < targetFreeSpace || totalSize > diskCacheMaxSize)) {
        const QFileInfo fileInfo = files.takeFirst();
        const qint64 fileSize = fileInfo.size();

        if (QFile::remove(fileInfo.absoluteFilePath())) {
            totalSize -= fileSize;
            removedCount++;
            freeSpace += fileSize;
        }

        if (removedCount % 10 == 0) {
            m_storageInfo.refresh();
            freeSpace = m_storageInfo.bytesAvailable();
        }

        if (freeSpace >= targetFreeSpace && totalSize <= diskCacheMaxSize) {
            break;
        }
    }

    m_storageInfo.refresh();
    freeSpace = m_storageInfo.bytesAvailable();

    qDebug() << "Disk cache cleanup finished. Removed" << removedCount << "files."
             << "Current cache size:" << totalSize / (1024 * 1024) << "MB,"
             << "Free space:" << freeSpace / (1024 * 1024) << "MB";
}

void EXImageLoaderPrivate::monitorDiskSpace()
{
    m_storageInfo.refresh();

    if (!m_storageInfo.isValid()) {
        qWarning() << "Invalid storage info for path:" << diskCachePath;
        return;
    }

    const qint64 freeSpace = m_storageInfo.bytesAvailable();
    const qint64 threshold = qMax(minFreeSpace, m_storageInfo.bytesTotal() / 10);

    if (freeSpace < threshold) {
        qDebug() << "Low disk space detected:" << freeSpace / (1024 * 1024)
                 << "MB (threshold:" << threshold / (1024 * 1024) << "MB), cleaning cache...";
        cleanDiskCache();
    }
}

QString EXImageLoaderPrivate::makeCacheKey(const QUrl& url,
                                        const QSize& size,
                                        const QString& processingId) const
{
    return QString("%1_%2x%3_%4")
        .arg(url.toString())
        .arg(size.width())
        .arg(size.height())
        .arg(processingId);
}

EXImageLoader::EXImageLoader(QObject *parent)
    : QObject(parent),
    d_ptr(new EXImageLoaderPrivate(this)),
    m_config(new EXImageLoaderConfiguration(this))
{
    Q_D(EXImageLoader);
    d->downloader = new EXImageRequestScheduler(m_config.data(), this);

    connect(d->downloader, &EXImageRequestScheduler::concurrentCountChanged,
            this, &EXImageLoader::concurrentCountChanged);
    connect(d->downloader, &EXImageRequestScheduler::requestQueueOverflow,
            this, &EXImageLoader::requestQueueOverflow);
}

EXImageLoader::~EXImageLoader()
{
}

void EXImageLoader::loadImage(const QUrl& url,
                           const std::function<void (const QPixmap&)>& callback,
                           ImageLoader::Priority priority,
                           const QSize& thumbnailSize,
                           const EXImageProcessingChain& processingChain)
{
    Q_D(EXImageLoader);
    d->loadImage(url, callback, priority, thumbnailSize, processingChain);
}

void EXImageLoader::cancelLoad(const QUrl& url, const QString& processingId)
{
    Q_UNUSED(url)
    Q_UNUSED(processingId)
    qWarning() << "cancelLoad for specific url is not implemented, use cancelAll instead";
}

void EXImageLoader::cancelAll()
{
    Q_D(EXImageLoader);
    d->downloader->cancelAll();
}

void EXImageLoader::setMaxMemoryUsage(quint64 bytes)
{
    Q_D(EXImageLoader);
    // d->memoryCache->setCapacity(bytes);
}

void EXImageLoader::setDiskCachePath(const QString& path, quint64 maxSize)
{
    Q_D(EXImageLoader);
    d->diskCachePath = path;
    d->diskCacheMaxSize = maxSize;
    QDir().mkpath(path);
}

void EXImageLoader::setMinFreeSpace(quint64 bytes)
{
    Q_D(EXImageLoader);
    d->minFreeSpace = bytes;
}

EXImageLoader* EXImageLoader::globalInstance()
{
    static EXImageLoader instance;
    return &instance;
}

QPixmap EXImageLoader::cachedImage(const QUrl& url) const
{
    Q_D(const EXImageLoader);
    return d->memoryCache->get(d->makeCacheKey(url, QSize(), QString())).value();
}

void EXImageLoader::clearMemoryCache()
{
    Q_D(EXImageLoader);
    d->memoryCache->clear();
}

void EXImageLoader::clearDiskCache()
{
    Q_D(EXImageLoader);
    QDir dir(d->diskCachePath);
    dir.removeRecursively();
    dir.mkpath(d->diskCachePath);
}

void EXImageLoader::setMaxConcurrentDownloads(int maxConcurrent)
{
    m_config->setMaxConcurrent(maxConcurrent);
}

int EXImageLoader::maxConcurrentDownloads() const
{
    return m_config->maxConcurrent();
}

int EXImageLoader::activeDownloadCount() const
{
    Q_D(const EXImageLoader);
    return d->downloader->activeRequestCount();
}

int EXImageLoader::queuedDownloadCount() const
{
    Q_D(const EXImageLoader);
    return d->downloader->queuedRequestCount();
}

EXImageLoaderConfiguration* EXImageLoader::config() const
{
    return m_config.data();
}

void EXImageLoader::setGlobalProcessingChain(const EXImageProcessingChain& chain)
{
    EXImageProcessingChain::globalChain() = chain;
}

EXImageProcessingChain EXImageLoader::globalProcessingChain() const
{
    return EXImageProcessingChain::globalChain();
}
