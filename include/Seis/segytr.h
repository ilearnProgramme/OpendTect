#ifndef segytrctr_h
#define segytrctr_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		2-4-1996
 RCS:		$Id: segytr.h,v 1.21 2008-09-11 13:54:56 cvsbert Exp $
________________________________________________________________________

Translators for SEGY files traces.

-*/

#include <segylike.h>
class LinScaler;
class SegyTxtHeader;
class SegyBinHeader;
class SegyTraceheader;


class SEGYSeisTrcTranslator : public SegylikeSeisTrcTranslator
{			      isTranslator(SEGY,SeisTrc)
public:

			SEGYSeisTrcTranslator(const char*,const char*);
			~SEGYSeisTrcTranslator();

    virtual int		dataBytes() const;

    void		toSupported(DataCharacteristics&) const;
    void		usePar(const IOPar&);

    virtual const char*	defExtension() const		{ return "sgy"; }

    static const char*	sNumberFormat;
    static const char*	sExternalNrSamples;
    static const char*	sExternalCoordScaling;
    static const char*	sExternalTimeShift;
    static const char*	sExternalSampleRate;
    static const char*	sUseOffset;
    static const char*	sForceRev0;

    bool		isRev1() const;
    int			numbfmt;

    const SegyTxtHeader&	txtHeader() const	{ return txthead; }
    const SegyBinHeader&	binHeader() const	{ return binhead; }
    const SegyTraceheader&	trcHeader() const	{ return trchead; }

protected:

    SegyTxtHeader&	txthead;
    SegyBinHeader&	binhead;
    SegyTraceheader&	trchead;
    int			itrc;
    short		binhead_ns;
    float		binhead_dpos;
    LinScaler*		trcscale;
    const LinScaler*	curtrcscale;
    int			ext_nr_samples;
    float		ext_coord_scaling;
    float		ext_time_shift;
    float		ext_sample_rate;
    bool		force_rev0;

    bool		readTapeHeader();
    void		updateCDFromBuf();
    int			nrSamplesRead() const;
    void		interpretBuf(SeisTrcInfo&);
    bool		writeTapeHeader();
    void		fillHeaderBuf(const SeisTrc&);
    void		toPreSelected(DataCharacteristics&) const;
    const LinScaler*	getTraceScaler() const		{ return curtrcscale; }
    virtual void	toPreferred(DataCharacteristics&) const;

    DataCharacteristics	getDataChar(int) const;
    int			nrFormatFor(const DataCharacteristics&) const;

};


#endif
