#ifndef uisurfaceman_h
#define uisurfaceman_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uisurfaceman.h,v 1.17 2008-05-26 12:16:47 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"
class uiListBox;
class BufferStringSet;


class uiSurfaceMan : public uiObjFileMan
{
public:
			uiSurfaceMan(uiParent*,const char* typ);
			~uiSurfaceMan();

protected:

    uiListBox*		attribfld;

    bool		isCur2D() const;
    bool		isCurFault() const;

    void		copyCB(CallBacker*);
    void		setRelations(CallBacker*);
    void		stratSel(CallBacker*);

    void		removeAttribCB(CallBacker*);
    void		renameAttribCB(CallBacker*);

    void		mkFileInfo();
    void		fillAttribList(const BufferStringSet&);
    double		getFileSize(const char*,int&) const;
};


#endif
