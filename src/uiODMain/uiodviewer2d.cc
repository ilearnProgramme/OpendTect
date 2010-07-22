/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Feb 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiodviewer2d.cc,v 1.37 2010-07-22 05:22:39 cvsumesh Exp $";

#include "uiodviewer2d.h"

#include "uiattribpartserv.h"
#include "uiflatauxdataeditor.h"
#include "uiflatviewdockwin.h"
#include "uiflatviewer.h"
#include "uiflatviewmainwin.h"
#include "uiflatviewslicepos.h"
#include "uiflatviewstdcontrol.h"
#include "uiflatviewpropdlg.h"
#include "uiodmain.h"
#include "uiodviewer2dmgr.h"
#include "uitoolbar.h"
#include "uivispartserv.h"

#include "uilistview.h"
#include "uiodvw2dtreeitem.h"

#include "attribdatacubes.h"
#include "attribdatapack.h"
#include "attribdataholder.h"
#include "attribsel.h"
#include "settings.h"
#include "sorting.h"
#include "survinfo.h"

#include "visvw2ddataman.h"

void initSelSpec( Attrib::SelSpec& as )
{ as.set( 0, Attrib::SelSpec::cNoAttrib(), false, 0 ); }

uiODViewer2D::uiODViewer2D( uiODMain& appl, int visid )
    : appl_(appl)
    , visid_(visid)
    , vdselspec_(*new Attrib::SelSpec)
    , wvaselspec_(*new Attrib::SelSpec)
    , viewwin_(0)
    , slicepos_(0)
    , datamgr_(new Vw2DDataManager)
    , tifs_(0)
    , treetp_(0)
{
    basetxt_ = "2D Viewer - ";
    BufferString info;
    appl.applMgr().visServer()->getObjectInfo( visid, info );
    if ( info.isEmpty() )
	info = appl.applMgr().visServer()->getObjectName( visid );
    basetxt_ += info;

    initSelSpec( vdselspec_ );
    initSelSpec( wvaselspec_ );
}


uiODViewer2D::~uiODViewer2D()
{
    if ( viewstdcontrol_ && viewstdcontrol_->propDialog() )
	viewstdcontrol_->propDialog()->close();

    mDynamicCastGet(uiFlatViewDockWin*,fvdw,viewwin())
    if ( fvdw )
	appl_.removeDockWindow( fvdw );

    deepErase( auxdataeditors_ );

    delete treetp_;
    delete datamgr_;

    delete viewwin();
}


void uiODViewer2D::setUpView( DataPack::ID packid, bool wva )
{
    DataPack* dp = DPM(DataPackMgr::FlatID()).obtain( packid, false );
    mDynamicCastGet(Attrib::Flat3DDataPack*,dp3d,dp)
    mDynamicCastGet(Attrib::Flat2DDataPack*,dp2d,dp)
    mDynamicCastGet(Attrib::Flat2DDHDataPack*,dp2ddh,dp)

    const bool isnew = !viewwin();
    if ( isnew )
    {
	if ( dp3d )
	    tifs_ = ODMainWin()->viewer2DMgr().treeItemFactorySet3D();
	else if ( dp2ddh )
	    tifs_ = ODMainWin()->viewer2DMgr().treeItemFactorySet2D();

	createViewWin( (dp3d && dp3d->isVertical()) ||
		       (dp2d && dp2d->isVertical()) );
    }

    if ( slicepos_ )
	slicepos_->getToolBar()->display( dp3d );

    for ( int ivwr=0; ivwr<viewwin()->nrViewers(); ivwr++ )
    {
	if( dp3d )
	{
	    const CubeSampling& cs = dp3d->cube().cubeSampling();
	    if ( slicepos_ ) slicepos_->setCubeSampling( cs );
	}

	DataPack::ID curpackid = viewwin()->viewer(ivwr).packID( wva );
	viewwin()->viewer(ivwr).removePack( curpackid );
	DPM(DataPackMgr::FlatID()).release( curpackid );

	FlatView::DataDispPars& ddp = 
	    		viewwin()->viewer(ivwr).appearance().ddpars_;
	(wva ? ddp.wva_.show_ : ddp.vd_.show_) = true;
	viewwin()->viewer(ivwr).setPack( wva, packid, false, isnew );
    }
    
    //updating stuff
    if ( treetp_ )
    {
	treetp_->updSelSpec( &wvaselspec_, true );
	treetp_->updSelSpec( &vdselspec_, false );

	if ( dp3d )
	    treetp_->updCubeSamling( dp3d->cube().cubeSampling(), true );
	else if ( dp2ddh )
	    treetp_->updCubeSamling( dp2ddh->dataholder().getCubeSampling(),
		    		     true );
    }
    
    viewwin()->start();
}


