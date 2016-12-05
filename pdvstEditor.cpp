/* PdVst v0.0.2 - VST - Pd bridging plugin
** Copyright (C) 2004 Joseph A. Sarlo
**
** This program is free software; you can redistribute it and/orsig
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
**
** jsarlo@ucsd.edu
*/

#ifndef __pdvstEditor
#include "pdvstEditor.hpp"
#endif

#ifndef __PDVST_H
#include "pdvst.hpp"
#endif

#include <stdio.h>
#include <windows.h>

extern HINSTANCE hInstance;
//extern int pdGuiWindowHeight, pdGuiWindowWidth;
int useCount = 0;

pdvstEditor::pdvstEditor(AudioEffect *effect)
           : AEffEditor(effect)
{
   systemWindowHidden=false;
   effect->setEditor(this);

   editWindow = NULL;
    //((pdvst *)(effect))->debugLog("begin editor");
}

pdvstEditor::~pdvstEditor()
{
}

bool pdvstEditor::getRect(ERect **erect)
{
    static ERect r = {0,0,((pdvst *)this->effect)->customGuiHeight,((pdvst *)this->effect)->customGuiWidth};
   *erect = &r;
    return (true);
}


bool pdvstEditor::open(void *ptr)
{


   systemWindow = ptr;



    WNDCLASS myWindowClass;

    useCount++;
    if (useCount == 1)
    {
        myWindowClass.style = 0;
        myWindowClass.lpfnWndProc = (WNDPROC)windowProc;
        myWindowClass.cbClsExtra = 0;
        myWindowClass.cbWndExtra = 0;
        myWindowClass.hInstance = hInstance;
        myWindowClass.hIcon = 0;
        myWindowClass.hCursor = 0;
        myWindowClass.hbrBackground = GetSysColorBrush(COLOR_APPWORKSPACE);
       // myWindowClass.hbrBackground = CreateSolidBrush(RGB(180, 180, 180));
        myWindowClass.lpszMenuName = 0;
        myWindowClass.lpszClassName = "pdvstWindowClass";
        RegisterClass(&myWindowClass);
    }
    LPTSTR plugName= new TCHAR[MAXSTRINGSIZE];

     GetWindowText((HWND)systemWindow,plugName,MAXSTRINGSIZE);
     ((pdvst *)this->effect)->sendPlugName((char *) plugName );

 //  sprintf( ((pdvst *)this->effect)->displayString,"valeurSetParameter=%f",((pdvst *)this->effect)->vstParam[0]);


 //((pdvst *)this->effect)->sendPlugName((char *) ((pdvst *)this->effect)->displayString );
    //////
    editWindow = CreateWindowEx(0, "pdvstWindowClass", "Window",
                                WS_CHILD | WS_VISIBLE, 0,0,((pdvst *)this->effect)->customGuiWidth,((pdvst *)this->effect)->customGuiHeight,
                                (HWND)systemWindow, NULL, hInstance, NULL);
   SetWindowLongPtr(editWindow, GWLP_USERDATA, (LONG_PTR)this);

  /*  openButton = CreateWindowEx(0, "Button", "open", WS_CHILD | WS_VISIBLE,
                                70, 24, 120, 25, editWindow, NULL, hInstance,
                                NULL);
                                */

    ((pdvst *)this->effect)->sendGuiAction(1);
  return (true);
}



void pdvstEditor::close()
{



       if (pdGuiWindow)
       {
        // detach pdGuiWindow from editWindow
           SetParent(pdGuiWindow,NULL);
           //Remove WS_CHILD style and add WS_POPUP style
        DWORD style = GetWindowLong(pdGuiWindow,GWL_STYLE);
      // style = style & ~(WS_POPUPWINDOW);
        style = style | WS_POPUP;
       style = style & ~(WS_CHILD);
       style = style |(WS_SYSMENU);
       style = style | (WS_BORDER);
        SetWindowLong(pdGuiWindow,GWL_STYLE,style);
        systemWindowHidden=false;
        pdGuiWindow=NULL;



       }

    if (editWindow)
    {
          ((pdvst *)this->effect)->sendGuiAction(0);
          DestroyWindow(editWindow);

    }

    editWindow = NULL;
    useCount = 0;
    UnregisterClass("pdvstWindowClass", hInstance);
}

void pdvstEditor::idle()
{

       // JYG :: masquer la fenêtre GUI créée par hôte VST pour laisser puredata le faire
  //  SetWindowPos((HWND)systemWindow,HWND_TOPMOST,-300,-300,0,0,SWP_NOSIZE);  // déplacer la fenetre
  //  SetWindowPos((HWND)systemWindow,NULL,0,0,0,0,SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_HIDEWINDOW); //masquer la fenêtre
  //  systemWindowHidden=true;
  // JYG
                if (((pdvst *)this->effect)->guiNameUpdated)
                if (editWindow)//&&!pdGuiWindow)
            {
           HWND tref=NULL;

            parms.ref = &tref;
            strcpy(parms.s,(char*)((pdvst *)this->effect)->guiName);

            parms.exact = false;
            EnumWindows(enumwnd,(LPARAM)&parms);
            pdGuiWindow = tref;



           // pdGuiWindow=FindWindow(NULL,"Pd_Gain(gui).pd  - C:/Program Files (x86)/Ableton/Live 8.0.4/Program/pdvst");
           //  pdGuiWindow=FindWindow(NULL,(char*)((pdvst *)this->effect)->guiName);

             if (pdGuiWindow){

               {
                if(SetParent(pdGuiWindow,(HWND)editWindow))//systemWindow))
                {
                ((pdvst *)this->effect)->guiNameUpdated=0;
                //Remove WS_POPUP style and add WS_CHILD style
               DWORD style = GetWindowLong(pdGuiWindow,GWL_STYLE);
              // style = style & ~(WS_POPUPWINDOW);
                style = style & ~(WS_POPUP);
               style = style & ~(WS_CHILD);    // WS_CHILD Crashes tcltk with reaper
               style = style & ~(WS_SYSMENU);
               style = style & ~(WS_BORDER);
               style = style & ~(WS_HSCROLL);
                style = style & ~(WS_VSCROLL);
                style = style & ~(WS_SIZEBOX);
                style = style & ~(WS_CAPTION);
                SetWindowLong(pdGuiWindow,GWL_STYLE,style);
                MoveWindow(pdGuiWindow, 0,0,((pdvst *)this->effect)->customGuiWidth,((pdvst *)this->effect)->customGuiHeight,TRUE);
                systemWindowHidden=true;
                }
                }

             }
            }





    AEffEditor::idle();
}

LONG WINAPI pdvstEditor::windowProc(HWND hwnd, UINT message,
                                    WPARAM wParam, LPARAM lParam)
{
    pdvstEditor *self = (pdvstEditor *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (message == WM_COMMAND)
        {
            if (HIWORD(wParam) == BN_CLICKED &&
                (HWND)lParam == self->openButton)
            {
               // ((pdvst *)self->effect)->sendGuiAction(1);
            }
        }

    return (DefWindowProc(hwnd, message, wParam, lParam));
}



 BOOL CALLBACK  enumwnd(HWND hwnd,LPARAM lParam)
{
    enumparms *parms = (enumparms *)lParam;
    char buf[256];
    GetWindowText(hwnd,buf,sizeof buf);
    if(parms->exact? strcmp(buf,parms->s) == 0 : strstr(buf,parms->s) != NULL) {
        *parms->ref = hwnd;
        return FALSE;
    }
    else {
        // also search for child windows
        EnumChildWindows(hwnd,enumwnd,lParam);
        return TRUE;
    }
}
// ***********
