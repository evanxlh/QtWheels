//
//  EXImageRequest.h
//
//  Created by evanxlh on 2025/6/29.
//

#pragma once

#include "EXImageProcessing.h"
#include <QRunnable>
#include <QObject>
#include <QUrl>
#include <QSize>
#include <QPixmap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QBuffer>
#include <QCryptographicHash>
#include <atomic>

class EXImageRequest : public QObject, public QRunnable
{
    Q_OBJECT
public:
    EXImageRequest(const QUrl& url,
                  std::function<void(const QPixmap&, bool)> callback,
                  ImageLoader::Priority priority,
                  const QSize& thumbnailSize,
                  const EXImageProcessingChain& processingChain);

    ~EXImageRequest();

    void run() override;
    void cancel();

    ImageLoader::Priority priority() const { return m_priority; }
    QString requestId() const { return m_requestId; }

    bool isSameRequest(const EXImageRequest* other) const;

    void reportProgress(int percent);

signals:
    void finished();
    void progress(int percent);

private:
    QPixmap downloadImage();
    QPixmap processImage(QPixmap pixmap) const;
    QString generateRequestId() const;

    QUrl m_url;
    std::function<void(const QPixmap&, bool)> m_callback;
    ImageLoader::Priority m_priority;
    QSize m_thumbnailSize;
    EXImageProcessingChain m_processingChain;
    QString m_requestId;
    std::atomic<bool> m_cancelled;
};
