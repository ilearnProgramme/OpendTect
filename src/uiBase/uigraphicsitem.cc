/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uigraphicsitem.h"
#include "uigraphicsscene.h"

#include "uicursor.h"
#include "uimain.h"
#include "uistring.h"
#include "bufstringset.h"

#include "draw.h"

#include "odgraphicsitem.h"

#include <QBrush>
#include <QCursor>
#include <QGraphicsItem>
#include <QGraphicsItemGroup>
#include <QGraphicsScene>
#include <QPen>
#include <QTransform>


mUseQtnamespace

class ODGraphicsItem : public QGraphicsItem
{

QRectF boundingRect() const
{ return QRectF(); }

void paint( QPainter* painter, const QStyleOptionGraphicsItem* option,
	    QWidget* widget )
{
}

};

uiGraphicsItem::uiGraphicsItem()
    : qgraphicsitem_(new ODGraphicsItem)
    , scene_(0)
    , id_(getNewID())
    , selected_(false)
    , translation_( 0, 0 )
    , scale_( 1, 1 )
    , angle_( 0 )
{
}


uiGraphicsItem::uiGraphicsItem( QGraphicsItem* itm )
    : qgraphicsitem_(itm)
    , scene_(0)
    , id_(getNewID())
    , selected_(false)
    , translation_( 0, 0 )
    , scale_( 1, 1 )
    , angle_( 0 )
{
}


uiGraphicsItem::~uiGraphicsItem()
{
    if ( scene_ )
    {
	scene_->removeItem( this );
	delete qgraphicsitem_;
    }
}


void uiGraphicsItem::setScene( uiGraphicsScene* scene )
{ scene_ = scene; }


int uiGraphicsItem::getNewID()
{
    mDefineStaticLocalObject( Threads::Mutex, mutex, );
    Threads::MutexLocker lock( mutex );
    mDefineStaticLocalObject( int, curid, = 1 );
    return curid++;
}


void uiGraphicsItem::show()	{ qgraphicsitem_->show(); }
void uiGraphicsItem::hide()	{ qgraphicsitem_->hide(); }


void uiGraphicsItem::setAcceptHoverEvents( bool yn )
{
    qgraphicsitem_->setAcceptHoverEvents( yn );
}


bool uiGraphicsItem::isHoverEventsAccepted() const
{
    return qgraphicsitem_->acceptHoverEvents();
}


bool uiGraphicsItem::isMovable() const
{ return qgraphicsitem_->flags().testFlag( QGraphicsItem::ItemIsMovable ); }


void uiGraphicsItem::setMovable( bool yn )
{ qgraphicsitem_->setFlag( QGraphicsItem::ItemIsMovable, yn ); }


bool uiGraphicsItem::isVisible() const
{ return qgraphicsitem_->isVisible(); }


void uiGraphicsItem::setVisible( bool yn )
{ qgraphicsitem_->setVisible( yn ); }


Geom::Point2D<float> uiGraphicsItem::getPos() const
{
    return Geom::Point2D<float>( mCast(float,qgraphicsitem_->pos().x()),
				 mCast(float,qgraphicsitem_->pos().y()) );
}


uiRect uiGraphicsItem::boundingRect() const
{
    QRectF qr( qgraphicsitem_->sceneBoundingRect() );
    return uiRect( mNINT32(qr.left()), mNINT32(qr.top()),
	    	   mNINT32(qr.right()), mNINT32(qr.bottom()) );
}

void uiGraphicsItem::setPos( float x, float y )
{ stPos( x, y ); }


void uiGraphicsItem::setPos( const uiPoint& p )
{ stPos( p.x, p.y ); }


void uiGraphicsItem::setPos( const uiWorldPoint& p )
{ stPos( p.x, p.y ); }


void uiGraphicsItem::setPos( const Geom::Point2D<float>& p )
{ stPos(p.x, p.y); }



void uiGraphicsItem::stPos( float x, float y )
{
    translation_.x = x;
    translation_.y = y;

    updateTransform();
}


void uiGraphicsItem::moveBy( float x, float y )
{
    translation_.x += x;
    translation_.y += y;
    updateTransform();
}


float uiGraphicsItem::getRotation()
{
    return qgraphicsitem_->rotation();
}


void uiGraphicsItem::setRotation( float angle )
{
    angle_ = angle;
    updateTransform();
}


void uiGraphicsItem::setScale( float sx, float sy )
{
    scale_.x = sx;
    scale_.y = sy;

    updateTransform();
}


void uiGraphicsItem::updateTransform()
{
    QTransform transform;
    transform.translate( translation_.x, translation_.y );
    transform.scale( scale_.x, scale_.y );
    transform.rotate( angle_ );

    qgraphicsitem_->setTransform( transform );
    qgraphicsitem_->update();
}

