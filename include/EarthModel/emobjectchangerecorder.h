#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		Dec 2016
________________________________________________________________________


-*/

#include "emcommon.h"
#include "uistring.h"
#include "monitorchangerecorder.h"

namespace EM
{

/*!\brief Holds one change in an EM::Object . */

mExpClass(General) EMChangeRecord : public ChangeRecorder::Record
{
public:

    typedef ChangeRecorder::Action	Action;

    virtual bool	apply(Monitorable&,Action) const;

protected:

    virtual bool	doApply(Monitorable&,Action) consti	= 0;
};


mExpClass(General) EMPosChangeRecord : public EMChangeRecord
{
public:

    EM::PosID		posid_;
    Coord		pos_;

    virtual bool	isValid() const		{ return posid_.isValid();}
    static const EMChangeRecord& udf();

protected:

			EMChangeRecord( PosID id, const Coord& pos )
			    : posid_(id), pos_(pos)	{}

    virtual bool	doApply(Object&,bool) const;

};


/*!\brief Keeps track of changes in an EM::Object */

mExpClass(General) EMChangeRecorder : public ::ChangeRecorder
{
public:

			EMChangeRecorder(Object&);
			EMChangeRecorder(const Object&);
			mDeclMonitorableAssignment(EMChangeRecorder);

protected:

    void		handleObjChange(const ChangeData&);

};

} // namespace EM
