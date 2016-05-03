#include "albumpanel.h"
#include "controller/databasemanager.h"
#include "controller/importer.h"
#include "widgets/importframe.h"
#include <dimagebutton.h>
#include <QFileInfo>
#include <QDropEvent>
#include <QPushButton>
#include <QPointer>
#include <QDebug>

namespace {

const int MIN_ICON_SIZE = 96;

}   // namespace

using namespace Dtk::Widget;

AlbumPanel::AlbumPanel(QWidget *parent)
    : ModulePanel(parent)
{
    setAcceptDrops(true);
    initMainStackWidget();
    initStyleSheet();
}


QWidget *AlbumPanel::toolbarBottomContent()
{
    QWidget *tBottomContent = new QWidget;
    tBottomContent->setStyleSheet(this->styleSheet());

    m_slider = new DSlider(Qt::Horizontal);
    m_slider->setMinimum(0);
    m_slider->setMaximum(9);
    m_slider->setValue(0);
    m_slider->setFixedWidth(120);
    connect(m_slider, &DSlider::valueChanged, this, [=] (int multiple) {
        int newSize = MIN_ICON_SIZE + multiple * 32;
        if (m_mainStackWidget->currentWidget() == m_imagesView) {
            m_imagesView->setIconSize(QSize(newSize, newSize));
        }
        else {
            m_albumsView->setItemSize(QSize(newSize, newSize));
        }
    });

    m_countLabel = new QLabel;
    m_countLabel->setAlignment(Qt::AlignLeft);
    m_countLabel->setObjectName("CountLabel");

    updateBottomToolbarContent();

    connect(m_signalManager, &SignalManager::imageCountChanged,
            this, &AlbumPanel::updateBottomToolbarContent, Qt::DirectConnection);
    connect(m_signalManager, &SignalManager::selectImageFromTimeline, this, [=] {
        emit m_signalManager->updateTopToolbarLeftContent(toolbarTopLeftContent());
        emit m_signalManager->updateTopToolbarMiddleContent(toolbarTopMiddleContent());
    });

    QHBoxLayout *layout = new QHBoxLayout(tBottomContent);
    layout->setContentsMargins(13, 0, 5, 0);
    layout->setSpacing(0);
    layout->addWidget(m_countLabel, 1, Qt::AlignLeft);
    layout->addStretch(1);
    layout->addWidget(m_slider, 1, Qt::AlignRight);

    return tBottomContent;
}

QWidget *AlbumPanel::toolbarTopLeftContent()
{
    QWidget *tTopleftContent = new QWidget;
    tTopleftContent->setStyleSheet(this->styleSheet());
    QLabel *icon = new QLabel;
    // TODO update icon path
    icon->setPixmap(QPixmap(":/images/logo/resources/images/logo/deepin_image_viewer_24.png"));

    QHBoxLayout *layout = new QHBoxLayout(tTopleftContent);
    layout->setContentsMargins(8, 0, 0, 0);

    if (m_mainStackWidget->currentWidget() == m_imagesView) {

        DImageButton *returnButton = new DImageButton();
        returnButton->setNormalPic(":/images/resources/images/return_normal.png");
        returnButton->setHoverPic(":/images/resources/images/return_hover.png");
        returnButton->setPressPic(":/images/resources/images/return_press.png");
        connect(returnButton, &DImageButton::clicked, this, [=] {
            m_mainStackWidget->setCurrentWidget(m_albumsView);
            // Make sure top toolbar content still show as album content
            // during adding images from timeline
            emit m_signalManager->gotoPanel(this);
        });

        QLabel *rt = new QLabel();
        rt->setObjectName("ReturnLabel");
        rt->setText(tr("Return"));

        layout->addWidget(icon);
        layout->addWidget(returnButton);
        layout->addWidget(rt);
        layout->addStretch(1);
    }
    else {
        layout->addWidget(icon);
        layout->addStretch(1);
    }

    return tTopleftContent;
}

