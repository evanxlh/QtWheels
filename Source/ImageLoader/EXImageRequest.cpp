//
//  EXImageRequest.cpp
//
//  Created by evanxlh on 2025/6/29.
//

#include "EXImageRequest.h"

EXImageRequest::EXImageRequest(const QUrl& url,
                             std::function<void (const QPixmap&, bool)> callback,
                             ImageLoader::Priority priority,
                             const QSize& thumbnailSize,
                             const EXImageProcessingChain& processingChain)
    : m_url(url),
    m_callback(callback),
    m_priority(priority),
    m_thumbnailSize(thumbnailSize),
    m_processingChain(processingChain),
    m_cancelled(false)
{
    m_requestId = generateRequestId();
}

EXImageRequest::~EXImageRequest()
{
    cancel();
}

void EXImageRequest::run()
{
    if (m_cancelled) {
        emit finished();
        return;
    }

    QPixmap result;
    bool fromNetwork = false;

    if (m_url.isLocalFile()) {
        result = QPixmap(m_url.toLocalFile());
    } else {
        result = downloadImage();
        fromNetwork = !result.isNull();
    }

    if (m_cancelled) {
        emit finished();
        return;
    }

    if (!result.isNull()) {
        if (!m_thumbnailSize.isEmpty() || !m_processingChain.isEmpty()) {
            result = processImage(result);
        }

        if (!m_cancelled) {
            m_callback(result, fromNetwork);
        }
    }

    emit finished();
}

void EXImageRequest::cancel()
{
    m_cancelled = true;
}

void EXImageRequest::reportProgress(int percent)
{
    if (!m_cancelled) {
        emit progress(percent);
    }
}

QPixmap EXImageRequest::downloadImage()
{
    QNetworkAccessManager manager;
    QEventLoop loop;
    QNetworkReply *reply = manager.get(QNetworkRequest(m_url));

    connect(reply, &QNetworkReply::downloadProgress,
            [this](qint64 received, qint64 total) {
                if (total > 0) {
                    int percent = static_cast<int>(received * 100 / total);
                    reportProgress(percent);
                }
            });

    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (m_cancelled || reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        return QPixmap();
    }

    QByteArray data = reply->readAll();
    QImage image;
    image.loadFromData(data);
    QPixmap pixmap = QPixmap::fromImage(image);

    reply->deleteLater();
    return pixmap;
}

QPixmap EXImageRequest::processImage(QPixmap pixmap) const
{
    if (pixmap.isNull()) return pixmap;
    return m_processingChain.apply(pixmap);
}

QString EXImageRequest::generateRequestId() const
{
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(m_url.toString().toUtf8());
    hash.addData(m_thumbnailSize.isEmpty() ? "" :
                     QByteArray::number(m_thumbnailSize.width()) +
                         QByteArray::number(m_thumbnailSize.height()));
    hash.addData(m_processingChain.chainIdentifier().toUtf8());

    return hash.result().toHex();
}

bool EXImageRequest::isSameRequest(const EXImageRequest* other) const
{
    return m_url == other->m_url &&
           m_thumbnailSize == other->m_thumbnailSize &&
           m_processingChain.chainIdentifier() == other->m_processingChain.chainIdentifier();
}
