/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          March 2005
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiemobjeditor.cc,v 1.2 2008-11-25 15:35:25 cvsbert Exp $";

#include "uiemobjeditor.h"

#include "emeditor.h"

namespace MPE
{

uiEMObjectEditor::uiEMObjectEditor( uiParent* p, MPE::ObjectEditor* he )
    : uiEMEditor( p )
    , editor( he )
    , snapmenuitem( "Snap after edit" )
{}


void uiEMObjectEditor::createNodeMenus(CallBacker* cb)
{
    if ( node.objectID()==-1 ) return;
    mDynamicCastGet( MenuHandler*, menu, cb );

    mAddMenuItem( menu, &snapmenuitem, editor->canSnapAfterEdit(node), 
	   	  editor->getSnapAfterEdit() );
}


void uiEMObjectEditor::handleNodeMenus(CallBacker* cb)
{
    
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( MenuHandler*, menu, caller );
    if ( mnuid==-1 || menu->isHandled() )
	return;

    if ( mnuid==snapmenuitem.id )
    {
	menu->setIsHandled(true);
	editor->setSnapAfterEdit(!editor->getSnapAfterEdit());
    }
}


};