QWidget *AlbumPanel::toolbarTopMiddleContent()
{
    QWidget *tTopMiddleContent = new QWidget;

    DImageButton *timelineButton = new DImageButton();
    timelineButton->setNormalPic(":/images/resources/images/timeline_normal.png");
    timelineButton->setHoverPic(":/images/resources/images/timeline_hover.png");
    connect(timelineButton, &DImageButton::clicked, this, [=] {
        qDebug() << "Change to Timeline Panel...";
        emit needGotoTimelinePanel();
    });

    QLabel *albumLabel = new QLabel();
    albumLabel->setPixmap(QPixmap(":/images/resources/images/album_active.png"));

    DImageButton *searchButton = new DImageButton();
    searchButton->setNormalPic(":/images/resources/images/search_normal_24px.png");
    searchButton->setHoverPic(":/images/resources/images/search_hover_24px.png");
    searchButton->setPressPic(":/images/resources/images/search_press_24px.png");
    connect(searchButton, &DImageButton::clicked, this, [=] {
        qDebug() << "Change to Search Panel...";
        emit needGotoSearchPanel();
    });

    QHBoxLayout *layout = new QHBoxLayout(tTopMiddleContent);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(20);
    layout->addStretch(1);
    layout->addWidget(timelineButton);
    layout->addWidget(albumLabel);
    layout->addWidget(searchButton);
    layout->addStretch(1);

    return tTopMiddleContent;
}

QWidget *AlbumPanel::extensionPanelContent()
{
    return NULL;
}

void AlbumPanel::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    if (!urls.isEmpty()) {
        for (QUrl url : urls) {
            QFileInfo info(url.toLocalFile());
            if (info.isDir()) {
                Importer::instance()->importFromPath(url.toLocalFile());
            }
            else {
                if (m_dbManager->supportImageType().indexOf(info.suffix()) != 0) {
                    Importer::instance()->importSingleFile(url.toLocalFile());
                }
            }
        }
    }
}

void AlbumPanel::dragEnterEvent(QDragEnterEvent *event)
{
    event->setDropAction(Qt::CopyAction);
    event->accept();
}

void AlbumPanel::initMainStackWidget()
{
    initImagesView();
    initAlbumsView();

    m_mainStackWidget = new QStackedWidget;
    m_mainStackWidget->setContentsMargins(0, 0, 0, 0);
    m_mainStackWidget->addWidget(new ImportFrame(this));
    m_mainStackWidget->addWidget(m_albumsView);
    m_mainStackWidget->addWidget(m_imagesView);
    //show import frame if no images in database
    m_mainStackWidget->setCurrentIndex(m_dbManager->imageCount() > 0 ? 1 : 0);
    connect(m_mainStackWidget, &QStackedWidget::currentChanged, this, [=] {
        updateBottomToolbarContent();
        emit SignalManager::instance()->
                updateTopToolbarLeftContent(toolbarTopLeftContent());
    });
    connect(m_signalManager, &SignalManager::imageCountChanged, this, [=] {
        m_mainStackWidget->setCurrentIndex(m_dbManager->imageCount() > 0 ? 1 : 0);
    }, Qt::DirectConnection);

    QLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_mainStackWidget);
}

void AlbumPanel::initAlbumsView()
{
    m_albumsView = new AlbumsView(this);
    QStringList albums = m_dbManager->getAlbumNameList();
    for (const QString name : albums) {
        m_albumsView->addAlbum(m_dbManager->getAlbumInfo(name));
    }
    connect(m_albumsView, &AlbumsView::openAlbum, this, &AlbumPanel::onOpenAlbum);
}

void AlbumPanel::initImagesView()
{
    m_imagesView = new ImagesView(this);
}

void AlbumPanel::initStyleSheet()
{
    QFile sf(":/qss/resources/qss/album.qss");
    if (!sf.open(QIODevice::ReadOnly)) {
        qWarning() << "Open style-sheet file error:" << sf.errorString();
        return;
    }

    setStyleSheet(QString(sf.readAll()));
    sf.close();
}

void AlbumPanel::updateBottomToolbarContent()
{
    if (! this->isVisible()) {
        return;
    }

    const int albumCount = m_dbManager->albumsCount();
    const int imagesCount = m_dbManager->getImagesCountByAlbum(m_currentAlbum);
    const bool inAlbumsFrame = m_mainStackWidget->currentIndex() == 1;
    const int count = inAlbumsFrame ? albumCount : imagesCount;

    if (count <= 1) {
        m_countLabel->setText(QString("%1 %2")
                              .arg(count)
                              .arg(inAlbumsFrame ? tr("Album") : tr("Image")));
    }
    else {
        m_countLabel->setText(QString("%1 %2")
                              .arg(count)
                              .arg(inAlbumsFrame ? tr("Albums") : tr("Images")));
    }

    //set width to 1px for layout center
    m_slider->setFixedWidth(count > 0 ? 120 : 1);
}

void AlbumPanel::onOpenAlbum(const QString &album)
{
    qDebug() << "Open Album : " << album;
    m_currentAlbum = album;
    m_mainStackWidget->setCurrentIndex(2);
    m_imagesView->setAlbum(album);
}

