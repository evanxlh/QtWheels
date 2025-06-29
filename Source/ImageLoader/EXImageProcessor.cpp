//
//  EXImageProcessor.cpp
//
//  Created by evanxlh on 2025/6/29.
//

#include "EXImageProcessor.h"
#include <QPainter>
#include <QImage>
#include <QOpenGLContext>
#include <QtMath>

EXScaleImageProcessor::EXScaleImageProcessor(const QSize& size, Qt::AspectRatioMode mode, int order)
    : m_size(size), m_mode(mode), m_order(order) {}

QPixmap EXScaleImageProcessor::process(const QPixmap& input) const
{
    return input.scaled(m_size, m_mode, Qt::SmoothTransformation);
}

QString EXScaleImageProcessor::identifier() const
{
    return QString("Scale_%1x%2_%3")
        .arg(m_size.width())
        .arg(m_size.height())
        .arg(static_cast<int>(m_mode));
}

QSharedPointer<EXImageProcessing> EXScaleImageProcessor::clone() const
{
    return QSharedPointer<EXImageProcessing>(new EXScaleImageProcessor(m_size, m_mode, m_order));
}

EXRotateImageProcessor::EXRotateImageProcessor(qreal angle, int order)
    : m_angle(angle), m_order(order) {}

QPixmap EXRotateImageProcessor::process(const QPixmap& input) const
{
    QTransform transform;
    transform.rotate(m_angle);
    return input.transformed(transform, Qt::SmoothTransformation);
}

QString EXRotateImageProcessor::identifier() const
{
    return QString("Rotate_%1").arg(m_angle);
}

QSharedPointer<EXImageProcessing> EXRotateImageProcessor::clone() const
{
    return QSharedPointer<EXImageProcessing>(new EXRotateImageProcessor(m_angle, m_order));
}

EXRoundedCornerImageProcessor::EXRoundedCornerImageProcessor(int radius, int order)
    : m_radius(radius), m_order(order) {}

QPixmap EXRoundedCornerImageProcessor::process(const QPixmap& input) const
{
    if (input.isNull()) return input;

    QPixmap result(input.size());
    result.fill(Qt::transparent);

    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    QPainterPath path;
    path.addRoundedRect(0, 0, input.width(), input.height(), m_radius, m_radius);
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, input);

    return result;
}

QString EXRoundedCornerImageProcessor::identifier() const
{
    return QString("Rounded_%1").arg(m_radius);
}

QSharedPointer<EXImageProcessing> EXRoundedCornerImageProcessor::clone() const
{
    return QSharedPointer<EXImageProcessing>(new EXRoundedCornerImageProcessor(m_radius, m_order));
}

EXGrayscaleImageProcessor::EXGrayscaleImageProcessor(int order) : m_order(order) {}

QPixmap EXGrayscaleImageProcessor::process(const QPixmap& input) const
{
    if (input.isNull()) return input;

    QImage image = input.toImage();
    for (int y = 0; y < image.height(); ++y) {
        QRgb* line = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            int gray = qGray(line[x]);
            line[x] = qRgba(gray, gray, gray, qAlpha(line[x]));
        }
    }
    return QPixmap::fromImage(image);
}

QSharedPointer<EXImageProcessing> EXGrayscaleImageProcessor::clone() const
{
    return QSharedPointer<EXImageProcessing>(new EXGrayscaleImageProcessor(m_order));
}

EXBlurImageProcessor::EXBlurImageProcessor(int radius, int order)
    : m_radius(radius), m_order(order) {}

QPixmap EXBlurImageProcessor::process(const QPixmap& input) const
{
    if (input.isNull() || m_radius <= 0) return input;

    QGraphicsScene scene;
    QGraphicsPixmapItem item;
    item.setPixmap(input);

    QGraphicsBlurEffect blur;
    blur.setBlurRadius(m_radius);
    item.setGraphicsEffect(&blur);

    scene.addItem(&item);

    QImage result(input.size(), QImage::Format_ARGB32);
    result.fill(Qt::transparent);

    QPainter painter(&result);
    scene.render(&painter, QRectF(), QRectF(0, 0, input.width(), input.height()));

    return QPixmap::fromImage(result);
}

QString EXBlurImageProcessor::identifier() const
{
    return QString("Blur_%1").arg(m_radius);
}

QSharedPointer<EXImageProcessing> EXBlurImageProcessor::clone() const
{
    return QSharedPointer<EXImageProcessing>(new EXBlurImageProcessor(m_radius, m_order));
}

EXSepiaImageProcessor::EXSepiaImageProcessor(int order) : m_order(order) {}

QPixmap EXSepiaImageProcessor::process(const QPixmap& input) const
{
    if (input.isNull()) return input;

    QImage image = input.toImage();
    for (int y = 0; y < image.height(); ++y) {
        QRgb* line = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            QRgb pixel = line[x];
            int r = qRed(pixel);
            int g = qGreen(pixel);
            int b = qBlue(pixel);

            int newR = qMin(255, static_cast<int>(r * 0.393 + g * 0.769 + b * 0.189));
            int newG = qMin(255, static_cast<int>(r * 0.349 + g * 0.686 + b * 0.168));
            int newB = qMin(255, static_cast<int>(r * 0.272 + g * 0.534 + b * 0.131));

            line[x] = qRgba(newR, newG, newB, qAlpha(pixel));
        }
    }
    return QPixmap::fromImage(image);
}

QSharedPointer<EXImageProcessing> EXSepiaImageProcessor::clone() const
{
    return QSharedPointer<EXImageProcessing>(new EXSepiaImageProcessor(m_order));
}
