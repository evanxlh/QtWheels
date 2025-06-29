//
//  EXImageProcessor.h
//
//  Created by evanxlh on 2025/6/29.
//

#pragma once

#include "EXImageProcessing.h"
#include <QPainter>
#include <QGraphicsBlurEffect>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QOpenGLContext>

class EX_IMAGE_LOADER_EXPORT EXScaleImageProcessor : public EXImageProcessing
{
public:
    explicit EXScaleImageProcessor(const QSize& size,
                             Qt::AspectRatioMode mode = Qt::KeepAspectRatio,
                             int order = 10);

    QPixmap process(const QPixmap& input) const override;
    QString identifier() const override;
    QSharedPointer<EXImageProcessing> clone() const override;
    int processingOrder() const override { return m_order; }

private:
    QSize m_size;
    Qt::AspectRatioMode m_mode;
    int m_order;
};

class EX_IMAGE_LOADER_EXPORT EXRotateImageProcessor : public EXImageProcessing
{
public:
    explicit EXRotateImageProcessor(qreal angle, int order = 20);
    QPixmap process(const QPixmap& input) const override;
    QString identifier() const override;
    QSharedPointer<EXImageProcessing> clone() const override;
    int processingOrder() const override { return m_order; }

private:
    qreal m_angle;
    int m_order;
};

class EX_IMAGE_LOADER_EXPORT EXRoundedCornerImageProcessor : public EXImageProcessing
{
public:
    explicit EXRoundedCornerImageProcessor(int radius, int order = 60);
    QPixmap process(const QPixmap& input) const override;
    QString identifier() const override;
    QSharedPointer<EXImageProcessing> clone() const override;
    int processingOrder() const override { return m_order; }

private:
    int m_radius;
    int m_order;
};

class EX_IMAGE_LOADER_EXPORT EXGrayscaleImageProcessor : public EXImageProcessing
{
public:
    explicit EXGrayscaleImageProcessor(int order = 30);
    int processingOrder() const override { return m_order; }
    QPixmap process(const QPixmap& input) const override;
    QString identifier() const override { return "Grayscale"; }
    QSharedPointer<EXImageProcessing> clone() const override;

private:
    int m_order;
};

class EX_IMAGE_LOADER_EXPORT EXBlurImageProcessor : public EXImageProcessing
{
public:
    explicit EXBlurImageProcessor(int radius = 5, int order = 40);
    int processingOrder() const override { return m_order; }
    QPixmap process(const QPixmap& input) const override;
    QString identifier() const override;
    QSharedPointer<EXImageProcessing> clone() const override;

private:
    int m_radius;
    int m_order;
};

class EX_IMAGE_LOADER_EXPORT EXSepiaImageProcessor : public EXImageProcessing
{
public:
    explicit EXSepiaImageProcessor(int order = 35);
    int processingOrder() const override { return m_order; }
    QPixmap process(const QPixmap& input) const override;
    QString identifier() const override { return "Sepia"; }
    QSharedPointer<EXImageProcessing> clone() const override;

private:
    int m_order;
};
