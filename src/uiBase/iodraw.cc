/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          08/12/1999
 RCS:           $Id: iodraw.cc,v 1.30 2007-07-25 12:18:09 cvsbert Exp $
________________________________________________________________________

-*/

#include "iodrawtool.h"
#include "iodrawimpl.h"
#include "errh.h"
#include "pixmap.h"
#include "color.h"
#include "uifont.h"
#include "draw.h"

#ifdef USEQT3
# include <qpen.h>
# include <qbrush.h>
# include <qpaintdevicemetrics.h>
# include <qpointarray.h>
# include <qpainter.h>
#else
# include <QPolygon>
# include <QPainter>
# include <QPen>
# include <QBrush>
#endif


ioDrawTool::ioDrawTool( QPaintDevice* pd )
    : qpainter_(0)
    , qpaintermine_(false)
    , qpainterprepared_(false)
    , qpen_(*new QPen())
    , qpaintdev_(*pd)
    , font_(&uiFontList::get())
#ifdef USEQT3
    , qpaintdevmetr_( 0 )
#endif
{
    if ( !pd )
	pErrMsg("Null paint device passed. Crash will follow");

    setLineStyle( LineStyle() );
}


ioDrawTool::~ioDrawTool() 
{ 
    if ( qpaintermine_ )
	delete qpainter_;

#ifdef USEQT3
    if ( qpaintdevmetr_ )
	delete qpaintdevmetr_;
#endif
    
    delete &qpen_;
}


void ioDrawTool::drawText( int x, int y, const char* txt, const Alignment& al )
{
    preparePainter();

    uiPoint bl( x, y );
    const uiSize sz( font_->width(txt), font_->ascent() );
    if ( al.hor_ == Alignment::Middle )
	bl.x -= sz.width() / 2;
    else if ( al.hor_ == Alignment::Stop )
	bl.x -= sz.width();
    if ( al.ver_ == Alignment::Middle )
	bl.y += sz.height() / 2;
    else if ( al.ver_ == Alignment::Start )
	bl.y += sz.height();

    qpainter_->drawText( bl.x, bl.y, QString(txt) );
}


void ioDrawTool::drawText( const uiPoint& p, const char* t, const Alignment& a )
{
    drawText( p.x, p.y, t, a );
}


void ioDrawTool::drawLine( int x1, int y1, int x2, int y2 )
{
    preparePainter();
    qpainter_->drawLine( x1, y1, x2, y2 );
}


void ioDrawTool::drawLine( const uiPoint& p1, const uiPoint& p2 )
{
    drawLine( p1.x, p1.y, p2.x, p2.y );
}


void ioDrawTool::drawLine( const uiPoint& pt, double angle, double len )
{
    uiPoint endpt( pt );
    double delta = len * cos( angle );
    endpt.x += mNINT(delta);
    // In UI, Y is positive downward
    delta = -len * sin( angle );
    endpt.y += mNINT(delta);
    drawLine( pt, endpt );
}


void ioDrawTool::drawLine( const TypeSet<uiPoint>& pts, bool close )
{
    const int nrpoints = pts.size();
    if ( nrpoints < 2 ) return;

#ifdef USEQT3
    QPointArray qarray( nrpoints );
#else
    QPolygon qarray( nrpoints );
#endif

    for ( int idx=0; idx<nrpoints; idx++ )
	qarray.setPoint( (unsigned int)idx, pts[idx].x, pts[idx].y );

    if ( close )
	qpainter_->drawPolygon( qarray );
    else
	qpainter_->drawPolyline( qarray );
}


void ioDrawTool::drawRect ( int x, int y, int w, int h )
{
    preparePainter();
    int pensize = qpainter_->pen().width();
    if ( pensize < 1 )
	{ pErrMsg("Pen size < 1"); pensize = 1; }
    qpainter_->drawRect ( x, y, w - pensize, h - pensize );
}


