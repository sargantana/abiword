/* AbiWord
 * Copyright (C) 2002 Dom Lachowicz, William Lachance and others
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "ut_types.h"
#include "ap_Frame.h"

#include "ap_FrameData.h"
#include "fv_View.h"
#include "xav_View.h"
#include "xad_Document.h"
#include "pd_Document.h"
#include "xap_ViewListener.h"
#include "xap_Scrollbar_ViewListener.h"
#include "ap_TopRuler.h"
#include "ap_LeftRuler.h"
#include "ap_StatusBar.h"
#include "xap_Dlg_Zoom.h"

/*****************************************************************/

#define ENSUREP_C(p)		do { UT_ASSERT(p); if (!p) goto Cleanup; } while (0)

/*****************************************************************/

void AP_Frame::setZoomPercentage(UT_uint32 iZoom)
{
	bool bChanged = (getZoomPercentage() != iZoom);
	XAP_Frame::setZoomPercentage(iZoom);
	if (bChanged) {
		// only redisplay if zoom factor changed
		_showDocument(iZoom);
	}
}

UT_uint32 AP_Frame::getZoomPercentage(void)
{
	return static_cast<AP_FrameData*>(m_pData)->m_pG->getZoomPercentage();
}

AP_Frame::~AP_Frame()
{
}


bool AP_Frame::initFrameData()
{
	UT_ASSERT(!static_cast<AP_FrameData*>(m_pData));

	AP_FrameData* pData = new AP_FrameData(static_cast<XAP_App *>(m_pApp));

	m_pData = static_cast<void*>(pData);
	return (pData ? true : false);
}

void AP_Frame::killFrameData()
{
	AP_FrameData* pData = static_cast<AP_FrameData*>(m_pData);
	DELETEP(pData);
	m_pData = NULL;
}

UT_Error AP_Frame::_loadDocument(const char * szFilename, IEFileType ieft,
				     bool createNew)
{
#ifdef DEBUG
	if (szFilename) {
		UT_DEBUGMSG(("DOM: trying to load %s (%d, %d)\n", szFilename, ieft, createNew));
	}
	else {
		UT_DEBUGMSG(("DOM: trying to load %s (%d, %d)\n", "(NULL)", ieft, createNew));
	}
#endif

	// are we replacing another document?
	if (m_pDoc)
	{
		// yep.  first make sure it's OK to discard it, 
		// TODO: query user if dirty...
	}

	// load a document into the current frame.
	// if no filename, create a new document.
	if(m_pApp->findFrame(this) < 0)
	{
		m_pApp->rememberFrame(this);
	}
	AD_Document * pNewDoc = new PD_Document(getApp());
	UT_ASSERT(pNewDoc);
	
	if (!szFilename || !*szFilename)
	{
		pNewDoc->newDocument();
		m_iUntitled = _getNextUntitledNumber();
		goto ReplaceDocument;
	}
	UT_Error errorCode;
	errorCode = pNewDoc->readFromFile(szFilename, ieft);
	if (!errorCode)
		goto ReplaceDocument;

	if (createNew)
	  {
	    // we have a file name but couldn't load it
	    pNewDoc->newDocument();

	    // here, we want to open a new document if it doesn't exist.
	    // errorCode could also take several other values, indicating
	    // that the document exists but for some reason we could not
	    // open it. in those cases, we do not wish to overwrite the
	    // existing documents, but instead open a new blank document. 
	    // this fixes bug 1668 - DAL

		UT_DEBUGMSG(("Could not open the document - create new istead error code is %d \n", errorCode));
	    if ( UT_IE_FILENOTFOUND == errorCode ||  UT_INVALIDFILENAME == errorCode  )
		{
			UT_DEBUGMSG(("File NOT found!! Create new doc \n"));
			if( UT_IE_FILENOTFOUND == errorCode)
			{
				errorCode = pNewDoc->saveAs(szFilename, ieft);
			}
			else
			{
				errorCode = 0;
			}
			UT_DEBUGMSG(("errocode after save is \n",errorCode));
		}
	  }
	if (!errorCode)
	  goto ReplaceDocument;
	
	UT_DEBUGMSG(("ap_Frame: could not open the file [%s]\n",szFilename));
	UNREFP(pNewDoc);
	return errorCode;

ReplaceDocument:
	getApp()->forgetClones(this);
	UT_DEBUGMSG(("Doing replace document \n"));
	// NOTE: prior document is discarded in _showDocument()
	m_pDoc = pNewDoc;
	return UT_OK;
}
	
