/*
 * Qt版本的Lottie文件查看器
 * 可以浏览多个Lottie文件
 */

#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QMainWindow>
#include <QPainter>
#include <QPushButton>
#include <QWidget>
#include <QTimer>
#include <QDir>
#include <QFileInfo>
#include <memory>
#include <iostream>
#include <functional>

#include "rlottie.h"

class QtViewerWidget : public QWidget {
    Q_OBJECT

public:
    QtViewerWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        setMinimumSize(400, 400);
        
        // 创建缓冲区
        mBuffer = std::make_unique<uint32_t[]>(400 * 400);
        mImage = QImage((uchar*)mBuffer.get(), 400, 400, 400 * 4, 
                       QImage::Format_ARGB32_Premultiplied);
        mImage.fill(Qt::transparent);
        
        // 设置定时器
        mTimer = new QTimer(this);
        connect(mTimer, &QTimer::timeout, this, &QtViewerWidget::renderFrame);
    }

    bool loadFile(const QString &filePath) {
        // 停止当前动画
        if (mTimer->isActive()) {
            mTimer->stop();
        }

        // 加载新动画
        mAnimation = rlottie::Animation::loadFromFile(filePath.toStdString());
        if (mAnimation) {
            mAnimation->setRenderBackend(rlottie::RenderBackend::Qt);
            mCurrentFrame = 0;
            
            // 清空画布
            mImage.fill(Qt::transparent);
            
            // 开始播放
            mTimer->start(1000 / mAnimation->frameRate());
            return true;
        }
        return false;
    }

    void play() {
        if (mAnimation && !mTimer->isActive()) {
            mTimer->start(1000 / mAnimation->frameRate());
        }
    }

    void pause() {
        if (mTimer->isActive()) {
            mTimer->stop();
        }
    }

    bool isPlaying() const {
        return mTimer->isActive();
    }

    size_t getCurrentFrame() const { return mCurrentFrame; }
    size_t getTotalFrames() const { 
        return mAnimation ? mAnimation->totalFrame() : 0;
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        
        if (mAnimation) {
            // 居中绘制
            QRect sourceRect = mImage.rect();
            QRect targetRect = rect();
            
            // 保持宽高比
            double scale = std::min(double(targetRect.width()) / sourceRect.width(),
                                   double(targetRect.height()) / sourceRect.height());
            
            int scaledWidth = int(sourceRect.width() * scale);
            int scaledHeight = int(sourceRect.height() * scale);
            
            QRect drawRect((targetRect.width() - scaledWidth) / 2,
                          (targetRect.height() - scaledHeight) / 2,
                          scaledWidth, scaledHeight);
            
            painter.drawImage(drawRect, mImage);
        } else {
            // 绘制提示文本
            painter.setPen(Qt::gray);
            painter.drawText(rect(), Qt::AlignCenter, "请选择一个Lottie文件");
        }
    }

private slots:
    void renderFrame() {
        if (!mAnimation) return;

        // 渲染当前帧
        rlottie::Surface surface(mBuffer.get(), 400, 400, 400 * 4);
        mAnimation->renderSync(mCurrentFrame, surface);

        // 更新帧计数器
        mCurrentFrame = (mCurrentFrame + 1) % mAnimation->totalFrame();

        // 重绘
        update();
        
        // 发送帧更新信号
        emit frameUpdated();
    }

signals:
    void frameUpdated();

private:
    std::unique_ptr<rlottie::Animation> mAnimation;
    std::unique_ptr<uint32_t[]> mBuffer;
    QTimer *mTimer;
    QImage mImage;
    size_t mCurrentFrame = 0;
};

class QtViewer : public QMainWindow {
    Q_OBJECT

public:
    QtViewer(QWidget *parent = nullptr) : QMainWindow(parent) {
        // 设置窗口
        setWindowTitle("rlottie Qt文件查看器");
        resize(800, 600);

        // 创建中央窗口部件
        QWidget *centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);

        // 创建主布局
        QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

