/*
 * Qt渲染后端示例程序
 * 这个示例展示了如何使用Qt渲染后端播放Lottie动画
 */

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QPainter>
#include <QPushButton>
#include <QSlider>
#include <QSplitter>
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

        // 创建主布局
        QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

        // 创建左侧文件列表区域
        QWidget *leftPanel = new QWidget(this);
        leftPanel->setMaximumWidth(250);
        leftPanel->setMinimumWidth(200);
        QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);

        // 添加选择文件夹按钮
        QPushButton *openFolderButton = new QPushButton("选择文件夹", this);
        connect(openFolderButton, &QPushButton::clicked, this, &MainWindow::openFolder);
        leftLayout->addWidget(openFolderButton);

        // 添加文件列表
        mFileList = new QListWidget(this);
        connect(mFileList, &QListWidget::itemClicked, this, &MainWindow::onFileSelected);
        leftLayout->addWidget(mFileList);

        mainLayout->addWidget(leftPanel);

        // 创建右侧动画显示区域
        QWidget *rightPanel = new QWidget(this);
        QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);

        // 创建动画显示窗口
        mLottieWidget = new LottieWidget(this);
        rightLayout->addWidget(mLottieWidget);

        // 创建控制面板
        QHBoxLayout *controlLayout = new QHBoxLayout();
        rightLayout->addLayout(controlLayout);

        // 添加播放按钮
        mPlayButton = new QPushButton("播放", this);
        connect(mPlayButton, &QPushButton::clicked, this, &MainWindow::togglePlay);
        controlLayout->addWidget(mPlayButton);

        // 添加当前文件标签
        mCurrentFileLabel = new QLabel("未选择文件", this);
        controlLayout->addWidget(mCurrentFileLabel);

        mainLayout->addWidget(rightPanel);

        // 设置窗口标题和大小
        setWindowTitle("rlottie Qt渲染示例");
        resize(800, 600);
    }

private slots:
    void openFolder()
    {
        QString folderPath = QFileDialog::getExistingDirectory(
            this, "选择包含Lottie文件的文件夹", QString());

        if (!folderPath.isEmpty()) {
            scanJsonFiles(folderPath);
        }
    }

    void onFileSelected(QListWidgetItem *item)
    {
        QString filePath = mCurrentFolderPath + "/" + item->text();
        
        if (mLottieWidget->loadAnimation(filePath)) {
            mIsPlaying = false;
            mPlayButton->setText("播放");
            mCurrentFileLabel->setText("当前文件: " + item->text());
            
            // 自动开始播放
            mLottieWidget->play();
            mPlayButton->setText("暂停");
            mIsPlaying = true;
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
    void scanJsonFiles(const QString &folderPath)
    {
        mCurrentFolderPath = folderPath;
        mFileList->clear();

        QDir dir(folderPath);
        QStringList nameFilters;
        nameFilters << "*.json" << "*.lottie";
        
        QFileInfoList fileList = dir.entryInfoList(nameFilters, QDir::Files);
        
        if (fileList.isEmpty()) {
            QListWidgetItem *noFilesItem = new QListWidgetItem("未找到JSON文件");
            noFilesItem->setFlags(noFilesItem->flags() & ~Qt::ItemIsSelectable);
            mFileList->addItem(noFilesItem);
            return;
        }

        foreach (const QFileInfo &fileInfo, fileList) {
            QListWidgetItem *item = new QListWidgetItem(fileInfo.fileName());
            mFileList->addItem(item);
        }

                 setWindowTitle("rlottie Qt渲染示例 - " + QFileInfo(folderPath).fileName());
     }

    LottieWidget *mLottieWidget;
    QPushButton  *mPlayButton;
    QListWidget  *mFileList;
    QLabel       *mCurrentFileLabel;
    QString       mCurrentFolderPath;
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