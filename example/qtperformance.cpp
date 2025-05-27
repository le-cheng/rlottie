/*
 * Qt版本的Lottie性能测试程序
 * 可以同时显示多个动画进行性能测试
 */

#include <QApplication>
#include <QGridLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPainter>
#include <QPushButton>
#include <QWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QProgressBar>
#include <memory>
#include <iostream>
#include <vector>
#include <chrono>
#include <functional>
#include <cmath>

#include "rlottie.h"

class QtPerformanceWidget : public QWidget {
    Q_OBJECT

public:
    QtPerformanceWidget(const std::string &filePath, int index, QWidget *parent = nullptr)
        : QWidget(parent), mIndex(index)
    {
        setFixedSize(150, 150);
        
        // 加载动画
        mAnimation = rlottie::Animation::loadFromFile(filePath);
        if (mAnimation) {
            mAnimation->setRenderBackend(rlottie::RenderBackend::Qt);
            
            // 创建缓冲区
            mBuffer = std::make_unique<uint32_t[]>(150 * 150);
            mImage = QImage((uchar*)mBuffer.get(), 150, 150, 150 * 4, 
                           QImage::Format_ARGB32_Premultiplied);
            
            // 设置定时器
            mTimer = new QTimer(this);
            connect(mTimer, &QTimer::timeout, this, &QtPerformanceWidget::renderFrame);
        }
    }

    void startAnimation() {
        if (mAnimation && mTimer) {
            mStartTime = std::chrono::high_resolution_clock::now();
            mFrameCount = 0;
            mTimer->start(1000 / mAnimation->frameRate());
        }
    }

    void stopAnimation() {
        if (mTimer) {
            mTimer->stop();
        }
    }

    double getFPS() const {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double>(now - mStartTime).count();
        return duration > 0 ? mFrameCount / duration : 0;
    }

    size_t getFrameCount() const { return mFrameCount; }

protected:
    void paintEvent(QPaintEvent *) override {
        if (!mAnimation) return;
        
        QPainter painter(this);
        painter.drawImage(rect(), mImage);
        
        // 绘制索引
        painter.setPen(Qt::white);
        painter.drawText(5, 15, QString::number(mIndex));
    }

private slots:
    void renderFrame() {
        if (!mAnimation) return;

        // 渲染当前帧
        rlottie::Surface surface(mBuffer.get(), 150, 150, 150 * 4);
        mAnimation->renderSync(mCurrentFrame, surface);

        // 更新帧计数器
        mCurrentFrame = (mCurrentFrame + 1) % mAnimation->totalFrame();
        mFrameCount++;

        // 重绘
        update();
    }

private:
    std::unique_ptr<rlottie::Animation> mAnimation;
    std::unique_ptr<uint32_t[]> mBuffer;
    QTimer *mTimer;
    QImage mImage;
    int mIndex;
    size_t mCurrentFrame = 0;
    size_t mFrameCount = 0;
    std::chrono::high_resolution_clock::time_point mStartTime;
};

class QtPerformanceTest : public QMainWindow {
    Q_OBJECT

public:
    QtPerformanceTest(QWidget *parent = nullptr) : QMainWindow(parent) {
        // 设置窗口
        setWindowTitle("rlottie Qt性能测试");
        resize(800, 600);

        // 创建中央窗口部件
        QWidget *centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);

        // 创建主布局
        QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

        // 创建控制面板
        createControlPanel(mainLayout);

        // 创建动画显示区域
        mAnimationArea = new QWidget(this);
        mainLayout->addWidget(mAnimationArea);

        // 创建状态栏
        createStatusPanel(mainLayout);
        
        // 设置更新定时器
        mUpdateTimer = new QTimer(this);
        connect(mUpdateTimer, &QTimer::timeout, this, &QtPerformanceTest::updateStats);
        mUpdateTimer->start(1000); // 每秒更新一次统计信息
    }

private:
    void createControlPanel(QVBoxLayout *mainLayout) {
        QHBoxLayout *controlLayout = new QHBoxLayout();

        // 动画数量选择
        QLabel *countLabel = new QLabel("动画数量:", this);
        mCountSpinBox = new QSpinBox(this);
        mCountSpinBox->setRange(1, 100);
        mCountSpinBox->setValue(25);

        // 开始按钮
        mStartButton = new QPushButton("开始测试", this);
        connect(mStartButton, &QPushButton::clicked, this, &QtPerformanceTest::startTest);

        // 停止按钮
        mStopButton = new QPushButton("停止测试", this);
        mStopButton->setEnabled(false);
        connect(mStopButton, &QPushButton::clicked, this, &QtPerformanceTest::stopTest);

        controlLayout->addWidget(countLabel);
        controlLayout->addWidget(mCountSpinBox);
        controlLayout->addWidget(mStartButton);
        controlLayout->addWidget(mStopButton);
        controlLayout->addStretch();

        mainLayout->addLayout(controlLayout);
    }

    void createStatusPanel(QVBoxLayout *mainLayout) {
        QHBoxLayout *statusLayout = new QHBoxLayout();

        // FPS显示
        mFpsLabel = new QLabel("FPS: 0", this);
        mFpsLabel->setStyleSheet("font-weight: bold;");

        // 总帧数显示
        mFrameLabel = new QLabel("总帧数: 0", this);

        // 进度条
        mProgressBar = new QProgressBar(this);

        statusLayout->addWidget(mFpsLabel);
        statusLayout->addWidget(mFrameLabel);
        statusLayout->addWidget(mProgressBar);

        mainLayout->addLayout(statusLayout);
    }