UT_Error AP_Frame::_importDocument(const char * szFilename, int ieft,
									  bool markClean)
{
	UT_DEBUGMSG(("DOM: trying to import %s (%d, %d)\n", szFilename, ieft, markClean));

	// are we replacing another document?
	if (m_pDoc)
	{
		// yep.  first make sure it's OK to discard it,
		// TODO: query user if dirty...
	}

	// load a document into the current frame.
	// if no filename, create a new document.

	AD_Document * pNewDoc = new PD_Document(getApp());
	UT_ASSERT(pNewDoc);

	if (!szFilename || !*szFilename)
	{
		pNewDoc->newDocument();
		goto ReplaceDocument;
	}
	UT_Error errorCode;
	errorCode = pNewDoc->importFile(szFilename, ieft, markClean);
	if (!errorCode)
		goto ReplaceDocument;

	UT_DEBUGMSG(("ap_Frame: could not open the file [%s]\n",szFilename));

	UNREFP(pNewDoc);
	return errorCode;

ReplaceDocument:
	getApp()->forgetClones(this);

	m_iUntitled = _getNextUntitledNumber();

	// NOTE: prior document is discarded in _showDocument()
	m_pDoc = pNewDoc;
	return UT_OK;
}

XAP_Frame * AP_Frame::buildFrame(XAP_Frame * pF)
{
	UT_Error error = UT_OK;
	AP_Frame * pClone = static_cast<AP_Frame *>(pF);
	XAP_Frame::tZoomType iZoomType = pF->getZoomType();;
	setZoomType(iZoomType);
	UT_uint32 iZoom = XAP_Frame::getZoomPercentage();
	ENSUREP_C(pClone);
	if (!pClone->initialize())
		goto Cleanup;

	// we remember the view of the parent frame ...
	static_cast<AP_FrameData*>(pClone->m_pData)->m_pRootView = m_pView;
	
	error = pClone->_showDocument(iZoom);
	if (error)
		goto Cleanup;

	pClone->show();
	return static_cast<XAP_Frame *>(pClone);

 Cleanup:
	// clean up anything we created here
	if (pClone)
	{
		static_cast<XAP_App *>(m_pApp)->forgetFrame(pClone);
		delete pClone;
	}
	return NULL;
}

UT_Error AP_Frame::loadDocument(const char * szFilename, int ieft, bool createNew)
{
	bool bUpdateClones;
	UT_Vector vClones;
	XAP_App * pApp = getApp();
	UT_uint32 j = 0;
	if(pApp->findFrame(this) < 0)
	{
			pApp->rememberFrame(this);
	}
	bUpdateClones = (getViewNumber() > 0);
	if (bUpdateClones)
	{
		pApp->getClones(&vClones, this);
	}
	for(j=0; j<vClones.getItemCount();j++)
	{
		XAP_Frame * pFrame = static_cast<XAP_Frame *>(vClones.getNthItem(j));
		if(pApp->findFrame(pFrame) < 0)
		{
			pApp->rememberFrame(pFrame,this);
		}
	}
	UT_Error errorCode;
	errorCode =  _loadDocument(szFilename, static_cast<IEFileType>(ieft), createNew);
	if (errorCode)
	{
		// we could not load the document.
		// we cannot complain to the user here, we don't know
		// if the app is fully up yet.  we force our caller
		// to deal with the problem.
		return errorCode;
	}
	XAP_Frame::tZoomType iZoomType;
	UT_uint32 iZoom = getNewZoom(&iZoomType);
	setZoomType(iZoomType);
	if(pApp->findFrame(this) < 0)
	{
		pApp->rememberFrame(this);
	}
	if (bUpdateClones)
	{
		for (UT_uint32 i = 0; i < vClones.getItemCount(); i++)
		{
			AP_Frame * pFrame = static_cast<AP_Frame *>(vClones.getNthItem(i));
			if(pFrame != this)
			{
				pFrame->_replaceDocument(m_pDoc);
			}
		}
	}

	return _showDocument(iZoom);
}

UT_Error AP_Frame::loadDocument(const char * szFilename, int ieft)
{
  return loadDocument(szFilename, ieft, false);
}

