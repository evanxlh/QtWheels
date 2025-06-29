//
//  EXImageLoaderPrivate.h
//
//  Created by evanxlh on 2025/6/29.
//

#pragma once

#include "EXImageLoader.h"
#include "EXImageRequestScheduler.h"
#include "../Cache/EXMemoryCache.h"
#include <QHash>
#include <QSet>
#include <optional>
#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDebug>
#include <QStorageInfo>
#include <QCryptographicHash>
#include <QFile>
#include <QDataStream>
#include <QTimer>

class EXImageLoaderPrivate
{
public:
    EXImageLoaderPrivate(EXImageLoader* q);
    ~EXImageLoaderPrivate();

    void loadImage(const QUrl& url,
                   const std::function<void(const QPixmap&)>& callback,
                   ImageLoader::Priority priority,
                   const QSize& thumbnailSize,
                   const EXImageProcessingChain& processingChain);

    std::optional<QPixmap> loadFromDiskCache(const QString& key);
    void saveToDiskCache(const QString& key, const QPixmap& pixmap);
    void checkDiskSpace();
    void monitorDiskSpace();
    void cleanDiskCache();

    QString makeCacheKey(const QUrl& url,
                         const QSize& size,
                         const QString& processingId) const;

    EXImageLoader* const q_ptr;
    EXImageRequestScheduler* downloader;
    EXMemoryCache<QString, QPixmap>* memoryCache;
    QString diskCachePath;
    qint64 diskCacheMaxSize;
    qint64 minFreeSpace = 100 * 1024 * 1024;
    QTimer* m_diskMonitorTimer = nullptr;
    QStorageInfo m_storageInfo;

    Q_DECLARE_PUBLIC(EXImageLoader)
};