void uiODViewer2D::createViewWin( bool isvert )
{    
    bool wantdock = false;
    Settings::common().getYN( "FlatView.Use Dockwin", wantdock );
    uiParent* controlparent = 0;
    if ( !wantdock )
    {
	uiFlatViewMainWin* fvmw = new uiFlatViewMainWin( 0,
		uiFlatViewMainWin::Setup(basetxt_).deleteonclose(true)
	       					  .withhanddrag(true) );
	fvmw->windowClosed.notify( mCB(this,uiODViewer2D,winCloseCB) );

	slicepos_ = new uiSlicePos2DView( fvmw );
	slicepos_->positionChg.notify( mCB(this,uiODViewer2D,posChg) );
	viewwin_ = fvmw;

	createTree( fvmw );
    }
    else
    {
	uiFlatViewDockWin* dwin = new uiFlatViewDockWin( &appl_,
				   uiFlatViewDockWin::Setup(basetxt_) );
	appl_.addDockWindow( *dwin, uiMainWin::Top );
	dwin->setFloating( true );
	viewwin_ = dwin;
	controlparent = &appl_;
    }

    viewwin_->setInitialSize( 600, 400 );
    for ( int ivwr=0; ivwr<viewwin_->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewwin()->viewer( ivwr);
	vwr.appearance().setDarkBG( wantdock );
	vwr.appearance().setGeoDefaults(isvert);
	vwr.appearance().annot_.setAxesAnnot(true);
    }

    uiFlatViewer& mainvwr = viewwin()->viewer();
    viewstdcontrol_ = new uiFlatViewStdControl( mainvwr,
	    uiFlatViewStdControl::Setup(controlparent).helpid("51.0.0")
						      .withedit(true) );
    viewwin_->addControl( viewstdcontrol_ );
    createViewWinEditors();
}


void uiODViewer2D::createTree( uiMainWin* mw )
{
    if ( !mw || !tifs_ ) return;

    uiDockWin* treedoc = new uiDockWin( mw, "Annotation Items" );
    treedoc->setMinimumWidth( 200 );
    uiListView* lv = new uiListView( treedoc, "Annotation Items" );
    treedoc->setObject( lv );
    BufferStringSet labels;
    labels.add( "Annotations" );
    labels.add( "Color" );
    lv->addColumns( labels );
    lv->setFixedColumnWidth( uiODViewer2DMgr::cColorColumn(), 40 );

    treetp_ = new uiODVw2DTreeTop( lv, &appl_.applMgr(), this, tifs_ );
    
    TypeSet<int> idxs;
    TypeSet<int> placeidxs;
    
    for ( int idx=0; idx < tifs_->nrFactories(); idx++ )
    {
	SurveyInfo::Pol2D pol2d = (SurveyInfo::Pol2D)tifs_->getPol2D( idx );
	if ( SI().survDataType() == SurveyInfo::Both2DAnd3D
	     || pol2d == SurveyInfo::Both2DAnd3D
	     || pol2d == SI().survDataType() )
	{
	    idxs += idx;
	    placeidxs += tifs_->getPlacementIdx(idx);
	}
    }

    sort_coupled( placeidxs.arr(), idxs.arr(), idxs.size() );
    
    for ( int iidx=0; iidx<idxs.size(); iidx++ )
    {
	const int fidx = idxs[iidx];
	treetp_->addChild( tifs_->getFactory(iidx)->create(), true );
    }

    lv->display( true );
    mw->addDockWindow( *treedoc, uiMainWin::Left );
    treedoc->display( true );
}


void uiODViewer2D::createViewWinEditors()
{   
    for ( int ivwr=0; ivwr<viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewwin()->viewer( ivwr);
	uiFlatViewAuxDataEditor* adeditor = new uiFlatViewAuxDataEditor( vwr );
	adeditor->setSelActive( false );
	auxdataeditors_ += adeditor;
    }
} 


void uiODViewer2D::winCloseCB( CallBacker* cb )
{
    for ( int ivwr=0; ivwr<viewwin()->nrViewers(); ivwr++ )
    {
	DataPack::ID packid = viewwin()->viewer( ivwr ).packID( true );
	DPM(DataPackMgr::FlatID()).release( packid );
	packid = viewwin()->viewer( ivwr ).packID( false );
	DPM(DataPackMgr::FlatID()).release( packid );
    }
    
    deepErase( auxdataeditors_ );

    delete treetp_; treetp_ = 0;
    datamgr_->removeAll();

    mDynamicCastGet(uiMainWin*,mw,cb)
    if ( mw ) mw->windowClosed.remove( mCB(this,uiODViewer2D,winCloseCB) );
    if ( slicepos_ )
	slicepos_->positionChg.remove( mCB(this,uiODViewer2D,posChg) );

    if ( viewstdcontrol_ && viewstdcontrol_->propDialog() )
	viewstdcontrol_->propDialog()->close();

    viewstdcontrol_ = 0;
    viewwin_ = 0;
}


void uiODViewer2D::setSelSpec( const Attrib::SelSpec* as, bool wva )
{
    if ( as )
	(wva ? wvaselspec_ : vdselspec_) = *as;
    else
	initSelSpec( wva ? wvaselspec_ : vdselspec_ );
}


void uiODViewer2D::posChg( CallBacker* )
{
    if ( !slicepos_ ) return;

    CubeSampling cs = slicepos_->getCubeSampling();
    uiAttribPartServer* attrserv = appl_.applMgr().attrServer();

    if ( vdselspec_.id().asInt() > -1 )
    {
	attrserv->setTargetSelSpec( vdselspec_ );
	DataPack::ID dpid = attrserv->createOutput( cs, DataPack::cNoID() );
	setUpView( dpid, false );
    }

    if ( wvaselspec_.id().asInt() > -1 )
    {
	attrserv->setTargetSelSpec( wvaselspec_ );
	DataPack::ID dpid = attrserv->createOutput( cs, DataPack::cNoID() );
	setUpView( dpid, true );
    }
}
