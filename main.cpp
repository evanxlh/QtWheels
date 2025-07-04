//
//  hello.cpp
//
//  Created by evanxlh on 2025-06-15.
//

#include "MainWindow.h"
#include "Source/Cache/EXMemoryCache.h"
#include "Source/ImageLoader/EXImageLoader.h"
#include "Source/ImageLoader/EXImageProcessor.h"

#include <QApplication>
#include <QLabel>
#include <iostream>
#include <memory>
#include <chrono>

// 测试对象类
class TestObject {
public:
    TestObject(int id, const std::string& name)
        : id(id), name(name) {}

    void print() const {
        std::cout << "Object #" << id << ": " << name << "\n";
    }

public:
    int id;
    std::string name;
};

// 值类型缓存测试
void testValueCache()
{
    std::cout << "\n=== 测试值类型缓存 (int -> std::string) ===\n";

    srand(static_cast<unsigned>(time(nullptr)));

    EXMemoryCache<int, std::string>::Config config;
    config.costLimit = 500 * 1024 * 1024; // 100MB
    config.countLimit = 1000000;
    config.enablesThreadSafe = false;
    config.enablesTTL = false;
    config.defaultTTL = 60; // 默认60秒过期

    EXMemoryCache<int, std::string> cache(config);

    const int numItems = 1000000;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numItems; ++i) {
        cache.put(i, "Value for key: " + std::to_string(i), sizeof(std::string));
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "插入 " << numItems << " 条数据耗时: "
              << duration.count() << " ms\n";
    std::cout << "当前缓存数量: " << cache.count() << "\n";
    std::cout << "当前缓存成本: " << cache.totalCost() << " bytes\n";

    // 随机访问测试
    int hits = 0;
    start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100000; ++i) {
        int key = rand() % numItems;
        if (auto val = cache.get(key)) {
            hits++;
        }
    }

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "随机访问 100,000 次, 命中: " << hits
              << ", 耗时: " << duration.count() << " ms\n";
}

// 智能指针缓存测试
void testSharedPtrCache() {
    std::cout << "\n=== 测试智能指针缓存 (int -> shared_ptr<TestObject>) ===\n";

    srand(static_cast<unsigned>(time(nullptr)));

    EXMemoryCache<int, std::shared_ptr<TestObject>>::Config config;
    config.costLimit = 500 * 1024 * 1024; // 200MB
    config.countLimit = 1000000;
    config.enablesThreadSafe = true;
    config.enablesTTL = true;
    config.defaultTTL = 60; // 默认60秒过期

    EXMemoryCache<int, std::shared_ptr<TestObject>> cache(config);

    const int numItems = 1000000;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numItems; ++i) {
        auto obj = std::make_shared<TestObject>(i, "Object " + std::to_string(i));
        // 计算成本（对象大小 + 字符串大小）
        size_t cost = sizeof(TestObject) + obj->name.capacity();
        cache.put(i, std::move(obj), cost, 30); // TTL 30秒
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "插入 " << numItems << " 个对象耗时: "
              << duration.count() << " ms\n";
    std::cout << "当前缓存数量: " << cache.count() << "\n";
    std::cout << "当前缓存成本: " << cache.totalCost() << " bytes\n";

    // 随机访问测试
    int hits = 0;
    start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 10000; ++i) {
        int key = rand() % numItems;
        if (auto val = cache.get(key)) {
            hits++;
        }
    }

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "随机访问 10,000 次, 命中: " << hits
              << ", 耗时: " << duration.count() << " ms\n";

    // 测试生命周期管理
    std::cout << "\n测试生命周期管理...\n";
    {
        auto externalRef = cache.get(500); // 获取一个外部引用

        // 移除所有缓存项
        cache.clear();
        std::cout << "清空缓存后，缓存数量: " << cache.count() << "\n";

        // 外部引用仍然有效
        if (externalRef && *externalRef) {
            std::cout << "外部引用仍然有效: ";
            (*externalRef)->print();
        }
    }
    std::cout << "外部引用销毁后，对象自动释放\n";
}

void testImageLoader()
{
    QLabel* label = new QLabel();
    label->setFixedWidth(300);
    label->setFixedHeight(200);
    label->show();

    // 设置全局处理链
    EXImageProcessingChain globalChain;
    globalChain.addStep(QSharedPointer<EXImageProcessing>(new EXRoundedCornerImageProcessor(10)));
    globalChain.addStep(QSharedPointer<EXImageProcessing>(new EXSepiaImageProcessor()));
    EXImageLoader::globalInstance()->setGlobalProcessingChain(globalChain);

    // 设置并发配置
    EXImageLoader::globalInstance()->config()->setMaxConcurrent(12);
    EXImageLoader::globalInstance()->config()->setQueueCapacity(200);

    // 加载图片
    EXImageLoader::globalInstance()->loadImage(
        QUrl("https://pic.rmb.bdstatic.com/bjh/other/148cbc3884a23b4c72b96194ba9066ee.png?for=bg"),
        [label](const QPixmap& pixmap) {
            label->setPixmap(pixmap);
        },
        ImageLoader::Priority::High,
        QSize(200, 200) // 缩略图尺寸
        );

    // 带单次处理链的加载
    EXImageProcessingChain requestChain;
    requestChain.addStep(QSharedPointer<EXImageProcessing>(new EXGrayscaleImageProcessor()));

    // EXImageLoader::globalInstance()->loadImage(
    //     QUrl("https://example.com/another.jpg"),
    //     [label](const QPixmap& pixmap) {
    //         label->setPixmap(pixmap);
    //     },
    //     ImageLoader::Priority::Medium,
    //     QSize(),
    //     requestChain
    //     );

    // 监控并发状态
    label->connect(EXImageLoader::globalInstance(), &EXImageLoader::concurrentCountChanged,
            [](int count) {
                qDebug() << "当前并发任务数:" << count;
            });
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    testImageLoader();
    // testValueCache();
    // testSharedPtrCache();

    return a.exec();
}
