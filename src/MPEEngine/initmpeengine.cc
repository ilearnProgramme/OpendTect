/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initmpeengine.cc,v 1.7 2008-11-25 15:35:22 cvsbert Exp $";

#include "initmpeengine.h"
#include "faulteditor.h"
#include "faultstickseteditor.h"
#include "faulttracker.h"
#include "horizoneditor.h"
#include "horizon2dtracker.h"
#include "horizon3dtracker.h"
#include "polygonsurfeditor.h"

void MPEEngine::initStdClasses()
{
    MPE::FaultEditor::initClass();
    MPE::FaultStickSetEditor::initClass();
    //MPE::FaultTracker::initClass();
    MPE::HorizonEditor::initClass();
    MPE::Horizon2DTracker::initClass();
    MPE::Horizon3DTracker::initClass();
    MPE::PolygonBodyEditor::initClass();
}
