/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: viscube.cc,v 1.3 2002-02-28 07:51:08 kristofer Exp $";

#include "viscube.h"
#include "geompos.h"

#include "Inventor/nodes/SoCube.h"
#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/nodes/SoTranslation.h"



visBase::Cube::Cube( visBase::Scene& s )
    : VisualObjectImpl( s )
    , cube( new SoCube )
    , position( new SoTranslation )
{
    root->addChild( position );
    root->addChild( cube );
}


void visBase::Cube::setCenterPos( const Geometry::Pos& pos )
{
    position->translation.setValue( pos.x, pos.y, pos.z );
}


Geometry::Pos visBase::Cube::centerPos() const
{
    Geometry::Pos res;
    SbVec3f pos = position->translation.getValue();

    res.x = pos[0];
    res.y = pos[1];
    res.z = pos[2];

    return res;
}


void visBase::Cube::setWidth( const Geometry::Pos& n )
{
    cube->width.setValue(n.x);
    cube->height.setValue(n.y);
    cube->depth.setValue(n.z);
}


Geometry::Pos visBase::Cube::width() const
{
    Geometry::Pos res;
    
    res.x = cube->width.getValue();
    res.y = cube->height.getValue();
    res.z = cube->depth.getValue();

    return res;
}
