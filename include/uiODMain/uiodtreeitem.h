#ifndef uiodtreeitem_h
#define uiodtreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: uiodtreeitem.h,v 1.5 2004-06-23 15:19:21 nanne Exp $
________________________________________________________________________


-*/

#include "uitreeitemmanager.h"
class uiParent;
class uiSoViewer;
class uiListView;
class uiPopUpMenu;
class uiODApplMgr;
class uiListViewItem;

class uiODTreeItem : public uiTreeItem
{
public:
    			uiODTreeItem(const char*);

protected:

    uiODApplMgr*	applMgr();
    uiSoViewer*		viewer();
    int			sceneID() const;

};


class uiODTreeTop : public uiTreeTopItem
{
public:
			uiODTreeTop(uiSoViewer*,uiListView*,
				    uiODApplMgr*,uiTreeCreaterSet*);
			~uiODTreeTop();

    static const char*	sceneidkey;
    static const char*	viewerptr;
    static const char*	applmgrstr;
    static const char*	scenestr;

    int			sceneID() const;
    bool		select(int selkey);

protected:

    void		addCreaterCB(CallBacker*);
    void		removeCreaterCB(CallBacker*);

    virtual const char*	parentType() const { return 0; } 
    uiODApplMgr*	applMgr();

    uiTreeCreaterSet*	tcs;

};



class uiODTreeItemCreater : public uiTreeItemCreater
{
public:

    virtual uiTreeItem*	create(int visid) const = 0;

};


#endif

