#ifndef uisegyscandlg_h
#define uisegyscandlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2008
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uisegyreaddlg.h"
namespace SEGY { class Scanner; class FileIndexer; }
class uiSeisSel;
class uiSeis2DLineSel;
class uiBatchJobDispatcherSel;


/*!\brief Dialog to import SEG-Y files after basic setup. */

mExpClass(uiSeis) uiSEGYScanDlg : public uiSEGYReadDlg
{
public :

			uiSEGYScanDlg(uiParent*,const Setup&,IOPar&,
				      bool forsurvsetup);
			~uiSEGYScanDlg();

    SEGY::Scanner*	getScanner();	//!< becomes yours
    static void		presentReport(uiParent*,const SEGY::Scanner&,
				      const char* reportfilenm=0);

    virtual MultiID	outputID() const;

protected:

    SEGY::FileIndexer*	indexer_;
    SEGY::Scanner*	scanner_;
    bool		forsurvsetup_;

    uiSeisSel*		outfld_;
    uiBatchJobDispatcherSel* batchfld_;
    uiSeis2DLineSel*	lnmfld_;

    virtual bool	doWork(const IOObj&);

};


#endif
