#ifndef SoMultiTexture2_h
#define SoMultiTexture2_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		Dec 2005
 RCS:		$Id: SoMultiTexture2.h,v 1.1 2005-12-16 17:57:43 cvskris Exp $
________________________________________________________________________


-*/

#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFImage.h>
#include <Inventor/fields/SoMFEnum.h>
#include <Inventor/fields/SoMFShort.h>


#include "SoMFImage.h"


class SoFieldSensor;
class SoSensor;
class SbMutex;
class SoGLImage;

/*! SoMultiTexture2 combines one or more textures of the same size into one.
All textures are put in a sequence in SoMultiTexture2::image, and the sequence
is processed from the start to the end to create one texture. */

class SoMultiTexture2 : public SoNode
{
    SO_NODE_HEADER( SoMultiTexture2 );
public:

    static void		initClass();
    			SoMultiTexture2();

    enum Operator	{ BLEND, ADD, REPLACE };
   			/*!\enum Operator
			 	 Specifies how a texture should interact
			         with the previous texture(s).
			   \var  Operator BLEND
			   	 The current texture is mixed with the
				 previous texture(s) depending on the
				 value of the opacity component.
			   \var  Operator ADD
			   	 The average between the previous texture(s) and
				 the current texture is taken.
			   \var  Operator REPLACE
			   	 The old texture(s) is replaced by the current
				 one.  */
    enum Component	{ RED=1, GREEN=2, BLUE=4, OPACITY=8 };
    			/*!\enum Component
			 	 Specifies which components that are affected
				 by the current texture. */

    SoMFImage		image;
    			/*!< The images themselves. */
    SoMFShort		numcolor;
    			/*!< Number of colors in colortable for each image.
			     If nonzero, the corresponding image must be 1 byte
			     per pixel and map to the colortable.  */
    SoSFImage		colors;
    			/*!< Colortables for all images. The colortable
			     for image N starts at index=numcolor[0] +
			     numcolor[1] + numcolor[2] + ... +
			     numcolor[N-2] + numcolor[N-1] . */
			     
    SoMFEnum		operation;
    			/*!< Operation for each image. */
    SoMFShort		component;
    			/*!< Component for each image. */


    SoSFColor		blendColor;
    			/*!<See SoTexture2 for documentation. */

    SoSFEnum		wrapS;	/*!<See SoTexture2 for documentation. */
    SoSFEnum		wrapT;	/*!<See SoTexture2 for documentation. */
    SoSFEnum		model;	/*!<See SoTexture2 for documentation. */

    void		GLRender( SoGLRenderAction* );
    void		callback( SoCallbackAction* );
    void		rayPick( SoRayPickAction* );

protected:
    void		doAction( SoAction* );
    			~SoMultiTexture2();
    const unsigned char* createImage( SbVec2s&, int & ) const;
    static void		imageChangeCB( void*, SoSensor* );
    static bool		findTransperancy( const unsigned char* colors, int ncol,
	    				  int nc, const unsigned char* idxs=0,
					  int nidx=0 );
    			/*!<Checks if any of the colors in \a colors has
			    a transparency. If idxs and nidx are set,
			    only the colors that are indexed by idxs are
			    checked. */

    SoFieldSensor*	imagesensor;
    SoFieldSensor*	numcolorsensor;
    SoFieldSensor*	colorssensor;
    SoFieldSensor*	operationsensor;
    SoFieldSensor*	componentsensor;

    SbMutex*		glimagemutex;
    bool		glimagevalid;
    SoGLImage*		glimage;
    const unsigned char* imagedata;
    SbVec2s		imagesize;
    int			imagenc;
};



#endif
    