UT_Error AP_Frame::importDocument(const char * szFilename, int ieft, bool markClean)
{
	bool bUpdateClones;
	UT_Vector vClones;
	XAP_App * pApp = getApp();

	bUpdateClones = (getViewNumber() > 0);
	if (bUpdateClones)
	{
		pApp->getClones(&vClones, this);
	}
	UT_Error errorCode;
	errorCode =  _importDocument(szFilename, static_cast<IEFileType>(ieft), markClean);
	if (errorCode)
	{
		return errorCode;
	}

	if (bUpdateClones)
	{
		for (UT_uint32 i = 0; i < vClones.getItemCount(); i++)
		{
			AP_Frame * pFrame = static_cast<AP_Frame *>(vClones.getNthItem(i));
			if(pFrame != this)
			{
				pFrame->_replaceDocument(m_pDoc);
			}
		}
	}
	XAP_Frame::tZoomType iZoomType;
	UT_uint32 iZoom = getNewZoom(&iZoomType);
	setZoomType(iZoomType);
	return _showDocument(iZoom);
}

/*!
 * This method returns the zoomPercentage of the new frame.
 * Logic goes like this.
 * If there is a valid last focussed frame return the zoom and zoom type 
 * for that.
 * Otherwise use the preference value.
 */
UT_uint32 AP_Frame::getNewZoom(XAP_Frame::tZoomType * tZoom)
{
	UT_Vector vecClones;
	XAP_Frame *pF = NULL;
	XAP_App * pApp = getApp();
	UT_ASSERT(pApp);
	XAP_Frame * pLastFrame = pApp->getLastFocussedFrame();
	UT_uint32 iZoom = 100;
	if(pLastFrame == NULL)
	{
		UT_String sZoom;
		pApp->getPrefsValue(XAP_PREF_KEY_ZoomType, sZoom);
		*tZoom = getZoomType();
		if( (UT_stricmp( sZoom.c_str(), "Width" ) == 0 ) || (UT_stricmp( sZoom.c_str(), "Page" ) == 0 ))
		{
			iZoom = 100;
		}
		else
		{
			iZoom =  atoi( sZoom.c_str() );
		}
		return iZoom;
	}
	else
	{
		UT_uint32 nFrames = getViewNumber();

		if(nFrames == 0)
		{
			iZoom = pLastFrame->getZoomPercentage();
			*tZoom = pLastFrame->getZoomType();
			return iZoom;
		}
		getApp()->getClones(&vecClones,this);
		UT_uint32 i =0;
		bool bMatch = false;
		for (i=0; !bMatch && (i< vecClones.getItemCount()); i++)
		{
			XAP_Frame *pF = static_cast<XAP_Frame *>(vecClones.getNthItem(i));
			bMatch = (pF == pLastFrame);
		}
		if(bMatch)
		{
			iZoom = pLastFrame->getZoomPercentage();
			*tZoom = pLastFrame->getZoomType();
			return iZoom;
		}
	}
	iZoom = pF->getZoomPercentage();
	*tZoom = pF->getZoomType();
	return iZoom;
}

UT_Error AP_Frame::_replaceDocument(AD_Document * pDoc)
{
	// NOTE: prior document is discarded in _showDocument()
	m_pDoc = REFP(pDoc);
	XAP_Frame::tZoomType iZoomType;
	UT_uint32 iZoom = getNewZoom(&iZoomType);
	setZoomType(iZoomType);
	return _showDocument(iZoom);
}