private slots:
    void startTest() {
        int count = mCountSpinBox->value();
        createAnimations(count);
        
        // 开始所有动画
        for (auto& widget : mAnimationWidgets) {
            widget->startAnimation();
        }

        mStartButton->setEnabled(false);
        mStopButton->setEnabled(true);
        mTestStartTime = std::chrono::high_resolution_clock::now();
    }

    void stopTest() {
        // 停止所有动画
        for (auto& widget : mAnimationWidgets) {
            widget->stopAnimation();
        }

        mStartButton->setEnabled(true);
        mStopButton->setEnabled(false);
        
        // 显示最终统计
        showFinalStats();
    }

    void updateStats() {
        if (mAnimationWidgets.empty()) return;

        double totalFPS = 0;
        size_t totalFrames = 0;

        for (const auto& widget : mAnimationWidgets) {
            totalFPS += widget->getFPS();
            totalFrames += widget->getFrameCount();
        }

        mFpsLabel->setText(QString("平均FPS: %1").arg(totalFPS / mAnimationWidgets.size(), 0, 'f', 1));
        mFrameLabel->setText(QString("总帧数: %1").arg(totalFrames));

        // 更新进度条（基于运行时间）
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double>(now - mTestStartTime).count();
        mProgressBar->setValue(int(duration) % 100);
    }

private:
    void createAnimations(int count) {
        // 清理旧的动画
        clearAnimations();

        // 创建网格布局
        QGridLayout *gridLayout = new QGridLayout(mAnimationArea);
        
        int cols = int(std::ceil(std::sqrt(count)));
        
        std::string filePath = std::string(DEMO_DIR) + "test.json";

        for (int i = 0; i < count; ++i) {
            auto widget = new QtPerformanceWidget(filePath, i + 1, this);
            mAnimationWidgets.push_back(widget);
            
            int row = i / cols;
            int col = i % cols;
            gridLayout->addWidget(widget, row, col);
        }
    }

    void clearAnimations() {
        // 停止并删除所有动画部件
        for (auto& widget : mAnimationWidgets) {
            widget->stopAnimation();
            widget->deleteLater();
        }
        mAnimationWidgets.clear();

        // 清理布局
        if (mAnimationArea->layout()) {
            QLayoutItem *child;
            while ((child = mAnimationArea->layout()->takeAt(0)) != nullptr) {
                delete child;
            }
            delete mAnimationArea->layout();
        }
    }

    void showFinalStats() {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double>(endTime - mTestStartTime).count();
        
        size_t totalFrames = 0;
        for (const auto& widget : mAnimationWidgets) {
            totalFrames += widget->getFrameCount();
        }

        std::cout << "\n=== 性能测试结果 ===" << std::endl;
        std::cout << "动画数量: " << mAnimationWidgets.size() << std::endl;
        std::cout << "测试时长: " << duration << " 秒" << std::endl;
        std::cout << "总渲染帧数: " << totalFrames << std::endl;
        std::cout << "平均FPS: " << (totalFrames / duration) << std::endl;
        std::cout << "每个动画平均FPS: " << (totalFrames / duration / mAnimationWidgets.size()) << std::endl;
    }

private:
    QSpinBox *mCountSpinBox;
    QPushButton *mStartButton;
    QPushButton *mStopButton;
    QLabel *mFpsLabel;
    QLabel *mFrameLabel;
    QProgressBar *mProgressBar;
    QWidget *mAnimationArea;
    QTimer *mUpdateTimer;
    
    std::vector<QtPerformanceWidget*> mAnimationWidgets;
    std::chrono::high_resolution_clock::time_point mTestStartTime;
};

int main(int argc, char *argv[])
{
    // 配置渲染后端
    rlottie::configureRenderBackend(rlottie::RenderBackend::Qt);

    QApplication app(argc, argv);

    QtPerformanceTest test;
    test.show();

    return app.exec();
}

#include "qtperformance.moc" 