/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          August 2008
________________________________________________________________________

-*/

#include "uimultcomputils.h"
#include "bufstringset.h"
#include "seisioobjinfo.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "survgeom.h"


uiMultCompDlg::uiMultCompDlg( uiParent* p, const BufferStringSet& complist )
	: uiDialog(p,uiDialog::Setup(tr("Multi-Attribute selection"),
                                     mNoDlgTitle, mNoHelpKey) )
	, compfld_(0)
{
    uiString instructions( tr("Workflow :-\n"
	"1) Select multiple attributes and press \"OK\".\n"
	"2) Wait until the attributes are loaded and displayed\n"
	"3) Make sure the attribute tree-item is still selected\n"
	"4) Press the PageUp / PageDown key to scroll through"
	    " the individual attributes") );
    setTitleText( instructions );

    compfld_ = new uiListBox( this, "", OD::ChooseAtLeastOne );
    compfld_->addItems( complist.getUiStringSet() );
    compfld_->doubleClicked.notify( mCB(this,uiMultCompDlg,accept) );
}


void uiMultCompDlg::getCompNrs( TypeSet<int>& selitems ) const
{
    compfld_->getChosen( selitems );
}


const char* uiMultCompDlg::getCompName( int idx ) const
{
    return compfld_->itemText( idx );
}


uiMultCompSel::uiMultCompSel( uiParent* p )
    : uiCompoundParSel(p,tr("Components subselection"))
    , dlg_(0)
{
}


uiMultCompSel::~uiMultCompSel()
{
    if ( dlg_ ) delete dlg_;
}


void uiMultCompSel::setUpList( const DBKey& mid )
{
    compnms_.erase();
    SeisIOObjInfo::getCompNames( mid, compnms_ );
    butPush.notify( mCB(this,uiMultCompSel,doDlg) );
    prepareDlg();
}


void uiMultCompSel::setUpList( const BufferStringSet& bfsset )
{
    compnms_ = bfsset;
    butPush.notify( mCB(this,uiMultCompSel,doDlg) );
    prepareDlg();
}


void uiMultCompSel::prepareDlg()
{
    if ( dlg_ )
    {
	dlg_->outlistfld_->setEmpty();
	dlg_->outlistfld_->addItems( compnms_.getUiStringSet() );
    }
    else
	dlg_ = new MCompDlg( this, compnms_ );
}


void uiMultCompSel::doDlg( CallBacker* )
{
    if ( !dlg_ ) return;
    dlg_->selChg(0);
    dlg_->go();
}


uiString uiMultCompSel::getSummary() const
{
    uiString ret;
    if ( !allowChoice() || !dlg_ || dlg_->useallfld_->getBoolValue()
	|| dlg_->outlistfld_->nrChosen() == compnms_.size() )
	ret = tr( "-- All components --" );
    else
    {
	uiStringSet selnms;
	for ( int idx=0; idx<compnms_.size(); idx++ )
	    if ( dlg_->outlistfld_->isChosen( idx) )
		selnms.add( toUiString(compnms_.get(idx)) );
	ret = selnms.createOptionString();
    }

    return ret;
}


uiMultCompSel::MCompDlg::MCompDlg( uiParent* p, const BufferStringSet& names )
    : uiDialog( p, uiDialog::Setup(tr("Components selection dialog"),
				   uiString::empty(),mNoHelpKey) )
{
    useallfld_ = new uiGenInput( this, tr("Components to use:"),
				 BoolInpSpec( true, uiStrings::sAll(),
                                 tr("Subselection") ) );
    useallfld_->valuechanged.notify( mCB(this,uiMultCompSel::MCompDlg,selChg));

    uiListBox::Setup su( OD::ChooseAtLeastOne, tr("Available components"),
			 uiListBox::AboveMid );
    outlistfld_ = new uiListBox( this, su );
    outlistfld_->addItems( names.getUiStringSet() );
    outlistfld_->attach( ensureBelow, useallfld_ );
}


void uiMultCompSel::MCompDlg::selChg( CallBacker* )
{
    outlistfld_->display( !useallfld_->getBoolValue() );
}