UT_Error AP_Frame::_showDocument(UT_uint32 iZoom)
{
	if (!m_pDoc)
	{
		UT_DEBUGMSG(("Can't show a non-existent document\n"));
		return UT_IE_FILENOTFOUND;
	}
	if(isFrameLocked())
	{
		UT_DEBUGMSG(("_showDocument: Nasty race bug, please fix me!! \n"));
		UT_ASSERT(0);
		return  UT_IE_ADDLISTENERERROR;
	}
	setFrameLocked(true);
	if (!static_cast<AP_FrameData*>(m_pData))
	{
		UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
		setFrameLocked(false);
		return UT_IE_IMPORTERROR;
	}

// 	static_cast<XAP_FrameImpl *>(m_pFrameImpl)->setShowDocLocked(true);

	GR_Graphics * pG = NULL;
	FL_DocLayout * pDocLayout = NULL;
	AV_View * pView = NULL;
	AV_ScrollObj * pScrollObj = NULL;
	ap_ViewListener * pViewListener = NULL;
	AD_Document * pOldDoc = NULL;
	ap_Scrollbar_ViewListener * pScrollbarViewListener = NULL;
	AV_ListenerId lid;
	AV_ListenerId lidScrollbarViewListener;

	xxx_UT_DEBUGMSG(("_showDocument: Initial m_pView %x \n",m_pView));

	if(iZoom < XAP_DLG_ZOOM_MINIMUM_ZOOM) 
		iZoom = 100;
	else if (iZoom > XAP_DLG_ZOOM_MAXIMUM_ZOOM) 
		iZoom = 100;
	UT_DEBUGMSG(("!!!!!!!!! _showdOCument: Initial izoom is %d \n",iZoom));

	if (!_createViewGraphics(pG, iZoom))
		goto Cleanup;

	pDocLayout = new FL_DocLayout(static_cast<PD_Document *>(m_pDoc), pG);
	ENSUREP_C(pDocLayout);  

	pView = new FV_View(getApp(), this, pDocLayout);
	ENSUREP_C(pView);
	
	if(getZoomType() == XAP_Frame::z_PAGEWIDTH)
	{
		iZoom = pView->calculateZoomPercentForPageWidth();
		pG->setZoomPercentage(iZoom);
	}
	else if(getZoomType() == XAP_Frame::z_WHOLEPAGE)
	{
		iZoom = pView->calculateZoomPercentForWholePage();
		pG->setZoomPercentage(iZoom);
	}
	UT_DEBUGMSG(("!!!!!!!!! _showdOCument: izoom is %d \n",iZoom));
	XAP_Frame::setZoomPercentage(iZoom);
	_setViewFocus(pView);

	if (!_createScrollBarListeners(pView, pScrollObj, pViewListener, pScrollbarViewListener,
				       lid, lidScrollbarViewListener))
		goto Cleanup;
	if(getFrameMode() ==XAP_NormalFrame)
	{
		_bindToolbars(pView);	
	}
	_replaceView(pG, pDocLayout, pView, pScrollObj, pViewListener, pOldDoc, 
		     pScrollbarViewListener, lid, lidScrollbarViewListener, iZoom);

	setXScrollRange();
	setYScrollRange();

	m_pView->draw();

	if ( static_cast<AP_FrameData*>(m_pData)->m_bShowRuler  ) 
	{
		if ( static_cast<AP_FrameData*>(m_pData)->m_pTopRuler )
			static_cast<AP_FrameData*>(m_pData)->m_pTopRuler->draw(NULL);

		if ( static_cast<AP_FrameData*>(m_pData)->m_pLeftRuler )
			static_cast<AP_FrameData*>(m_pData)->m_pLeftRuler->draw(NULL);
	}
	if(isStatusBarShown())
	{
		if (static_cast<AP_FrameData*>(m_pData)->m_pStatusBar)
			static_cast<AP_FrameData*>(m_pData)->m_pStatusBar->notify(m_pView, AV_CHG_ALL);
	}

	m_pView->notifyListeners(AV_CHG_ALL);
	m_pView->focusChange(AV_FOCUS_HERE);

	//static_cast<XAP_FrameImpl *>(m_pFrameImpl)->setShowDocLocked(false);
	setFrameLocked(false);
	return UT_OK;

Cleanup:
	// clean up anything we created here
	DELETEP(pG);
	DELETEP(pDocLayout);
	DELETEP(pView);
	DELETEP(pViewListener);
	DELETEP(pScrollObj);
	DELETEP(pScrollbarViewListener);

	// change back to prior document
	UNREFP(m_pDoc);
	setFrameLocked(false);
	UT_return_val_if_fail(static_cast<AP_FrameData*>(m_pData)->m_pDocLayout, UT_IE_ADDLISTENERERROR);
	m_pDoc = static_cast<AP_FrameData*>(m_pData)->m_pDocLayout->getDocument();
	//static_cast<XAP_FrameImpl *>(m_pFrameImpl)->setShowDocLocked(false);
	return UT_IE_ADDLISTENERERROR;
}