/*
 * void uiGraphicsItem::scaleAroundXY( float sx, float sy, int x, int y )
{
    qgraphicsitem_->setTransform( QTransform().translate(x,y)
	   			  .scale(sx,sy).translate(-x,-y) );
}
*/

void uiGraphicsItem::setZValue( int zval )
{ qgraphicsitem_->setZValue( zval ); }


int uiGraphicsItem::nrChildren() const
{
    QList<QGraphicsItem*> qlist = qgraphicsitem_->childItems();
    return qlist.size();
}


uiPoint uiGraphicsItem::transformToScenePos( const uiPoint& pt ) const
{
    QPointF qpt = qgraphicsitem_->mapToScene( pt.x, pt.y );
    return uiPoint( mNINT32(qpt.x()),  mNINT32( qpt.y()) );
}


void uiGraphicsItem::setItemIgnoresTransformations( bool yn )
{ qgraphicsitem_->setFlag( QGraphicsItem::ItemIgnoresTransformations, yn ); }


void uiGraphicsItem::setPenColor( const Color& col, bool usetransparency )
{
    mDynamicCastGet(QAbstractGraphicsShapeItem*,agsitm,qgraphicsitem_)
    if ( !agsitm ) return;

    QColor color = QColor(QRgb(col.rgb()));
    if ( usetransparency ) color.setAlpha( 255-col.t() );

    QPen qpen( color );
    qpen.setCosmetic( true );
    agsitm->setPen( qpen );
}


bool uiGraphicsItem::isSelectable() const
{
    return qgraphicsitem_->flags().testFlag( QGraphicsItem::ItemIsSelectable );
}


void uiGraphicsItem::setSelected( bool yn )
{
    selected_ = yn;
}


void uiGraphicsItem::setSelectable( bool yn )
{
    qgraphicsitem_->setFlag( QGraphicsItem::ItemIsSelectable, yn );
}


void uiGraphicsItem::setParent( uiGraphicsItem* item )
{
    qgraphicsitem_->setParentItem( item ? item->qgraphicsitem_ : 0 );
}


void uiGraphicsItem::setPenStyle( const LineStyle& ls, bool usetransparency )
{
    mDynamicCastGet(QAbstractGraphicsShapeItem*,agsitm,qgraphicsitem_)
    if ( !agsitm ) return;

    QColor color = QColor( QRgb(ls.color_.rgb()) );
    if ( usetransparency ) color.setAlpha( 255-ls.color_.t() );

    QBrush qbrush( color );
    QPen qpen( qbrush, ls.width_, (Qt::PenStyle)ls.type_ );
    qpen.setCosmetic( true );
    agsitm->setPen( qpen );
}


void uiGraphicsItem::setFillColor( const Color& col, bool usetransparency )
{
    mDynamicCastGet(QAbstractGraphicsShapeItem*,agsitm,qgraphicsitem_)
    if ( !agsitm ) return;

    QColor color = Qt::transparent;
    if ( col != Color::NoColor() )
    {
	color = QColor( QRgb(col.rgb()) );
	if ( usetransparency ) color.setAlpha( 255-col.t() );
    }

    QBrush qbrush = agsitm->brush();
    if ( qbrush.style() == Qt::NoBrush )
	qbrush.setStyle( Qt::SolidPattern );
    qbrush.setColor( color );
    agsitm->setBrush( qbrush );
}


void uiGraphicsItem::setFillPattern( const FillPattern& inpfp )
{
    mDynamicCastGet(QAbstractGraphicsShapeItem*,agsitm,qgraphicsitem_)
    if ( !agsitm ) return;

    QBrush qbrush = agsitm->brush();
    Qt::BrushStyle qbs = Qt::NoBrush;
    FillPattern fp = inpfp;

    // Beware, this is a duplication of what is in draw.cc
    // Did this to get full freedom to solve extensions and changes in .cc only
    const int cDotsFillPatternType = 1;
    const int cLinesFillPatternType = 2;
    if ( fp.type_ == cDotsFillPatternType )
    {
	if ( fp.opt_ < 0 || fp.opt_ > 7 ) fp.opt_ = 0;
	qbs = (Qt::BrushStyle)(((int)Qt::SolidPattern)+fp.opt_);
    }
    else if ( fp.type_ == cLinesFillPatternType )
    {
	if ( fp.opt_ < 0 || fp.opt_ > 8 ) fp.opt_ = 0;
	qbs = (Qt::BrushStyle)(((int)Qt::HorPattern)+fp.opt_);
    }

    qbrush.setStyle( qbs );
    agsitm->setBrush( qbrush );
}


void uiGraphicsItem::setTransparency( float transparency )
{
    qgraphicsitem_->setOpacity( 1.0f - transparency );
}


