/*
 * Qt版本的Lottie演示程序
 * 展示各种动态属性修改功能
 */

#include <QApplication>
#include <QGridLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPainter>
#include <QWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <memory>
#include <iostream>
#include <functional>

#include "rlottie.h"

class QtLottieWidget : public QWidget {
    Q_OBJECT

public:
    QtLottieWidget(const std::string &filePath, const QString &title, QWidget *parent = nullptr)
        : QWidget(parent), mTitle(title)
    {
        setFixedSize(300, 320);
        
        // 创建布局
        QVBoxLayout *layout = new QVBoxLayout(this);
        
        // 添加标题
        QLabel *titleLabel = new QLabel(title, this);
        titleLabel->setAlignment(Qt::AlignCenter);
        titleLabel->setStyleSheet("font-weight: bold; margin: 5px;");
        layout->addWidget(titleLabel);
        
        // 创建动画显示区域
        mAnimWidget = new QWidget(this);
        mAnimWidget->setFixedSize(300, 300);
        layout->addWidget(mAnimWidget);

        // 加载动画
        mAnimation = rlottie::Animation::loadFromFile(filePath);
        if (mAnimation) {
            mAnimation->setRenderBackend(rlottie::RenderBackend::Qt);
            
            size_t width = 0, height = 0;
            mAnimation->size(width, height);
            
            // 创建缓冲区
            mBuffer = std::make_unique<uint32_t[]>(300 * 300);
            mImage = QImage((uchar*)mBuffer.get(), 300, 300, 300 * 4, 
                           QImage::Format_ARGB32_Premultiplied);
            
            // 设置定时器
            mTimer = new QTimer(this);
            connect(mTimer, &QTimer::timeout, this, &QtLottieWidget::renderFrame);
            mTimer->start(1000 / mAnimation->frameRate());
        }
    }

    void setPropertyCallback(std::function<void()> callback) {
        mPropertyCallback = callback;
    }

    std::unique_ptr<rlottie::Animation>& animation() { return mAnimation; }

protected:
    void paintEvent(QPaintEvent *) override {
        if (!mAnimation) return;
        
        QPainter painter(this);
        
        // 绘制标题背景
        QRect titleRect(0, 0, width(), 20);
        painter.fillRect(titleRect, QColor(240, 240, 240));
        
        // 绘制动画
        QRect animRect(0, 20, 300, 300);
        painter.drawImage(animRect, mImage);
    }

private slots:
    void renderFrame() {
        if (!mAnimation) return;

        // 应用属性回调
        if (mPropertyCallback) {
            mPropertyCallback();
        }

        // 渲染当前帧
        rlottie::Surface surface(mBuffer.get(), 300, 300, 300 * 4);
        mAnimation->renderSync(mCurrentFrame, surface);

        // 更新帧计数器
        mCurrentFrame = (mCurrentFrame + 1) % mAnimation->totalFrame();

        // 重绘
        update();
    }

private:
    std::unique_ptr<rlottie::Animation> mAnimation;
    std::unique_ptr<uint32_t[]> mBuffer;
    QWidget *mAnimWidget;
    QTimer *mTimer;
    QImage mImage;
    QString mTitle;
    size_t mCurrentFrame = 0;
    std::function<void()> mPropertyCallback;
};

class QtDemo : public QMainWindow {
    Q_OBJECT

public:
    QtDemo(QWidget *parent = nullptr) : QMainWindow(parent) {
        // 设置窗口
        setWindowTitle("rlottie Qt演示 - 动态属性");
        resize(1200, 700);

        // 创建中央窗口部件
        QWidget *centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);

        // 创建网格布局
        QGridLayout *gridLayout = new QGridLayout(centralWidget);

        std::string filePath = std::string(DEMO_DIR) + "test.json";

        // Demo1: 填充颜色
        auto demo1 = new QtLottieWidget(filePath, "填充颜色变化", this);
        demo1->setPropertyCallback([=]() {
            if (demo1->animation()) {
                demo1->animation()->setValue<rlottie::Property::FillColor>(
                    "Shape Layer 1.Ellipse 1.Fill 1",
                    [](const rlottie::FrameInfo& info) {
                        if (info.curFrame() < 60)
                            return rlottie::Color(0, 0, 1);
                        else
                            return rlottie::Color(1, 0, 0);
                    });
            }
        });
        gridLayout->addWidget(demo1, 0, 0);

