/*
 * Qt渲染后端示例程序
 * 这个示例展示了如何使用Qt渲染后端播放Lottie动画
 */

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QFileDialog>
#include <QImage>
#include <QLabel>
#include <QMainWindow>
#include <QPainter>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>

#include <iostream>

#include "rlottie.h"
#include "vdebug.h"

class LottieWidget : public QWidget {
    Q_OBJECT

public:
    LottieWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        setMinimumSize(300, 300);

        // 创建动画渲染定时器
        mTimer = new QTimer(this);
        connect(mTimer, &QTimer::timeout, this, &LottieWidget::renderFrame);
    }

    ~LottieWidget()
    {
        if (mTimer->isActive()) mTimer->stop();
    }

    // 加载Lottie文件
    bool loadAnimation(const QString &filePath)
    {
        if (mAnimation) {
            mAnimation.reset();
        }

        mAnimation = rlottie::Animation::loadFromFile(filePath.toStdString());
        if (!mAnimation) {
            return false;
        }

        // 设置使用Qt渲染后端
        mAnimation->setRenderBackend(rlottie::RenderBackend::Qt);

        // 获取动画尺寸
        size_t width = 0, height = 0;
        mAnimation->size(width, height);
        mSize = QSize(width, height);

        // 重置帧计数器
        mCurrentFrame = 0;

        // 创建图像缓冲区
        mBuffer = std::make_unique<uint32_t[]>(width * height);

        // 更新widget大小
        setMinimumSize(mSize);
        resize(mSize);

        // 创建QImage
        mImage = QImage((uchar *)mBuffer.get(), width, height, width * 4,
                        QImage::Format_ARGB32_Premultiplied);
        mImage.fill(Qt::transparent);

        return true;
    }

    // 播放动画
    void play()
    {
        if (!mAnimation) return;

        // 计算帧率
        int fps = mAnimation->frameRate();
        mTimer->start(1000 / fps);
    }

    // 暂停动画
    void pause()
    {
        if (mTimer->isActive()) mTimer->stop();
    }

    // 设置帧
    void setFrame(size_t frame)
    {
        if (!mAnimation) return;

        mCurrentFrame = frame % mAnimation->totalFrame();
        renderFrame();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        if (!mAnimation) return;

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        // 居中绘制
        QRect targetRect = QRect((width() - mImage.width()) / 2,
                                 (height() - mImage.height()) / 2,
                                 mImage.width(), mImage.height());

        painter.drawImage(targetRect, mImage);
    }

private slots:
    void renderFrame()
    {
        if (!mAnimation) return;

        // 渲染当前帧
        rlottie::Surface surface(mBuffer.get(), mSize.width(), mSize.height(),
                                 mSize.width() * 4);

        mAnimation->renderSync(mCurrentFrame, surface);

        // 更新帧计数器
        mCurrentFrame = (mCurrentFrame + 1) % mAnimation->totalFrame();

        // 重绘widget
        update();
    }

private:
    std::unique_ptr<rlottie::Animation> mAnimation;
    std::unique_ptr<uint32_t[]>         mBuffer;
    QTimer                             *mTimer;
    QImage                              mImage;
    QSize                               mSize;
    size_t                              mCurrentFrame = 0;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr) : QMainWindow(parent)
    {
        // 创建中央窗口部件
        QWidget *centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);

        // 创建布局
        QVBoxLayout *layout = new QVBoxLayout(centralWidget);

        // 创建动画显示窗口
        mLottieWidget = new LottieWidget(this);
        layout->addWidget(mLottieWidget);

        // 创建控制面板
        QHBoxLayout *controlLayout = new QHBoxLayout();
        layout->addLayout(controlLayout);

        // 添加打开文件按钮
        QPushButton *openButton = new QPushButton("打开文件", this);
        connect(openButton, &QPushButton::clicked, this, &MainWindow::openFile);
        controlLayout->addWidget(openButton);

        // 添加播放按钮
        mPlayButton = new QPushButton("播放", this);
        connect(mPlayButton, &QPushButton::clicked, this,
                &MainWindow::togglePlay);
        controlLayout->addWidget(mPlayButton);

        // 设置窗口标题和大小
        setWindowTitle("rlottie Qt渲染示例");
        resize(500, 500);
    }

private slots:
    void openFile()
    {
        QString filePath =
            QFileDialog::getOpenFileName(this, "选择Lottie文件", QString(),
                                         "Lottie Files (*.json *.lottie)");

        if (!filePath.isEmpty()) {
            if (mLottieWidget->loadAnimation(filePath)) {
                mIsPlaying = false;
                mPlayButton->setText("播放");
                setWindowTitle("rlottie Qt渲染示例 - " +
                               QFileInfo(filePath).fileName());
            }
        }
    }

    void togglePlay()
    {
        if (mIsPlaying) {
            mLottieWidget->pause();
            mPlayButton->setText("播放");
        } else {
            mLottieWidget->play();
            mPlayButton->setText("暂停");
        }

        mIsPlaying = !mIsPlaying;
    }

private:
    LottieWidget *mLottieWidget;
    QPushButton  *mPlayButton;
    bool          mIsPlaying = false;
};

int main(int argc, char *argv[])
{
    

    // 配置全局渲染后端
    rlottie::configureRenderBackend(rlottie::RenderBackend::Qt);

    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    // std::cout << "测试日志功能..." << std::endl;
    // vDebug << "调试信息";      // LogLevel::INFO
    // vWarning << "警告信息";    // LogLevel::WARN
    // vCritical << "严重错误";   // LogLevel::CRIT
    
    return app.exec();
}

#include "qtplayer.moc"