void uiGraphicsItem::setCursor( const MouseCursor& cursor )
{
    QCursor qcursor;
    uiCursorManager::fillQCursor( cursor, qcursor );
    qgraphicsitem_->setCursor( qcursor );
}


void uiGraphicsItem::setToolTip( const uiString& tt )
{
    tooltip_ = tt;
    qgraphicsitem_->setToolTip( tt.getQtString() );
}


void uiGraphicsItemSet::setZValue( int zval )
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( (*this)[idx] )
	    (*this)[idx]->setZValue( zval );
    }
}


int uiGraphicsItem::getZValue() const
{
    return mNINT32(qgraphicsitem_->zValue());
}


void uiGraphicsItem::translateText()
{
    qgraphicsitem_->setToolTip( tooltip_.getQtString() );
}


uiGraphicsItem* uiGraphicsItem::findItem( QGraphicsItem* qitm )
{ return qitm == qgraphicsitem_ ? this : 0; }



// +++++ uiGraphicsItemGroup +++++

uiGraphicsItemGroup::uiGraphicsItemGroup( bool owner )
    : uiGraphicsItem(mkQtObj())
    , isvisible_(true)
    , owner_(owner)
{}


void uiGraphicsItemGroup::setScene( uiGraphicsScene* scene )
{
    uiGraphicsItem::setScene( scene );
    for ( int idx=0; idx<items_.size(); idx++ )
	items_[idx]->setScene( scene );

}


uiGraphicsItemGroup::uiGraphicsItemGroup( const ObjectSet<uiGraphicsItem>& grp )
    : uiGraphicsItem(mkQtObj())
    , isvisible_(true)
    , owner_(false)
{
    ObjectSet<uiGraphicsItem>& itms =
				const_cast<ObjectSet<uiGraphicsItem>&>(grp);
    for ( int idx=0; idx<itms.size(); idx++ )
	add( itms[idx] );
}


uiGraphicsItemGroup::~uiGraphicsItemGroup()
{
    removeAll( owner_ );
    deepErase( items2bdel_ );
}


QGraphicsItem* uiGraphicsItemGroup::mkQtObj()
{
    qgraphicsitemgrp_ = new ODGraphicsItemGroup;
    return qgraphicsitemgrp_;
}


void uiGraphicsItemGroup::add( uiGraphicsItem* itm )
{
    if ( !itm ) return;

    if ( !isMainThreadCurrent() )
    {
	scene_->addUpdateToQueue(
		new uiGraphicsSceneChanger(*this,*itm,false) );
    }
    else
    {
	items_ += itm;
	itm->setScene( scene_ );
	itm->setParent( this );
	qgraphicsitemgrp_->addToGroup( itm->qGraphicsItem() );
    }
}


void uiGraphicsItemGroup::remove( uiGraphicsItem* itm, bool withdelete )
{
    if ( !itm ) return;

    items_ -= itm;
    itm->setScene( 0 );
    itm->setParent( 0 );

    QGraphicsItem* qitm = itm->qGraphicsItem();
    qgraphicsitemgrp_->removeFromGroup( qitm );
    if ( withdelete )
    {
	if ( qitm && qitm->scene() )
	    qitm->scene()->removeItem( qitm );
//	delete itm; TODO: This delete leads to crash in Qt 4.4.3
	itm->setVisible( false );
	items2bdel_ += itm;
    }
}


void uiGraphicsItemGroup::removeAll( bool withdelete )
{
    while ( !items_.isEmpty() )
	remove( items_[0], withdelete );
}


void uiGraphicsItemGroup::setVisible( bool yn )
{
    isvisible_ = yn;
    for ( int idx=0; idx<items_.size(); idx++ )
	items_[idx]->setVisible( yn );
}


bool uiGraphicsItemGroup::isVisible() const
{
    return isvisible_;
}


uiRect uiGraphicsItemGroup::boundingRect() const
{
    if ( isEmpty() ) return uiRect();

    uiRect ret( getUiItem(0)->boundingRect() );
    for ( int idx=1; idx<items_.size(); idx++ )
	ret.include( items_[idx]->boundingRect() );
    return ret;
}


void uiGraphicsItemGroup::translateText()
{
    uiGraphicsItem::translateText();
    for ( int idx=0; idx<items_.size(); idx++ )
	items_[idx]->translateText();
}


bool uiGraphicsItemGroup::isPresent( const uiGraphicsItem& itm ) const
{ return items_.isPresent( &itm ); }


uiGraphicsItem* uiGraphicsItemGroup::findItem( QGraphicsItem* qitm )
{
    if ( qitm == qgraphicsitemgrp_ )
	return this;

    for ( int idx=0; idx<items_.size(); idx++ )
    {
	if ( qitm == items_[idx]->qGraphicsItem() )
	    return items_[idx];
    }

    return 0;
}