        // Demo2: 描边透明度
        auto demo2 = new QtLottieWidget(filePath, "描边透明度", this);
        demo2->setPropertyCallback([=]() {
            if (demo2->animation()) {
                demo2->animation()->setValue<rlottie::Property::StrokeOpacity>(
                    "Shape Layer 2.Shape 1.Stroke 1",
                    [](const rlottie::FrameInfo& info) {
                        if (info.curFrame() < 60)
                            return 20;
                        else
                            return 100;
                    });
            }
        });
        gridLayout->addWidget(demo2, 0, 1);

        // Demo3: 描边宽度
        auto demo3 = new QtLottieWidget(filePath, "描边宽度", this);
        demo3->setPropertyCallback([=]() {
            if (demo3->animation()) {
                demo3->animation()->setValue<rlottie::Property::StrokeWidth>(
                    "**",
                    [](const rlottie::FrameInfo& info) {
                        if (info.curFrame() < 60)
                            return 1.0;
                        else
                            return 5.0;
                    });
            }
        });
        gridLayout->addWidget(demo3, 0, 2);

        // Demo4: 变换位置
        auto demo4 = new QtLottieWidget(filePath, "变换位置", this);
        demo4->setPropertyCallback([=]() {
            if (demo4->animation()) {
                demo4->animation()->setValue<rlottie::Property::TrPosition>(
                    "Shape Layer 1.Ellipse 1",
                    [](const rlottie::FrameInfo& info) {
                        return rlottie::Point(-20 + (double)info.curFrame()/2.0,
                                             -20 + (double)info.curFrame()/2.0);
                    });
            }
        });
        gridLayout->addWidget(demo4, 0, 3);

        // Demo5: 缩放变换
        auto demo5 = new QtLottieWidget(filePath, "缩放变换", this);
        demo5->setPropertyCallback([=]() {
            if (demo5->animation()) {
                demo5->animation()->setValue<rlottie::Property::TrScale>(
                    "Shape Layer 1.Ellipse 1",
                    [](const rlottie::FrameInfo& info) {
                        return rlottie::Size(100 - info.curFrame(), 50);
                    });
            }
        });
        gridLayout->addWidget(demo5, 1, 0);

        // Demo6: 旋转变换
        auto demo6 = new QtLottieWidget(filePath, "旋转变换", this);
        demo6->setPropertyCallback([=]() {
            if (demo6->animation()) {
                demo6->animation()->setValue<rlottie::Property::TrRotation>(
                    "Shape Layer 2.Shape 1",
                    [](const rlottie::FrameInfo& info) {
                        return info.curFrame() * 20;
                    });
            }
        });
        gridLayout->addWidget(demo6, 1, 1);

        // Demo7: 综合变换
        auto demo7 = new QtLottieWidget(filePath, "综合变换", this);
        demo7->setPropertyCallback([=]() {
            if (demo7->animation()) {
                auto& anim = demo7->animation();
                anim->setValue<rlottie::Property::TrRotation>(
                    "Shape Layer 1.Ellipse 1",
                    [](const rlottie::FrameInfo& info) {
                        return info.curFrame() * 20;
                    });
                anim->setValue<rlottie::Property::TrScale>(
                    "Shape Layer 1.Ellipse 1",
                    [](const rlottie::FrameInfo& info) {
                        return rlottie::Size(50, 100 - info.curFrame());
                    });
                anim->setValue<rlottie::Property::FillColor>(
                    "Shape Layer 1.Ellipse 1.Fill 1",
                    [](const rlottie::FrameInfo& info) {
                        if (info.curFrame() < 60)
                            return rlottie::Color(0, 0, 1);
                        else
                            return rlottie::Color(1, 0, 0);
                    });
            }
        });
        gridLayout->addWidget(demo7, 1, 2);
    }
};

int main(int argc, char *argv[])
{
    // 配置渲染后端
    rlottie::configureRenderBackend(rlottie::RenderBackend::Qt);

    QApplication app(argc, argv);

    QtDemo demo;
    demo.show();

    return app.exec();
}

#include "qtdemo.moc" 