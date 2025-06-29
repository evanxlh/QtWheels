//
//  EXImageLoaderGlobal.h
//
//  Created by evanxlh on 2025/6/29.
//

#pragma once

#include <QtGlobal>

#if defined(EX_IMAGE_LOADER_LIBRARY)
#  define EX_IMAGE_LOADER_EXPORT Q_DECL_EXPORT
#else
#  define EX_IMAGE_LOADER_EXPORT Q_DECL_IMPORT
#endif

namespace ImageLoader
{
enum class Priority
{
    VeryLow,
    Low,
    Medium,
    High,
    VeryHigh
};
}
