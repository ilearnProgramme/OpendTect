/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          July 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uicompparsel.cc,v 1.6 2008-11-25 15:35:26 cvsbert Exp $";

#include "uicompoundparsel.h"
#include "uigeninput.h"
#include "uibutton.h"


uiCompoundParSel::uiCompoundParSel( uiParent* p, const char* seltxt,
				    const char* btxt )
    : uiGroup(p,seltxt)
    , butPush(this)
{
    txtfld_ = new uiGenInput( this, seltxt, "" );
    txtfld_->setReadOnly( true );

    const char* buttxt = btxt ? btxt : "Select";
    selbut_ = new uiPushButton( this, buttxt, false );
    selbut_->setName( BufferString(buttxt," ",seltxt).buf() );
    selbut_->activated.notify( mCB(this,uiCompoundParSel,doSel) );
    selbut_->attach( rightOf, txtfld_ );

    setHAlignObj( txtfld_ );
    setHCentreObj( txtfld_ );

    mainObject()->finaliseDone.notify( mCB(this,uiCompoundParSel,updSummary) );
}


void uiCompoundParSel::doSel( CallBacker* )
{
    butPush.trigger();
    updateSummary();
}


void uiCompoundParSel::updSummary( CallBacker* )
{
    txtfld_->setText( getSummary() );
}


void uiCompoundParSel::setSelText( const char* txt )
{
    txtfld_->setTitleText( txt );
}
