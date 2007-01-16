/* AbiWord
 * Copyright (C) 2002-2005 Hubert Figuiere
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
/* $Id$ */



#import <Cocoa/Cocoa.h>
#include "ut_vector.h"

#include "xap_CocoaDialog_Utilities.h"

/** set the label of a NSControl
	\param control the control AppKit object
	\param label UT_UTF8String to set. It is a well formed XP label
	that will be converted.
	\note if the control object you pass is of an unknown type, the function will
	NSLog something on the console. Feel g_free to update the function if you want to
	handle that kind of object.
*/
void SetNSControlLabel (id control, const UT_UTF8String &label)
{
 	char buf [1024];
	NSString*	str;

	_convertLabelToMac(buf, sizeof (buf), label);

	str = [[NSString alloc] initWithUTF8String:buf];

	if ([control isKindOfClass:[NSButton class]] 
		|| [control isKindOfClass:[NSBox class]]
		|| [control isKindOfClass:[NSCell class]]) {
		[control setTitle:str];
	}
	else if ([control isKindOfClass:[NSTabViewItem class]]) {
		[control setLabel:str];
	}
	else if ([control isKindOfClass:[NSTextField class]]) {
		[control setStringValue:str];
	}
	else if ([control isKindOfClass:[NSWindow class]]) {
		[control setTitle:str];
	}
	else {
		NSLog(@"Unknown control type to localize: %@", [control class]);
	}
	[str release];
}


/*!
	Localize a control with a string from a string set
	
	\param control the control AppKit object
	\param pSS the string set
	\param stringID the string ID from the string set
	\seealso SetNSControlLabel
 */
void LocalizeControl (id control, const XAP_StringSet * pSS, XAP_String_Id stringId)
{
	UT_UTF8String label;
	pSS->getValueUTF8(stringId, label);
	SetNSControlLabel(control, label);
}

/*!
	Fetch a string from the string set and return a NSString
	
	\param pSS the string set
	\param stringId the string id
	\return a NSString
 */
NSString* LocalizedString (const XAP_StringSet * pSS, XAP_String_Id stringId)
{
	UT_UTF8String label;
	pSS->getValueUTF8(stringId, label);
	return [NSString stringWithUTF8String:label.utf8_str()];
}

/*!
	Append a menu item to the popup button
	
	\param menu the popup button
	\param pSS the string set
	\param stringId the string id
 */
void AppendLocalizedMenuItem (NSPopUpButton * menu, const XAP_StringSet * pSS, XAP_String_Id stringId, int tag)
{
	NSString * str = LocalizedString(pSS, stringId);

	[menu addItemWithTitle:str];

	[[menu lastItem] setTag:tag];
}

/*!
	Fill a NSPopUpButton with an UT_Vector full of char*

	\desc The popup button gets emptied first.
 */
void FillPopupWithCStrVector(NSPopUpButton* menu, const UT_GenericVector<const char*>& vec)
{
	int i;
	int count = vec.getItemCount();
	
	[menu removeAllItems];
	for (i = 0; i < count; i++) {
		[menu addItemWithTitle:[NSString stringWithUTF8String:vec[i]]];
	}
}


/*!
	Strip the '&' from the label
	
	\param buf the result buffer
	\param bufSize the allocated size for buf
	\param label the label to convert as an UT_String
 */
void _convertLabelToMac (char * buf, size_t bufSize, const UT_UTF8String& label)
{
	UT_ASSERT(buf);
	UT_ASSERT(label.length() < bufSize);

	strcpy (buf, label.utf8_str());

	char * src, *dst;
	src = dst = buf;
	while (*src)
	{
		*dst = *src;
		src++;
		if (*dst != '&') {
			dst++;
		}
	}
	*dst = 0;
}
