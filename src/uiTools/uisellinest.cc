/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          08/08/2000
 RCS:           $Id: uisellinest.cc,v 1.1 2000-11-27 10:20:46 bert Exp $
________________________________________________________________________

-*/

#include "uisellinest.h"
#include "uilabel.h"
#include "uicombobox.h"
#include "uicolor.h"


uiSelLineStyle::uiSelLineStyle( uiObject* p, const LineStyle& l,
				const char* txt )
	: uiGroup( p, "Line style selector", 0 )
	, ls( l )
{
    UserIDSet itms( LineStyle::TypeNames );
    itms.setName( "Line Style" );
    stylesel = new uiComboBox( this, itms );
    stylesel->setCurrentItem( (int)ls.type );
    new uiLabel( this, txt, stylesel );
    colinp = new uiColorInput( this, ls.color );
    colinp->attach( rightOf, stylesel );

    setHAlignObj( stylesel );
    setHCentreObj( stylesel );
}


LineStyle uiSelLineStyle::getStyle() const
{
    LineStyle ret = ls;
    ret.type = (LineStyle::Type)stylesel->currentItem();
    ret.color = colinp->color();
    return ret;
}
