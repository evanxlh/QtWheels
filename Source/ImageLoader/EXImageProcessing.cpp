//
//  EXImageProcessing.cpp
//
//  Created by evanxlh on 2025/6/29.
//

#include "EXImageProcessing.h"
#include <algorithm>

EXImageProcessingChain::EXImageProcessingChain(const EXImageProcessingChain& other)
{
    for (const auto& step : other.m_steps) {
        m_steps.append(step->clone());
    }
}

EXImageProcessingChain& EXImageProcessingChain::operator=(const EXImageProcessingChain& other)
{
    if (this != &other) {
        m_steps.clear();
        for (const auto& step : other.m_steps) {
            m_steps.append(step->clone());
        }
    }
    return *this;
}

void EXImageProcessingChain::addStep(const QSharedPointer<EXImageProcessing>& step)
{
    m_steps.append(step);
}

void EXImageProcessingChain::insertStep(int index, const QSharedPointer<EXImageProcessing>& step)
{
    m_steps.insert(index, step);
}

void EXImageProcessingChain::removeStep(int index)
{
    if (index >= 0 && index < m_steps.size()) {
        m_steps.removeAt(index);
    }
}

void EXImageProcessingChain::clear()
{
    m_steps.clear();
}

QPixmap EXImageProcessingChain::apply(const QPixmap& input) const
{
    QPixmap result = input;
    for (const auto& step : m_steps) {
        result = step->process(result);
    }
    return result;
}

QString EXImageProcessingChain::chainIdentifier() const
{
    QStringList ids;
    for (const auto& step : m_steps) {
        ids.append(step->identifier());
    }
    return ids.join("|");
}

bool EXImageProcessingChain::isEmpty() const
{
    return m_steps.isEmpty();
}

int EXImageProcessingChain::stepCount() const
{
    return m_steps.size();
}

EXImageProcessingChain EXImageProcessingChain::merge(const EXImageProcessingChain& base,
                                                 const EXImageProcessingChain& overlay)
{
    EXImageProcessingChain merged = base;

    for (const auto& step : overlay.m_steps) {
        bool exists = false;
        for (const auto& existing : merged.m_steps) {
            if (existing->identifier() == step->identifier()) {
                exists = true;
                break;
            }
        }

        if (!exists) {
            merged.addStep(step->clone());
        }
    }

    merged.sortByProcessingOrder();
    return merged;
}

void EXImageProcessingChain::sortByProcessingOrder()
{
    std::sort(m_steps.begin(), m_steps.end(),
              [](const QSharedPointer<EXImageProcessing>& a,
                 const QSharedPointer<EXImageProcessing>& b) {
                  return a->processingOrder() < b->processingOrder();
              });
}

EXImageProcessingChain& EXImageProcessingChain::globalChain()
{
    static EXImageProcessingChain global;
    return global;
}
