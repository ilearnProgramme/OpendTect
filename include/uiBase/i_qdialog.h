#ifndef i_qdialog_h
#define i_qdialog_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: i_qdialog.h,v 1.1 2000-11-27 10:19:26 bert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "i_qobjwrap.h"

#include <qobject.h>
#include <qwidget.h>
#include <qdialog.h> 


mTemplTypeDefT( i_QObjWrapper, QDialog, ii_QDialog )

class i_QDialog : public ii_QDialog
{
//    Q_OBJECT
public:
                        i_QDialog(uiDialog& client, uiObject* parnt, 
                                  const char* nm, bool modal )
                        : ii_QDialog( client, parnt, nm, modal )
                        , mClient( &client ) {}

//protected slots:
protected:
    virtual void done( int i ) { if( mClient.doneOK(i)) ii_QDialog::done(i); }
    virtual void accept(){ if( mClient.acceptOK()) ii_QDialog::accept(); }
    virtual void reject(){ if( mClient.rejectOK()) ii_QDialog::reject(); }

protected:
    uiDialog*		mClient;
};

//! Helper class for uiDialog to relay Qt's 'activated' messages to uiMenuItem.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/
class i_dialogMessenger : public QObject 
{
    Q_OBJECT
    friend class	uiDialog;

protected:
			i_dialogMessenger( QDialog* receiver,
					   uiDialog*  sender  )
			: _sender( sender )
			, _receiver( receiver )
			{ 
			    connect( this    , SIGNAL( accept() ),
				     receiver,   SLOT( accept() ) );
			    connect( this    , SIGNAL( reject() ),
				     receiver,   SLOT( reject() ) );
			}

    virtual		~i_dialogMessenger() {}
  
signals:

/*!
    \brief signals to be relayed to QDialog. Triggerd by callback functions
    \sa QDialog::accept()
*/
    void 		accept();
    void 		reject();

private:

    QDialog*		_receiver;
    uiDialog*		_sender;

};

#endif
