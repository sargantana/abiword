/* AbiWord
 * Copyright (C) 2000 AbiSource, Inc.
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

#ifndef XAP_WIN32DIALOG_ENCODING_H
#define XAP_WIN32DIALOG_ENCODING_H

#include "xap_Dlg_Encoding.h"
#include "xap_Win32DialogBase.h"

class XAP_Frame;

/*****************************************************************/

class ABI_EXPORT XAP_Win32Dialog_Encoding: public XAP_Win32DialogBase, public XAP_Dialog_Encoding
{
public:
	XAP_Win32Dialog_Encoding(XAP_DialogFactory * pDlgFactory, XAP_Dialog_Id id);
	virtual ~XAP_Win32Dialog_Encoding(void);

	virtual void			runModal(XAP_Frame * pFrame);

	static XAP_Dialog *		static_constructor(XAP_DialogFactory *, XAP_Dialog_Id id);

protected:
	BOOL _onInitDialog(HWND hWnd, WPARAM wParam, LPARAM lParam);
	BOOL _onCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
	BOOL _onDeltaPos(NM_UPDOWN * pnmud);
};

#endif /* XAP_WIN32DIALOG_ENCODING_H */
