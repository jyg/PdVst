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
#define __pdvstEditor
#include <stdio.h>
#include <windows.h>
#include "aeffeditor.h"



class pdvstEditor : public AEffEditor
{
    friend class pdvst;
public:
    pdvstEditor(AudioEffect *effect);
    virtual ~pdvstEditor ();
    HWND openButton;;
    HWND editWindow;
    HWND pdGuiWindow;
   static LONG WINAPI windowProc(HWND mhwnd, UINT message,
      WPARAM wParam, LPARAM lParam);

protected:
    virtual bool getRect(ERect **erect);
    virtual bool open(void *ptr);
    virtual void close();
    virtual void idle();

private:
    void *systemWindow;
    bool systemWindowHidden;
    bool vstEditWindowHide;
    HWND pdGui;
    HWND pdConsole;


};
#endif
