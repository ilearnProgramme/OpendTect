#ifndef uidatapointsetpickdlg_h
#define uidatapointsetpickdlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	H. Payraudeau
 Date:		February 2006
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uidialog.h"
#include "emobject.h"
#include "sets.h"

class uiTable;
class uiToolBar;
class Array2DInterpol;
class DataPointSet;

namespace Pick { class SetMgr; }
namespace visSurvey { class PickSetDisplay; }

mClass(uiHorizonAttrib) uiDataPointSetPickDlg : public uiDialog
{
public:
			uiDataPointSetPickDlg(uiParent*,int sceneid);
    virtual		~uiDataPointSetPickDlg();

protected:
    void		initPickSet();
    void		updateDPS();
    void		updateTable();
    void		updateButtons();
    virtual void	cleanUp();

    void		valChgCB(CallBacker*);
    void		rowClickCB(CallBacker*);
    void		pickModeCB(CallBacker*);
    void		openCB(CallBacker*);
    void		saveCB(CallBacker*);
    void		saveasCB(CallBacker*);
    void		doSave(bool saveas);
    void		pickCB(CallBacker*);
    void		locChgCB(CallBacker*);
    bool		acceptOK(CallBacker*);
    void		winCloseCB(CallBacker*);
    void		objSelCB(CallBacker*);

    int			sceneid_;
    uiTable*		table_;
    uiToolBar*		tb_;
    DataPointSet&	dps_;
    TypeSet<float>	values_;
    visSurvey::PickSetDisplay* psd_;
    Pick::SetMgr&	picksetmgr_;
    int			pickbutid_;
    int			savebutid_;
    bool		changed_;
};


mClass(uiHorizonAttrib) uiEMDataPointSetPickDlg : public uiDataPointSetPickDlg
{
public:
			uiEMDataPointSetPickDlg(uiParent*,int sceneid,
						EM::ObjectID);
			~uiEMDataPointSetPickDlg();

    const DataPointSet& getData() const		{ return emdps_; }
    Notifier<uiEMDataPointSetPickDlg> readyForDisplay;

protected:

    DataPointSet&	emdps_;
    EM::ObjectID	emid_;
    Array2DInterpol*	interpol_;

    int			addSurfaceData();
    int			dataidx_;

    virtual void	cleanUp();
    void		interpolateCB(CallBacker*);
    void		settCB(CallBacker*);

    HorSampling		hrg_;
};

#endif