void ioDrawTool::drawRect( const uiPoint& topLeft, const uiSize& sz )
{
    drawRect( topLeft.x, topLeft.y, sz.hNrPics(), sz.vNrPics() );
}


void ioDrawTool::drawRect( const uiRect& r )
{
    drawRect( r.left(), r.top(), r.hNrPics(), r.vNrPics() );
}


void ioDrawTool::drawEllipse ( int x, int y, int w, int h )
{
    preparePainter();
    qpainter_->drawEllipse ( x, y, w, h );
}


void ioDrawTool::drawEllipse( const uiPoint& topLeft, const uiSize& sz )
{
    drawEllipse( topLeft.x, topLeft.y, sz.hNrPics(), sz.vNrPics());
}


void ioDrawTool::drawEllipse( const uiRect& r )
{
    drawEllipse( r.left(), r.top(), r.hNrPics(), r.vNrPics() );
}


void ioDrawTool::drawHalfCircle( const uiPoint& from, const uiPoint& to,
				 bool left )
{
    // This function is not right, not only because Y is reversed
    const uiPoint relvec( to - from );
    QRectF qr;
    if ( left )
	qr.setCoords( from.x - 0.5 * relvec.y, from.y + 0.5 * relvec.x,
	       	      to.x, to.y );
    else
	qr.setCoords( from.x, from.y,
	       	      to.x + 0.5 * relvec.y, to.y - 0.5 * relvec.x );

    qpainter_->drawArc( qr, 0, 2880 /* 180 degrees * 16 (see Qt manual) */ );
}


void ioDrawTool::drawHalfCircle( const uiPoint& from, double ang, double d,
				 bool left )
{
    uiPoint to( from );
    double delta = d * cos( ang );
    to.x += mNINT( delta );
    // In UI, Y is positive downward
    delta = -d * sin( ang );
    to.y += mNINT( delta );
    drawHalfCircle( from, to, left );
}



Color ioDrawTool::backgroundColor() const
{
    preparePainter();
#ifdef USEQT3
    return Color( qpainter_->backgroundColor().rgb() );
#else
    return Color( qpainter_->background().color().rgb() );
#endif
}


void ioDrawTool::setBackgroundColor( const Color& c )
{
    preparePainter();
#ifdef USEQT3
    qpainter_->setBackgroundColor( QColor( QRgb(c.rgb()) ) );
#else
    QBrush br( qpainter_->background() );
    br.setColor( QColor( QRgb(c.rgb()) ) );
    qpainter_->setBackground( br );
#endif
}


void ioDrawTool::clear( const uiRect* rect, const Color* col )
{
    preparePainter();
#ifndef USEQT3
    qpainter_->eraseRect( 0, 0, getDevWidth(), getDevHeight() );
    return;
#endif

    uiRect r( 0, 0, getDevWidth(), getDevHeight() );
    if ( rect ) r = *rect;
    Color c( col ? *col : backgroundColor() );
    setPenColor( c ); setFillColor( c );
    drawRect( r );
}


void ioDrawTool::drawBackgroundPixmap( const Color* col )
{
    preparePainter();
    if ( col ) setBackgroundColor( *col );
    qpainter_->setBackgroundMode( Qt::OpaqueMode );
    qpainter_->setBrush( Qt::DiagCrossPattern );
    drawRect( uiRect( 0, 0, getDevWidth(), getDevHeight() ) );
}


void ioDrawTool::drawPixmap (const uiPoint& desttl, const ioPixmap* pm,
			     const uiRect& pmsrcarea )
{
    if ( !pm || !pm->qpixmap() ) 
	{ pErrMsg( "No pixmap" ); return; }

    preparePainter();

    QRect src( QPoint(pmsrcarea.left(),pmsrcarea.top()),
	       QPoint(pmsrcarea.right(),pmsrcarea.bottom()) );
    QPoint dest( desttl.x, desttl.y );
    qpainter_->drawPixmap( dest, *pm->qpixmap(), src );
}


