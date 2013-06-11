/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uistratseisevent.h"
#include "uistratlvlsel.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "stratlevel.h"
#include "survinfo.h"
#include "valseriesevent.h"


uiStratSeisEvent::uiStratSeisEvent( uiParent* p,
				    const uiStratSeisEvent::Setup& su )
    : uiGroup(p,"Strat Seis Event Specification Group")
    , setup_(su)
    , extrwinfld_(0)
    , levelfld_(0)
    , uptolvlfld_(0)
{
    if ( !setup_.fixedlevel_ )
	levelfld_ = new uiStratLevelSel( this, false, "Reference level" );

    BufferStringSet eventnms( VSEvent::TypeNames() );
    eventnms.removeSingle(0);
    evfld_ = new uiGenInput( this, "Snap synthetics to event",
	    			StringListInpSpec(eventnms) );
    evfld_->setWithCheck( true );
    evfld_->checked.notify( mCB(this,uiStratSeisEvent,evSnapCheck) );
    evfld_->setValue( 1 );
    if ( levelfld_ ) evfld_->attach( alignedBelow, levelfld_ );
    evfld_->setElemSzPol( uiObject::Medium );
    setHAlignObj( evfld_ );
    snapoffsfld_ = new uiGenInput( this, "Offset (ms)", FloatInpSpec(0) );
    snapoffsfld_->attach( rightOf, evfld_ );
    snapoffsfld_->setElemSzPol( uiObject::Small );
    snapoffsfld_->setSensitive( false );

    if ( setup_.withextrwin_ )
    {
	const float defstep = SI().zIsTime() ? SI().zStep() * 1000 : 4;
	extrwinfld_ = new uiGenInput( this, BufferString("Extraction window ",
		    SI().getZUnitString()),
	      FloatInpIntervalSpec(StepInterval<float>(0,0,defstep)) );
	extrwinfld_->attach( alignedBelow, evfld_ );
	BufferStringSet lvlnms; Strat::LVLS().getNames( lvlnms );
	if ( !lvlnms.isEmpty() )
	{
	    uptolvlfld_ = new uiGenInput( this, "Stop at",
		    			  StringListInpSpec(lvlnms) );
	    uptolvlfld_->setText( lvlnms.get(lvlnms.size()-1).buf() );
	    uptolvlfld_->setWithCheck( true );
	    uptolvlfld_->setChecked( false );
	    uptolvlfld_->attach( alignedBelow, extrwinfld_ );
	}
    }
}


void uiStratSeisEvent::setLevel( const char* lvlnm )
{
    const Strat::Level* lvl = Strat::LVLS().get( lvlnm );
    if ( levelfld_ )
	levelfld_->setSelected( lvl );
    else if ( lvl )
    {
	setup_.fixedlevel_ = lvl;
	ev_.level_ = lvl;
    }
}


const char* uiStratSeisEvent::levelName() const
{
    return levelfld_ ? levelfld_->getName() : setup_.fixedlevel_->name().buf();
}


void uiStratSeisEvent::evSnapCheck( CallBacker* )
{
    snapoffsfld_->setSensitive( evfld_->isChecked() );
}


#define mErrRet(s) { if ( s && *s ) uiMSG().error(s); return false; }

bool uiStratSeisEvent::getFromScreen()
{
    ev_.level_ = setup_.fixedlevel_;
    if ( levelfld_ )
    {
	ev_.level_ = levelfld_->selected();
	if ( !ev_.level_ )
	    mErrRet("Cannot find selected stratigraphic level")
    }

    ev_.evtype_ = !evfld_->isChecked() ? VSEvent::None
		: (VSEvent::Type)(evfld_->getIntValue()+1);
    if ( ev_.evtype_ != VSEvent::None )
	ev_.offs_ = snapoffsfld_->getfValue() *.001f;

    ev_.downtolevel_ = 0;
    if ( extrwinfld_ )
    {
	ev_.extrwin_ = extrwinfld_->getFStepInterval();
	if ( mIsUdf(ev_.extrwin_.start) || mIsUdf(ev_.extrwin_.stop)
	  || mIsUdf(ev_.extrwin_.step) )
	    mErrRet("Please enter all extraction window parameters")
	ev_.extrwin_.start *= 0.001; ev_.extrwin_.stop *= 0.001;
	ev_.extrwin_.step *= 0.001;
	if ( uptolvlfld_ && uptolvlfld_->isChecked() )
	    ev_.downtolevel_ = Strat::LVLS().get( uptolvlfld_->text() );
    }

    return true;
}


void uiStratSeisEvent::putToScreen()
{
    if ( levelfld_ )
	levelfld_->setSelected( ev_.level_ );
    evfld_->setChecked( ev_.evtype_ != VSEvent::None );
    if ( ev_.evtype_ == VSEvent::None )
	evfld_->setValue( ((int)ev_.evtype_)-1 );
    snapoffsfld_->setValue( ev_.offs_ );
    if ( extrwinfld_ )
    {
	extrwinfld_->setValue( ev_.extrwin_ );
	if ( uptolvlfld_ )
	{
	    uptolvlfld_->setChecked( (bool)ev_.downtolevel_ );
	    if ( ev_.downtolevel_ )
		uptolvlfld_->setText( ev_.downtolevel_->name() );
	}
    }
}