void AP_Frame::_replaceView(GR_Graphics * pG, FL_DocLayout *pDocLayout,
			    AV_View *pView, AV_ScrollObj * pScrollObj,
			    ap_ViewListener *pViewListener, AD_Document *pOldDoc,
			    ap_Scrollbar_ViewListener *pScrollbarViewListener,
			    AV_ListenerId lid, AV_ListenerId lidScrollbarViewListener,
			    UT_uint32 iZoom)
{
	bool holdsSelection = false, hadView = true;
	PD_DocumentRange range;
	PT_DocPosition inspt = 0;
	FV_View * pOldView = NULL;
	FL_DocLayout * pOldDocLayout  = NULL;

	// we want to remember point/selection when that is appropriate, which is when the new view is a
	// view of the same doc as the old view, or if this frame is being cloned from an existing frame
	
	// these are the view and doc from which this frame is being cloned
	FV_View * pRootView = NULL; // this should really be const, but
								// getDocumentRangeOfCurrentSelection() is not
	const AD_Document * pRootDoc = NULL;
	
	if (m_pView && !m_pView->isSelectionEmpty ())
	{
		holdsSelection = true;
		static_cast<FV_View*>(m_pView)->getDocumentRangeOfCurrentSelection (&range);
	} else if (m_pView)
	{
		inspt = static_cast<FV_View*>(m_pView)->getInsPoint ();
		pOldView = static_cast<FV_View *>(m_pView);
	}
	else if(static_cast<AP_FrameData*>(m_pData)->m_pRootView)
	{
		pRootView = static_cast<FV_View*>(static_cast<AP_FrameData*>(m_pData)->m_pRootView);
		pRootDoc = pRootView->getDocument();

		if (!pRootView->isSelectionEmpty())
		{
			holdsSelection = true;
			pRootView->getDocumentRangeOfCurrentSelection (&range);
		} else if (pRootView)
		{
			inspt = pRootView->getInsPoint ();
		}
		else
			hadView = false;

		// we want to set m_pData->m_pRootView to NULL, since it has fullfilled its function and we
		// do not want any dead pointers hanging around
		static_cast<AP_FrameData*>(m_pData)->m_pRootView = NULL;
	}
	else
		hadView = false;

	// switch to new view, cleaning up previous settings
	if (static_cast<AP_FrameData*>(m_pData)->m_pDocLayout)
	{
		pOldDoc = (static_cast<AP_FrameData*>(m_pData)->m_pDocLayout->getDocument());
		pOldDocLayout = static_cast<AP_FrameData*>(m_pData)->m_pDocLayout;
	}

	REPLACEP(static_cast<AP_FrameData*>(m_pData)->m_pG, pG);
	REPLACEP(static_cast<AP_FrameData*>(m_pData)->m_pDocLayout, pDocLayout);
	
	bool bSameDocument = false; // be cautious ...

	if(!pOldDoc)
	{
		// this is the case when this is a new frame unrelated to anything, or a frame cloned from
		// existing frame
		if(pRootDoc == m_pDoc)
		{
			// we have been cloned from a frame which related to the same document
			bSameDocument = true;
		}
	}
	else if (pOldDoc != m_pDoc)
	{
		UNREFP(pOldDoc);
	}
	else
	{
		// pOldDoc == m_pDoc
		bSameDocument = true;
	}

	REPLACEP(m_pView, pView);
	if(getApp()->getViewSelection() == m_pView)
		getApp()->setViewSelection(NULL);

	REPLACEP(m_pScrollObj, pScrollObj);
	REPLACEP(m_pViewListener, pViewListener);
	m_lid = lid;
	REPLACEP(m_pScrollbarViewListener, pScrollbarViewListener);
	m_lidScrollbarViewListener = lidScrollbarViewListener;

	m_pView->addScrollListener(m_pScrollObj);

	// Associate the new view with the existing TopRuler, LeftRuler.
	// Because of the binding to the actual on-screen widgets we do
	// not destroy and recreate the TopRuler, LeftRuler when we change
	// views, like we do for all the other objects.  We also do not
	// allocate the TopRuler, LeftRuler  here; that is done as the
	// frame is created.
	if ( static_cast<AP_FrameData*>(m_pData)->m_bShowRuler )
	{
		if ( static_cast<AP_FrameData*>(m_pData)->m_pTopRuler )
			static_cast<AP_FrameData*>(m_pData)->m_pTopRuler->setView(pView, iZoom);
		if ( static_cast<AP_FrameData*>(m_pData)->m_pLeftRuler )
			static_cast<AP_FrameData*>(m_pData)->m_pLeftRuler->setView(pView, iZoom);
	}

	if ( static_cast<AP_FrameData*>(m_pData)->m_pStatusBar && (getFrameMode() != XAP_NoMenusWindowLess))
		static_cast<AP_FrameData*>(m_pData)->m_pStatusBar->setView(pView);
	static_cast<FV_View *>(m_pView)->setShowPara(static_cast<AP_FrameData*>(m_pData)->m_bShowPara);

	pView->setInsertMode((static_cast<AP_FrameData*>(m_pData)->m_bInsertMode));
	m_pView->setWindowSize(_getDocumentAreaWidth(), _getDocumentAreaHeight());

	updateTitle();
	XAP_App * pApp = getApp();
	if(pApp->findFrame(this) < 0)
	{
		pApp->rememberFrame(this);
	}

	pDocLayout->fillLayouts();      

	// can only hold selection/point if the two views are for the same doc !!!
	if(bSameDocument)
	{
		FV_View * pFV_View = static_cast<FV_View*>(m_pView);
		if (holdsSelection)
			pFV_View->cmdSelect (range.m_pos1, range.m_pos2);
		else if (hadView)
			pFV_View->moveInsPtTo(inspt);
	}
}