void ioDrawTool::drawPixmap( int left, int top, ioPixmap* pm,
			     int sleft, int stop, int sright, int sbottom )
{
    drawPixmap( uiPoint(left,top), pm, uiRect(sleft,stop,sright,sbottom));
}


int ioDrawTool::getDevHeight() const
{
#ifdef USEQT3
    if ( !qpaintdevmetr_ )
	const_cast<ioDrawTool*>(this)->qpaintdevmetr_
			    = new QPaintDeviceMetrics( &qpaintdev_ );
    return qpaintdevmetr_->height();
#else
    return qpaintdev_.height();
#endif
}


int ioDrawTool::getDevWidth() const
{
#ifdef USEQT3
    if ( !qpaintdevmetr_ )
	const_cast<ioDrawTool*>(this)->qpaintdevmetr_
			    = new QPaintDeviceMetrics( &qpaintdev_ );
    return qpaintdevmetr_->width();
#else
    return qpaintdev_.width(); 
#endif
}


void ioDrawTool::drawMarker( const uiPoint& pt, const MarkerStyle2D& mstyle,
       			     float angle, int side )
{
    setPenColor( mstyle.color_ ); setFillColor( mstyle.color_ );
    if ( side != 0 )
	pErrMsg( "TODO: implement single-sided markers" );
    if ( !mIsZero(angle,1e-3) )
	pErrMsg( "TODO: implement tilted markers" );

    switch ( mstyle.type_ )
    {
    case MarkerStyle2D::Square:
	drawRect( pt.x-mstyle.size_, pt.y-mstyle.size_,
		  2 * mstyle.size_, 2 * mstyle.size_ );
    break;
    case MarkerStyle2D::Circle:
	drawCircle( pt.x - mstyle.size_, pt.y - mstyle.size_, mstyle.size_ );
    break;
    case MarkerStyle2D::Cross:
	drawLine( pt.x-mstyle.size_, pt.y-mstyle.size_,
		  pt.x+mstyle.size_, pt.y+mstyle.size_ );
	drawLine( pt.x-mstyle.size_, pt.y+mstyle.size_,
		  pt.x+mstyle.size_, pt.y-mstyle.size_ );
    break;
    }
}


static double getAddedAngle( double ang, float ratiopi )
{
    ang += ratiopi * M_PI;
    while ( ang < -M_PI ) ang += 2 * M_PI;
    while ( ang > M_PI ) ang -= 2 * M_PI;
    return ang;
}


static void drawArrowHead( ioDrawTool& dt,
			   ArrowStyle::HeadType ht, ArrowStyle::HandedNess hn,
			   int sz,
			   const Geom::Point2D<int>& pos,
			   const Geom::Point2D<int>& comingfrom )
{
    static const float headangfac = .8; // bigger => lines closer to main line

    // In UI, Y is positive downward
    const Geom::Point2D<int> relvec( pos.x - comingfrom.x,
	    			     comingfrom.y - pos.y );
    const double ang( atan2(relvec.y,relvec.x) );

    if ( hn == ArrowStyle::TwoHanded )
    {
	switch ( ht )
	{
	case ArrowStyle::Triangle:
	    pFreeFnErrMsg( "Triangle impl as Line", "drawArrowHead" );
	case ArrowStyle::Line:
	    dt.drawLine( pos, getAddedAngle(ang,headangfac), sz );
	    dt.drawLine( pos, getAddedAngle(ang,-headangfac), sz );
	break;
	default:
	    pFreeFnErrMsg( "Needs impl", "drawArrowHead" );
	break;
	}
    }
    else
    {
	const bool isleft = hn == ArrowStyle::LeftHanded;

	switch ( ht )
	{
	case ArrowStyle::Circle:
	    dt.drawHalfCircle( pos, ang+M_PI, sz, isleft );
	break;
	case ArrowStyle::Triangle:
	    pFreeFnErrMsg( "Triangle impl as Line", "drawArrowHead" );
	case ArrowStyle::Line:
	    dt.drawLine( pos, getAddedAngle(ang,headangfac*(isleft?1:-1)), sz );
	break;
	default:
	    pFreeFnErrMsg( "Needs impl", "drawArrowHead" );
	break;
	}
    }

}


