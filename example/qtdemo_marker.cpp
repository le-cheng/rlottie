/*
 * Qt版本的Lottie标记演示程序
 * 展示标记功能的使用
 */

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPainter>
#include <QWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <memory>
#include <iostream>
#include <functional>
#include <tuple>

#include "rlottie.h"

class QtMarkerWidget : public QWidget {
    Q_OBJECT

public:
    QtMarkerWidget(const std::string &filePath, const QString &title, 
                   const std::string &startMarker = "", const std::string &endMarker = "",
                   QWidget *parent = nullptr)
        : QWidget(parent), mTitle(title), mStartMarker(startMarker), mEndMarker(endMarker)
    {
        setFixedSize(400, 420);
        
        // 创建布局
        QVBoxLayout *layout = new QVBoxLayout(this);
        
        // 添加标题
        QLabel *titleLabel = new QLabel(title, this);
        titleLabel->setAlignment(Qt::AlignCenter);
        titleLabel->setStyleSheet("font-weight: bold; margin: 5px; background-color: #f0f0f0; padding: 5px;");
        layout->addWidget(titleLabel);
        
        // 创建动画显示区域
        mAnimWidget = new QWidget(this);
        mAnimWidget->setFixedSize(400, 400);
        layout->addWidget(mAnimWidget);

        // 加载动画
        mAnimation = rlottie::Animation::loadFromFile(filePath);
        if (mAnimation) {
            mAnimation->setRenderBackend(rlottie::RenderBackend::Qt);
            
            // 创建缓冲区
            mBuffer = std::make_unique<uint32_t[]>(400 * 400);
            mImage = QImage((uchar*)mBuffer.get(), 400, 400, 400 * 4, 
                           QImage::Format_ARGB32_Premultiplied);
            
            // 设置播放模式
            if (!mStartMarker.empty() && !mEndMarker.empty()) {
                // 标记到标记播放
                auto markers = mAnimation->markers();
                mFrameStart = 0;
                mFrameEnd = mAnimation->totalFrame() - 1;
                
                for (const auto& marker : markers) {
                    if (std::get<0>(marker) == mStartMarker) {
                        mFrameStart = std::get<1>(marker);
                    }
                    if (std::get<0>(marker) == mEndMarker) {
                        mFrameEnd = std::get<1>(marker);
                    }
                }
            } else if (!mStartMarker.empty()) {
                // 从标记开始播放
                auto markers = mAnimation->markers();
                mFrameStart = 0;
                mFrameEnd = mAnimation->totalFrame() - 1;
                
                for (const auto& marker : markers) {
                    if (std::get<0>(marker) == mStartMarker) {
                        mFrameStart = std::get<1>(marker);
                        break;
                    }
                }
            } else {
                // 完整播放
                mFrameStart = 0;
                mFrameEnd = mAnimation->totalFrame() - 1;
            }
            
            mCurrentFrame = mFrameStart;
            
            // 设置定时器
            mTimer = new QTimer(this);
            connect(mTimer, &QTimer::timeout, this, &QtMarkerWidget::renderFrame);
            mTimer->start(1000 / mAnimation->frameRate());
        }
    }

protected:
    void paintEvent(QPaintEvent *) override {
        if (!mAnimation) return;
        
        QPainter painter(this);
        
        // 绘制标题背景
        QRect titleRect(0, 0, width(), 20);
        painter.fillRect(titleRect, QColor(240, 240, 240));
        
        // 绘制动画
        QRect animRect(0, 20, 400, 400);
        painter.drawImage(animRect, mImage);
        
        // 绘制帧信息
        painter.setPen(Qt::black);
        painter.drawText(10, height() - 10, QString("Frame: %1/%2").arg(mCurrentFrame).arg(mFrameEnd));
    }

private slots:
    void renderFrame() {
        if (!mAnimation) return;

        // 渲染当前帧
        rlottie::Surface surface(mBuffer.get(), 400, 400, 400 * 4);
        mAnimation->renderSync(mCurrentFrame, surface);

        // 更新帧计数器
        mCurrentFrame++;
        if (mCurrentFrame > mFrameEnd) {
            mCurrentFrame = mFrameStart;
        }

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
    std::string mStartMarker;
    std::string mEndMarker;
    size_t mCurrentFrame = 0;
    size_t mFrameStart = 0;
    size_t mFrameEnd = 0;
};

class QtDemoMarker : public QMainWindow {
    Q_OBJECT

public:
    QtDemoMarker(QWidget *parent = nullptr) : QMainWindow(parent) {
        // 设置窗口
        setWindowTitle("rlottie Qt演示 - 标记功能");
        resize(1200, 500);

        // 创建中央窗口部件
        QWidget *centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);

        // 创建水平布局
        QHBoxLayout *hLayout = new QHBoxLayout(centralWidget);

        std::string filePath = std::string(DEMO_DIR) + "test.json";

        // Demo1: 完整播放
        auto demo1 = new QtMarkerWidget(filePath, "完整播放", "", "", this);
        hLayout->addWidget(demo1);

        // Demo2: 从标记开始播放 (由于test.json没有标记，这里使用空标记)
        auto demo2 = new QtMarkerWidget(filePath, "测试播放", "", "", this);
        hLayout->addWidget(demo2);

        // Demo3: 标记到标记播放 (由于test.json没有标记，这里使用空标记)
        auto demo3 = new QtMarkerWidget(filePath, "循环播放", "", "", this);
        hLayout->addWidget(demo3);
    }
};

int main(int argc, char *argv[])
{
    // 配置渲染后端
    rlottie::configureRenderBackend(rlottie::RenderBackend::Qt);

    QApplication app(argc, argv);

    QtDemoMarker demo;
    demo.show();

    return app.exec();
}

#include "qtdemo_marker.moc" 