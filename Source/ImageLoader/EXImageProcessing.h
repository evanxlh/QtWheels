//
//  EXImageProcessing.h
//
//  Created by evanxlh on 2025/6/29.
//

#pragma once

#include "EXImageLoaderGlobal.h"
#include <QPixmap>
#include <QString>
#include <QSharedPointer>
#include <QList>
#include <QMetaType>

class EXImageProcessing;

class EX_IMAGE_LOADER_EXPORT EXImageProcessing
{
public:
    virtual ~EXImageProcessing() {}
    virtual QPixmap process(const QPixmap& input) const = 0;
    virtual QString identifier() const = 0;
    virtual QSharedPointer<EXImageProcessing> clone() const = 0;
    virtual int processingOrder() const { return 50; }
};

class EX_IMAGE_LOADER_EXPORT EXImageProcessingChain
{
public:
    EXImageProcessingChain() = default;
    EXImageProcessingChain(const EXImageProcessingChain& other);
    EXImageProcessingChain& operator=(const EXImageProcessingChain& other);

    void addStep(const QSharedPointer<EXImageProcessing>& step);
    void insertStep(int index, const QSharedPointer<EXImageProcessing>& step);
    void removeStep(int index);
    void clear();

    QPixmap apply(const QPixmap& input) const;
    QString chainIdentifier() const;
    bool isEmpty() const;
    int stepCount() const;

    static EXImageProcessingChain merge(const EXImageProcessingChain& base,
                                      const EXImageProcessingChain& overlay);

    void sortByProcessingOrder();

    static EXImageProcessingChain& globalChain();

public:
    QList<QSharedPointer<EXImageProcessing>> m_steps;
};

Q_DECLARE_METATYPE(EXImageProcessingChain)

