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
int useCount = 0;

pdvstEditor::pdvstEditor(AudioEffect *effect)
           : AEffEditor(effect)
{
   systemWindowHidden=false;
    vstEditWindowHide=((pdvst *)this->effect)->vstEditWindowHide;
   effect->setEditor(this);

   editWindow = NULL;
    //((pdvst *)(effect))->debugLog("begin editor");
}

pdvstEditor::~pdvstEditor()
{
}

bool pdvstEditor::getRect(ERect **erect)
{
    static ERect r = {0,0,70,270};
   *erect = &r;
    return (true);
}


bool pdvstEditor::open(void *ptr)
{
    ((pdvst *)this->effect)->sendGuiAction(1);

   systemWindow = ptr;
  //  systemWindowHidden=false;


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
                                WS_CHILD | WS_VISIBLE, 10, 10, 240, 55,
                                (HWND)systemWindow, NULL, hInstance, NULL);
   SetWindowLongPtr(editWindow, GWLP_USERDATA, (LONG_PTR)this);

  /*  openButton = CreateWindowEx(0, "Button", "open", WS_CHILD | WS_VISIBLE,
                                70, 24, 120, 25, editWindow, NULL, hInstance,
                                NULL);
                                */
  return (true);
}

void pdvstEditor::close()
{
  //  if (!pdGui)    SetParent(pdGui,NULL);
    ((pdvst *)this->effect)->sendGuiAction(0);
    if(vstEditWindowHide)
        systemWindowHidden=false;

    if (editWindow)
        DestroyWindow(editWindow);

    editWindow = NULL;
    useCount = 0;
    UnregisterClass("pdvstWindowClass", hInstance);
}

void pdvstEditor::idle()
{
    // JYG
    if (vstEditWindowHide&&!systemWindowHidden)
    {

        // JYG :: masquer la fenêtre GUI créée par hôte VST pour laisser puredata le faire
    SetWindowPos((HWND)systemWindow,HWND_TOPMOST,-300,-300,0,0,SWP_NOSIZE);  // déplacer la fenetre
    SetWindowPos((HWND)systemWindow,NULL,0,0,0,0,SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_HIDEWINDOW); //masquer la fenêtre
    systemWindowHidden=true;


  /*  pdConsole=FindWindow(NULL,"Pd");

    if (pdConsole){
        SetParent(pdConsole,(HWND)systemWindow);

    pdGui=FindWindow(NULL,"Pd_Gain(gui).pd  - C:/Program Files (x86)/Ableton/Live 8.0.4/Program/pdvst");

     if (pdGui){
        SetParent(pdGui,(HWND)systemWindow);
        //Remove WS_POPUP style and add WS_CHILD style
       DWORD style = GetWindowLong(pdGui,GWL_STYLE);
      // style = style & ~(WS_POPUPWINDOW);
        style = style & ~(WS_POPUP);
       style = style | WS_CHILD;
        SetWindowLong(pdGui,GWL_STYLE,style);

        systemWindowHidden=true;}
    }*/

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

