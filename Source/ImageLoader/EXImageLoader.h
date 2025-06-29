//
//  EXImageLoader.h
//
//  Created by evanxlh on 2025/6/29.
//

#pragma once

#include "EXImageLoaderConfiguration.h"
#include "EXImageProcessing.h"
#include <QObject>
#include <QUrl>
#include <QPixmap>
#include <QSize>
#include <functional>

class EXImageLoaderPrivate;

class EX_IMAGE_LOADER_EXPORT EXImageLoader : public QObject
{
    Q_OBJECT
public:
    explicit EXImageLoader(QObject *parent = nullptr);
    ~EXImageLoader();

    void loadImage(const QUrl& url,
                   const std::function<void(const QPixmap&)>& callback,
                   ImageLoader::Priority priority = ImageLoader::Priority::Medium,
                   const QSize& thumbnailSize = QSize(),
                   const EXImageProcessingChain& processingChain = EXImageProcessingChain());

    void cancelLoad(const QUrl& url, const QString& processingId = QString());
    void cancelAll();

    void setMaxMemoryUsage(quint64 bytes);
    void setDiskCachePath(const QString& path, quint64 maxSize = 0);
    void setMinFreeSpace(quint64 bytes);

    static EXImageLoader* globalInstance();

    QPixmap cachedImage(const QUrl& url) const;
    void clearMemoryCache();
    void clearDiskCache();

    void setMaxConcurrentDownloads(int maxConcurrent);
    int maxConcurrentDownloads() const;

    int activeDownloadCount() const;
    int queuedDownloadCount() const;

    EXImageLoaderConfiguration* config() const;

    void setGlobalProcessingChain(const EXImageProcessingChain& chain);
    EXImageProcessingChain globalProcessingChain() const;

signals:
    void concurrentCountChanged(int count);
    void requestQueueOverflow();

private:
    Q_DECLARE_PRIVATE(EXImageLoader)
    QScopedPointer<EXImageLoaderPrivate> d_ptr;
    QScopedPointer<EXImageLoaderConfiguration> m_config;
};