void ioDrawTool::drawArrow( const uiPoint& tail, const uiPoint& head,
			    const ArrowStyle& as )
{
    setLineStyle( as.linestyle_ );
    float fhtsz = as.headsz_;
    if ( !as.fixedheadsz_ )
	fhtsz = .01 * as.headsz_ * tail.distTo(head);
    const int htsz = mNINT(fhtsz);

    drawLine( tail, head );
    if ( as.hasHead() )
	drawArrowHead( *this, as.headtype_, as.handedness_, htsz, head, tail );
    if ( as.hasTail() )
	drawArrowHead( *this, as.headtype_, as.handedness_, htsz, tail, head );
}


void ioDrawTool::setLineStyle( const LineStyle& ls )
{
    qpen_.setStyle( (Qt::PenStyle)ls.type_ );
    qpen_.setColor( QColor( QRgb( ls.color_.rgb() )));
    qpen_.setWidth( ls.width_ );

    if ( qpainter_ )
	qpainter_->setPen( qpen_ ); 
}


void ioDrawTool::setPenColor( const Color& colr )
{
    qpen_.setColor( QColor( QRgb(colr.rgb()) ) );
    if ( qpainter_ )
	qpainter_->setPen( qpen_ ); 
}


void ioDrawTool::setFillColor( const Color& colr )
{ 
    preparePainter();
    qpainter_->setBrush( QColor( QRgb(colr.rgb()) ) );
}


void ioDrawTool::setPenWidth( unsigned int w )
{
    qpen_.setWidth( w );
    if ( qpainter_ )
	qpainter_->setPen( qpen_ ); 
}


void ioDrawTool::setFont( const uiFont& f )
{
    font_ = &f;
    if ( qpainter_ )
	qpainter_->setFont( font_->qFont() );
}


void ioDrawTool::preparePainter() const
{
    if ( !qpainter_ ) 
    {
	ioDrawTool& self = *const_cast<ioDrawTool*>( this );
	self.qpainter_ = new QPainter( &qpaintdev_ );
	self.qpaintermine_ = true;
	self.qpainterprepared_ = false;
    }

    if ( !qpainterprepared_ )
    {
	ioDrawTool& self = *const_cast<ioDrawTool*>( this );
	self.qpainter_->setPen( qpen_ ); 
	self.qpainter_->setFont( font_->qFont() ); 
	self.qpainterprepared_ = true;
    }
}


void ioDrawTool::dismissPainter()
{
#ifdef USEQT3
    if ( qpainter_ )
	qpainter_->flush();
#endif

    if ( qpaintermine_ )
	delete qpainter_;
    qpainter_ = 0;
    qpaintermine_ = true;
    qpainterprepared_ = false;
}


void ioDrawTool::setActivePainter( QPainter* p )
{
    if ( p == qpainter_ ) return;

    dismissPainter();
    qpainter_ = p;
    qpaintermine_ = false;
}


void ioDrawTool::setRasterXor()
{
    preparePainter();
#ifdef USEQT3
    qpainter_->setRasterOp( Qt::XorROP );
#else 
    qpainter_->setCompositionMode( QPainter::CompositionMode_Xor );
#endif
}


void ioDrawTool::setRasterNorm()
{
    preparePainter();
#ifdef USEQT3
    qpainter_->setRasterOp( Qt::CopyROP );
#else
    qpainter_->setCompositionMode(
			    QPainter::CompositionMode_SourceOver );
#endif
}


void ioDrawAreaImpl::releaseDrawTool()
{
    delete dt_; dt_ = 0;
}


ioDrawTool& ioDrawAreaImpl::drawTool()
{
    if ( !dt_ )
	dt_ = new ioDrawTool( qPaintDevice() );
    return *dt_;
}