        // 创建左侧面板
        createLeftPanel(mainLayout);

        // 创建右侧显示区域
        createRightPanel(mainLayout);
        
        // 扫描示例文件
        scanExampleFiles();
    }

private:
    void createLeftPanel(QHBoxLayout *mainLayout) {
        QWidget *leftPanel = new QWidget(this);
        leftPanel->setMaximumWidth(250);
        leftPanel->setMinimumWidth(200);
        
        QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);

        // 文件列表
        QLabel *listLabel = new QLabel("Lottie文件:", this);
        mFileList = new QListWidget(this);
        connect(mFileList, &QListWidget::itemClicked, this, &QtViewer::onFileSelected);

        // 控制按钮
        mPlayButton = new QPushButton("播放", this);
        connect(mPlayButton, &QPushButton::clicked, this, &QtViewer::togglePlay);

        leftLayout->addWidget(listLabel);
        leftLayout->addWidget(mFileList);
        leftLayout->addWidget(mPlayButton);

        mainLayout->addWidget(leftPanel);
    }

    void createRightPanel(QHBoxLayout *mainLayout) {
        QWidget *rightPanel = new QWidget(this);
        QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);

        // 动画显示区域
        mViewerWidget = new QtViewerWidget(this);
        connect(mViewerWidget, &QtViewerWidget::frameUpdated, this, &QtViewer::updateFrameInfo);
        rightLayout->addWidget(mViewerWidget);

        // 信息显示
        mInfoLabel = new QLabel("准备就绪", this);
        mInfoLabel->setStyleSheet("padding: 5px; background-color: #f0f0f0;");
        rightLayout->addWidget(mInfoLabel);

        mainLayout->addWidget(rightPanel);
    }

    void scanExampleFiles() {
        // 扫描示例文件夹中的JSON文件
        QString exampleDir = QString(DEMO_DIR);
        QDir dir(exampleDir);
        
        if (!dir.exists()) {
            mFileList->addItem("示例文件夹不存在");
            return;
        }

        QStringList nameFilters;
        nameFilters << "*.json" << "*.lottie";
        
        QFileInfoList fileList = dir.entryInfoList(nameFilters, QDir::Files);
        
        if (fileList.isEmpty()) {
            mFileList->addItem("未找到Lottie文件");
            return;
        }

        for (const QFileInfo &fileInfo : fileList) {
            QListWidgetItem *item = new QListWidgetItem(fileInfo.fileName());
            item->setData(Qt::UserRole, fileInfo.absoluteFilePath());
            mFileList->addItem(item);
        }
    }

private slots:
    void onFileSelected(QListWidgetItem *item) {
        QString filePath = item->data(Qt::UserRole).toString();
        
        if (filePath.isEmpty()) return;
        
        if (mViewerWidget->loadFile(filePath)) {
            mCurrentFile = item->text();
            mPlayButton->setText("暂停");
            updateFrameInfo();
        }
    }

    void togglePlay() {
        if (mViewerWidget->isPlaying()) {
            mViewerWidget->pause();
            mPlayButton->setText("播放");
        } else {
            mViewerWidget->play();
            mPlayButton->setText("暂停");
        }
    }

    void updateFrameInfo() {
        if (mCurrentFile.isEmpty()) return;
        
        QString info = QString("文件: %1 | 帧: %2/%3")
                      .arg(mCurrentFile)
                      .arg(mViewerWidget->getCurrentFrame() + 1)
                      .arg(mViewerWidget->getTotalFrames());
        
        mInfoLabel->setText(info);
    }

private:
    QListWidget *mFileList;
    QPushButton *mPlayButton;
    QtViewerWidget *mViewerWidget;
    QLabel *mInfoLabel;
    QString mCurrentFile;
};

int main(int argc, char *argv[])
{
    // 配置渲染后端
    rlottie::configureRenderBackend(rlottie::RenderBackend::Qt);

    QApplication app(argc, argv);

    QtViewer viewer;
    viewer.show();

    return app.exec();
}

#include "qtviewer.moc" 