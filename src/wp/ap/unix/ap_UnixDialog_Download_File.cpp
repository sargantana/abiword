/* AbiWord
 * Copyright (C) 2002 Gabriel Gerhardsson
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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include "ut_string.h"
#include "ut_assert.h"
#include "ut_debugmsg.h"
#include "xap_UnixDialogHelper.h"

#include "xap_App.h"
#include "xap_Frame.h"
#include "xap_UnixApp.h"
#include "xap_UnixFrame.h"

#include "xap_Dialog_Id.h"
#include "ap_UnixDialog_Download_File.h"

#include "gr_UnixGraphics.h"
#include "gr_UnixImage.h"
#include "ut_bytebuf.h"
#include "ut_png.h"
#include "ut_worker.h"


XAP_Dialog * AP_UnixDialog_Download_File::static_constructor(XAP_DialogFactory * pFactory,
													 XAP_Dialog_Id id)
{
	AP_UnixDialog_Download_File * p = new AP_UnixDialog_Download_File(pFactory,id);
	return (XAP_Dialog*)p;
}

AP_UnixDialog_Download_File::AP_UnixDialog_Download_File(XAP_DialogFactory * pDlgFactory,
											 XAP_Dialog_Id id)
	: AP_Dialog_Download_File(pDlgFactory,id)
{
	m_windowMain = NULL;
	m_buttonCancel = NULL;
	m_gc = NULL;
}

AP_UnixDialog_Download_File::~AP_UnixDialog_Download_File(void)
{
	DELETEP(m_gc);
}

/*****************************************************************/

// These are all static callbacks, bound to GTK or GDK events.

static void s_cancel_clicked(GtkWidget * widget,
						 AP_UnixDialog_Download_File * dlg)
{
	UT_ASSERT(widget && dlg);
	
	dlg->event_Cancel();
	
}

static void s_delete_clicked(GtkWidget * /* widget */,
							 gpointer /* data */,
							 AP_UnixDialog_Download_File * dlg)
{
	UT_ASSERT(dlg);

	dlg->event_WindowDelete();
}


/*****************************************************************/


void AP_UnixDialog_Download_File::_runModal(XAP_Frame * pFrame)
{
	// stash away the frame
	m_pFrame = static_cast<XAP_UnixFrame *>(pFrame);

	// Build the window's widgets and arrange them
	GtkWidget * mainWindow = _constructWindow();
	UT_ASSERT(mainWindow);

	connectFocus(GTK_WIDGET(mainWindow),pFrame);
	
	// To center the dialog, we need the frame of its parent.
	XAP_UnixFrame * pUnixFrame = static_cast<XAP_UnixFrame *>(pFrame);
	UT_ASSERT(pUnixFrame);
	
	// Get the GtkWindow of the parent frame
	GtkWidget * parentWindow = pUnixFrame->getTopLevelWindow();
	UT_ASSERT(parentWindow);
	
	// Center our new dialog in its parent and make it a transient
	// so it won't get lost underneath
	centerDialog(parentWindow, mainWindow);

	// Show the top level dialog,
	gtk_widget_show(mainWindow);

	// Make it modal, and stick it up top
	gtk_grab_add(mainWindow);

	// Run into the GTK event loop for this window.
	gtk_main();

	if(mainWindow && GTK_IS_WIDGET(mainWindow))
	  gtk_widget_destroy(mainWindow);
}

void
AP_UnixDialog_Download_File::_abortDialog(void)
{
	event_WindowDelete();
}

void AP_UnixDialog_Download_File::event_Cancel(void)
{
	_setUserAnswer(a_CANCEL);
	gtk_main_quit();
}

void AP_UnixDialog_Download_File::event_WindowDelete(void)
{
	gtk_main_quit();
}

/*****************************************************************/
GtkWidget * AP_UnixDialog_Download_File::_constructButtonCancel(void)
{
	GtkWidget *buttonCancel;
	const XAP_StringSet * pSS = m_pApp->getStringSet();

	buttonCancel = gtk_button_new_with_label(pSS->getValue(XAP_STRING_ID_DLG_Cancel));
	gtk_widget_show (buttonCancel);
	gtk_widget_set_usize (buttonCancel, 85, 0);

	return buttonCancel;
}

GtkWidget * AP_UnixDialog_Download_File::_constructWindow(void)
{
	GtkWidget *windowDL;
	GtkWidget *hbox;
	GtkWidget *vboxMain;
	GtkWidget *label;
	GtkWidget *buttonboxAction;
	GtkWidget *buttonCancel;

	// we use this for all sorts of strings that can't appear in the string sets
	char buf[4096];

	const XAP_StringSet * pSS = m_pApp->getStringSet();

	windowDL = gtk_window_new(GTK_WINDOW_DIALOG);
	g_object_set_data (G_OBJECT (windowDL), "windowDL", windowDL);
	gtk_window_set_title (GTK_WINDOW (windowDL), pSS->getValue(XAP_STRING_ID_DLG_DlFile_Title));
	gtk_window_set_policy (GTK_WINDOW (windowDL), FALSE, FALSE, FALSE);
	
	vboxMain = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vboxMain);
	gtk_container_add (GTK_CONTAINER (windowDL), vboxMain);
	
	sprintf(buf, pSS->getValue(XAP_STRING_ID_DLG_DlFile_Status), getDescription(), getURL());
	label = gtk_label_new (buf);
	g_object_set_data (G_OBJECT (vboxMain), "label", label);
	gtk_misc_set_padding (GTK_MISC(label), 10, 10);
	gtk_box_pack_start (GTK_BOX(vboxMain), label, TRUE, TRUE, 0);
	gtk_widget_show (label);

	buttonboxAction = gtk_hbutton_box_new ();
	gtk_widget_show (buttonboxAction);
	gtk_box_pack_start (GTK_BOX (vboxMain), buttonboxAction, FALSE, TRUE, 0);
	gtk_container_border_width (GTK_CONTAINER (buttonboxAction), 11);

	buttonCancel = _constructButtonCancel();
	g_object_set_data (G_OBJECT (buttonboxAction), "buttonCancel", buttonCancel);
	gtk_box_pack_start (GTK_BOX(buttonboxAction), buttonCancel, FALSE, FALSE, 0);
 	GTK_WIDGET_SET_FLAGS (buttonCancel, GTK_CAN_DEFAULT);
	gtk_widget_grab_default (buttonCancel);

	
	g_signal_connect(G_OBJECT(buttonCancel),
					   "clicked",
					   G_CALLBACK(s_cancel_clicked),
					   (gpointer) this);
	
	g_signal_connect(G_OBJECT(windowDL),
							 "delete_event",
							 G_CALLBACK(s_delete_clicked),
							 (gpointer) this);

	g_signal_connect_after(G_OBJECT(windowDL),
							 "destroy",
							 NULL,
							 NULL);

	// Update member variables with the important widgets that
	// might need to be queried or altered later.

	m_windowMain = windowDL;
	m_buttonCancel = buttonCancel;

	return windowDL;
}

