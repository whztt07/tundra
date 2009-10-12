// For conditions of distribution and use, see copyright notice in license.txt

#include "StableHeaders.h"

#include "DebugOperatorNew.h"

#include "UICanvas.h"
#include "Profiler.h"

#ifndef Q_WS_WIN
#include <QX11Info>
#endif

#include <QCoreApplication>
#include <QPicture>
#include <QGraphicsSceneEvent>
#include <QUuid>
#include <QWidget>
#include <QGraphicsProxyWidget>
#include <QDebug>
#include <Ogre.h>
#include <QSize>

#include <OgreHardwarePixelBuffer.h>
#include <OgreTexture.h>
#include <OgreMaterial.h>
#include <OgreTextAreaOverlayElement.h>
#include <OgreFontManager.h> 
#include <OgrePanelOverlayElement.h>
#include <OgreTextureUnitState.h>

#include "MemoryLeakCheck.h"


namespace QtUI
{

UICanvas::UICanvas(): overlay_(0),
                      container_(0),
                      dirty_(true),
                      surfaceName_(""),
                      mode_(External),
                      id_(QUuid::createUuid().toString()),
                      widgets_(0),
                      locked_(false)
                     
                      
{
    setScene(new QGraphicsScene);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    QObject::connect(this->scene(),SIGNAL(changed(const QList<QRectF>&)),this,SLOT(SetDirty()));
}

UICanvas::UICanvas(Mode mode, const QSize& parentWindowSize): overlay_(0),
                               container_(0),
                               dirty_(true),
                               renderWindowSize_(parentWindowSize),
                               mode_(mode),
                               id_(QUuid::createUuid().toString()),
                               widgets_(0),
                               locked_(false)
                               
                          
{
    setScene(new QGraphicsScene);

    if (mode_ == External) 
    {
        // Deal canvas as normal QWidget. 
        // Currently do nothing
    }
    else if (mode_ == Internal)
    {
        setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
        QSize size = this->size();
        
        CreateOgreResources(size.width(), size.height());
        QObject::connect(this->scene(),SIGNAL(changed(const QList<QRectF>&)),this,SLOT(SetDirty()));
        
    }
}


UICanvas::~UICanvas()
{
    for (int i = scene_widgets_.size(); i--;)
    {
        QGraphicsProxyWidget* widget = scene_widgets_.takeAt(i);
        delete widget;
    }

    if (mode_ != External) 
    {
        Ogre::TextureManager::getSingleton().remove(surfaceName_.toStdString().c_str());
        QString surfaceMaterial = QString("mat") + id_;
        Ogre::MaterialManager::getSingleton().remove(surfaceMaterial.toStdString().c_str());

        QString containerName = QString("con") + id_;
        Ogre::OverlayManager::getSingleton().destroyOverlayElement(containerName.toStdString().c_str());

        QString overlayName = QString("over") + id_;
        Ogre::OverlayManager::getSingleton().destroy(overlayName.toStdString().c_str());
    }

    container_ = 0;
    overlay_ = 0;

    QGraphicsScene* scene = this->scene();
    delete scene;
    scene = 0;
}

void UICanvas::AddWidget(QWidget* widget)
{
    switch(mode_)
    {
    case Internal:
    {
        QGraphicsScene* scene = this->scene();
        scene_widgets_.append(scene->addWidget(widget));
        ++widgets_;
        break;
    }
    case External: 
    {
        ///todo Here lies a desing flaw do it better, this does not work.   
        if (widgets_ != 0)
        {
           // Possible memory LEAK !!!
            QGraphicsScene* scene = this->scene();
            if (widgets_ == 1)
            {
                QWidget* viewport = this->viewport();
                scene_widgets_.append(scene->addWidget(viewport));
                scene_widgets_.append(scene->addWidget(widget));
                setViewport(new QWidget);
            }
            else
                scene_widgets_.append(scene->addWidget(widget));
        }
        else
           setViewport(widget);

        ++widgets_;
        break;
    }
    default:
        break;
    }
}

void UICanvas::SetPosition(int x, int y)
{
    switch (mode_)
    {
    case External:
    {
        move(x,y);
        break;
    }
    case Internal:
    {
        float relX = x/double(renderWindowSize_.width()), relY = y/double(renderWindowSize_.height());
        container_->setPosition(relX, relY);
        move(x,y);
        dirty_ = true;
        break;
    }
    default:
        break;
    }
}

void UICanvas::SetZOrder(int order)
{
    if ( mode_ != External)
        overlay_->setZOrder(order);
}

int UICanvas::GetZOrder() const
{
    if (mode_ != External)
        return overlay_->getZOrder();

    return -1;
}

void UICanvas::SetTop()
{
    emit ToTop(id_);
}

void UICanvas::SetBack()
{
    emit ToBack(id_);
}

void UICanvas::Activate()
{
#ifdef Q_WS_WIN
    QSize current_size = size();
    //Qt::FramelessWindowHint
    setWindowFlags(Qt::FramelessWindowHint);
    SetCanvasSize(1,1);
    show();
    WId win_id = effectiveWinId();
    ShowWindow(static_cast<HWND>(win_id),SW_HIDE);
    SetCanvasSize(current_size.width(), current_size.height());
#endif 

}

QPoint UICanvas::MapToCanvas(int x, int y)
{
    if ( mode_ != External)
    {
        x -= container_->getLeft() * renderWindowSize_.width();
        y -= container_->getTop() * renderWindowSize_.height();
    }
     return QPoint(x,y);
}

QPointF UICanvas::GetPosition() const
{
    QPointF position;

    switch(mode_)
    {
    case External:
    {
        position = pos();
        break;
    }
    case Internal:
    {
        double width = container_->getLeft();
        double height = container_->getTop();
        
        // Calculate position in render window coordinate frame. 
        position.rx() = width * renderWindowSize_.width();
        position.ry() = height * renderWindowSize_.height();
        break;
    }
    default:
        break;
    }

    return position;
}

bool UICanvas::IsHidden() const
{
    if (mode_ != External)
    {
        if (overlay_)
            return !overlay_->isVisible(); 
        else
            return true;
    }
    else
    {
        return !isVisible();
    }
}

void UICanvas::SetCanvasSize(int width, int height)
{
    resize(width, height);

    if ( mode_ != External)
    {
        CreateOgreResources(width, height);
        Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton().getByName(surfaceName_.toStdString().c_str());
        float relWidth = (float)texture->getWidth()/double(renderWindowSize_.width());
        float relHeight = (float)texture->getHeight()/double(renderWindowSize_.height());
        container_->setDimensions(relWidth, relHeight);
        
    }

    // Repaint canvas. 
    dirty_ = true;
    RenderSceneToOgreSurface();
}

void UICanvas::SetRenderWindowSize(const QSize& size)
{
    renderWindowSize_ = size;
    if ( mode_ != External)
    {
        Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton().getByName(surfaceName_.toStdString().c_str());
        float relWidth = (float)texture->getWidth()/double(renderWindowSize_.width());
        float relHeight = (float)texture->getHeight()/double(renderWindowSize_.height());
        container_->setDimensions(relWidth, relHeight);
        dirty_ = true;
    }
}


void UICanvas::Resize(int height, int width, CanvasSide side)
{
    if ( mode_ == External)
    {
        SetCanvasSize(width, height);
        return;
    }
    
    QSize current_size = this->size();

    // Assure that new size is bigger then canvas minium size. 
    QSize minium = this->minimumSize();
    if ( height < minium.height() || width < minium.width() || height <= 0 || width <= 0)
        return;

    Ogre::PanelOverlayElement* element = static_cast<Ogre::PanelOverlayElement* >(container_);  
    Ogre::Real u1 = 0, v1 = 0, u2 = 0, v2= 0;
    Ogre::Real top = element->getTop();
    Ogre::Real left = element->getLeft();
    element->getUV(u1,v1,u2,v2);
    
    if ( current_size.width() > width || current_size.height() > height)
    {
        // Smaller
        

        if ( current_size.width() > width )
        {
            // Change width
            
            int diff = current_size.width() - width;

            Ogre::Real ratio = width/Ogre::Real(current_size.width());
            Ogre::Real current_container_height = element->getHeight();
            Ogre::Real new_container_width = width/Ogre::Real(renderWindowSize_.width());
            
         
            Ogre::Real xPos = 0.0;

            switch (side)
            {
                case Left:
                {
                    u1 = Ogre::Real(1.0)-ratio;
                   
                    xPos = left + diff/Ogre::Real(renderWindowSize_.width());
                    element->setPosition(xPos,top);
             
                    break;
                }
                case Right:
                {
                    u2 = ratio;
                    xPos = left - diff/Ogre::Real(renderWindowSize_.width());
                    element->setPosition(left,top);
                    break;
                }

            }
            element->setUV(u1,v1,u2,v2);
           
            element->setDimensions( new_container_width, current_container_height);

        }
        else
        {
            // height

            int diff = current_size.height() - height;
            Ogre::Real ratio =height/Ogre::Real(current_size.height());
            Ogre::Real relHeight = height/Ogre::Real(renderWindowSize_.height());
            
            Ogre::Real loc = renderWindowSize_.height() * top;
        
            switch ( side )
            {
            case Top:
                {
                    v2 = ratio;
                    Ogre::Real yPos = top + diff/Ogre::Real(renderWindowSize_.height());
                    element->setPosition(left, yPos);
                    break;
                    
                    
                }
            case Bottom:
                {
                    v1 = Ogre::Real(1.0) -  ratio;
                    element->setPosition(left, top);
                    break;
                }

            }
            
            
            Ogre::Real current_container_width = element->getWidth();
            element->setUV(u1,v1,u2,v2);
            element->setDimensions( current_container_width, relHeight);

        }

    }
    else 
    {
        // Growing.

    }

    resize(width,height);
    dirty_ = true;
    RenderSceneToOgreSurface();

}




void UICanvas::SetCanvasWindowTitle(QString title) 
{
	setWindowTitle(title);
}

void UICanvas::SetCanvasWindowIcon(QIcon &icon) 
{
	setWindowIcon(icon);
}


void UICanvas::ResizeOgreTexture(int width, int height)
{
    Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton().getByName(surfaceName_.toStdString().c_str());
    texture->freeInternalResources();
    texture->setWidth(width);
    texture->setHeight(height);
    texture->createInternalResources();
}

void UICanvas::CreateOgreResources(int width, int height)
{
    // If we've already created the resources, just resize the texture to a new size.
    if ( surfaceName_ != "")
    {
        Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton().getByName(surfaceName_.toStdString().c_str());
        if (width == texture->getWidth() && height == texture->getHeight())
            return;
        ResizeOgreTexture(width, height);
        assert(overlay_);
        assert(container_);
        return;
    }

    QString overlayName = QString("over") + id_;
    overlay_ = Ogre::OverlayManager::getSingleton().create(overlayName.toStdString().c_str());

    QString containerName = QString("con") + id_;
    container_ = static_cast<Ogre::OverlayContainer*>(Ogre::OverlayManager::getSingleton()
                                     .createOverlayElement("Panel", containerName.toStdString().c_str()));

    // This is not done so currently -- tuoki 
    // Make the overlay cover 100% of the render window. Note that the UI surface will be 
    // rendered pixel perfect without stretching only if the GraphicsView surface dimension 
    // matches the render window size.
    //container_->setDimensions(1.0,1.0);

    container_->setPosition(0,0);

    //overlay_->setZOrder(1);

    // Add container in default overlay
    overlay_->add2D(container_);

    ///\todo Tell Ogre not to generate a mipmap chain. This texture only needs to have only one
    /// mip level.

    surfaceName_ = QString("tex") + id_;

    Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton().createManual(surfaceName_.toStdString().c_str(), 
                                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, 
                                Ogre::TEX_TYPE_2D, width, height, 0, 
                                Ogre::PF_A8R8G8B8, Ogre::TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);


    QString surfaceMaterial = QString("mat") + id_;
    Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().create(surfaceMaterial.toStdString().c_str(),
                                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

    Ogre::TextureUnitState *state = material->getTechnique(0)->getPass(0)->createTextureUnitState();
    material->getTechnique(0)->getPass(0)->setSceneBlending(Ogre::SBF_SOURCE_ALPHA, Ogre::SBF_ONE_MINUS_SOURCE_ALPHA);
    state->setTextureName(surfaceName_.toStdString().c_str());

    // Generate pixel perfect texture. 
    float relWidth = (float)texture->getWidth()/double(renderWindowSize_.width());
    float relHeight = (float)texture->getHeight()/double(renderWindowSize_.height());
    container_->setDimensions(relWidth, relHeight);

    container_->setMaterialName(surfaceMaterial .toStdString().c_str());
    container_->setEnabled(true);
    container_->setColour(Ogre::ColourValue(1,1,1,1));
}

void UICanvas::Show()
{
    if ( mode_ != External)
    {
        
        
        QList<QGraphicsProxyWidget* >::iterator iter = scene_widgets_.begin();
        for (; iter != scene_widgets_.end(); ++iter)
            (*iter)->show();
            
        

        container_->show();
        overlay_->show();
      

        dirty_ = true;
        RenderSceneToOgreSurface();
    }
    else
        show();
}


void UICanvas::Hide()
{
    if ( mode_ != External)
    {
        hide();

        QList<QGraphicsProxyWidget* >::iterator iter = scene_widgets_.begin();
        for (; iter != scene_widgets_.end(); ++iter)
            (*iter)->hide();
          
        container_->hide();
        overlay_->hide();
        dirty_ = true;
        RenderSceneToOgreSurface();
    }
    else
        hide();
}

void UICanvas::drawBackground(QPainter *painter, const QRectF &rect)
{
    QBrush black(Qt::transparent);
    painter->fillRect(rect, black);
}

void UICanvas::RenderSceneToOgreSurface()
{
    // Render if and only if scene is dirty.
    if (!dirty_ || mode_ == External)
        return;

    PROFILE(RenderSceneToOgre);

    // We draw the GraphicsView area to an offscreen QPixmap and blit that onto the Ogre GPU surface.
    QPixmap pixmap(this->size());
    {
        PROFILE(FillEmpty);
        pixmap.fill(Qt::transparent);
    }
    assert(pixmap.hasAlphaChannel());
    QImage img = pixmap.toImage();

    QPainter painter(&img);
    QRectF destRect(0, 0, pixmap.width(), pixmap.height());
    QRect sourceRect(0, 0, pixmap.width(), pixmap.height());
    {
        PROFILE(RenderUI);
        this->render(&painter, destRect, sourceRect);
    }
    assert(img.hasAlphaChannel());

    ///\todo Can optimize an extra blit away if we paint directly onto the GPU surface.
    Ogre::Box dimensions(0,0, img.rect().width(), img.rect().height());
    Ogre::PixelBox pixel_box(dimensions, Ogre::PF_A8R8G8B8, (void*)img.bits());
    {
        PROFILE(UIToOgreBlit);
        Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton().getByName(surfaceName_.toStdString().c_str());
        texture->getBuffer()->blitFromMemory(pixel_box);
    }

    dirty_ = false;
}

void UICanvas::Render()
{
    if ( mode_ != External && container_->isVisible())
        RenderSceneToOgreSurface();    
}

}
