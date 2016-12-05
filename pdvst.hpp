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

#ifndef __PDVST_H
#define __PDVST_H

#include "audioeffectx.h"
#include "pdvstEditor.hpp"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#define PDBLKSIZE 64
#define MAXEXTERNS 128
#define MAXVSTBUFSIZE 4096
#define MAXPARAMS 128
#define MAXPROGRAMS 128
#define MAXCHANS 128
#define MAXFILENAMELEN 1024
#define MAXSTRLEN 4096
#define MAXEXTERNS 128
#define PDWAITMAX 1000
#define SETUPFILEEXT ".pdv"
#define DEFPDVSTBUFFERSIZE 1024

extern "C"
{
    #include "pdvstTransfer.h"
}

/* program data */
typedef struct _pdvstProgram
{
    char name[MAXSTRLEN];
    float paramValue[MAXPARAMETERS];
} pdvstProgram;

class pdVstBuffer
{

    friend class pdvst;

public:
    pdVstBuffer(int nChans);
    ~pdVstBuffer();
    void resize(int newSize);

protected:
    int nChannels;
    int inFrameCount;
    int outFrameCount;
    int size;
    float **in;
    float **out;
};

class pdvst : public AudioEffectX
{
    friend class pdvstEditor;

public:
    pdvst(audioMasterCallback audioMaster);
    ~pdvst();

    virtual void process(float **inputs, float **outputs, VstInt32 sampleFrames);
    virtual void processReplacing(float **inputs, float **outputs,
                                  VstInt32 sampleFrames);
    virtual VstInt32 processEvents (VstEvents* events);
    virtual void setProgram(VstInt32 prgmNum);
    virtual VstInt32 getProgram();
    virtual void setProgramName(char *name);
    virtual void getProgramName(char *name);
    virtual void setParameter(VstInt32 index, float value);
    virtual float getParameter(VstInt32 index);
    virtual void getParameterLabel(VstInt32 index, char *label);
    virtual void getParameterDisplay(VstInt32 index, char *text);
    virtual void getParameterName(VstInt32 index, char *text);
    virtual bool getEffectName(char* name);
    virtual bool getOutputProperties(VstInt32 index, VstPinProperties* properties);
    virtual VstInt32 canDo (char* text);
   // virtual VstInt32 canMono ();
    virtual void suspend();
    virtual void resume();
        void sendGuiAction(int action);
        void sendPlugName(char * name );  // JYG : to send plug name to puredatapatch

 LPTSTR displayString;//= new TCHAR[MAXSTRINGSIZE];

   HWND pdGui;

protected:
    static int referenceCount;
    void debugLog(char *fmt, ...);
    FILE *debugFile;
    int pdInFrameCount;
    int pdOutFrameCount;
    pdVstBuffer *audioBuffer;
    char pluginPath[MAXFILENAMELEN];
    char vstPluginPath[MAXFILENAMELEN];
    char pluginName[MAXSTRLEN];
    long pluginId;
    char pdFile[MAXFILENAMELEN];
    char errorMessage[MAXFILENAMELEN];
    char externalLib[MAXEXTERNS][MAXSTRLEN];
    float vstParam[MAXPARAMS];
    char **vstParamName;
    int nParameters;
    pdvstProgram *program;
    int nPrograms;
    int nChannels;
    int nExternalLibs;
    bool customGui;
    int customGuiHeight;
    int customGuiWidth;

    bool isASynth;
    bool dspActive;
    HANDLE pdvstTransferMutex,
           pdvstTransferFileMap,
           vstProcEvent,
           pdProcEvent;
    pdvstTransferData *pdvstData;
    char pdvstTransferMutexName[1024];
    char pdvstTransferFileMapName[1024];
    char vstProcEventName[1024];
    char pdProcEventName[1024];
    char guiName[1024];
    bool guiNameUpdated;  // used to signal to editor that the parameter guiName has changed
    void startPd();
    void parseSetupFile();

    void updatePdvstParameters();
    void setSyncToVst(int value);
    //  {JYG
    DWORD timeFromStartup; // to measure time before vst::setProgram call
    #ifdef VSTMIDIOUTENABLE
    /** Stores a single midi event.
   * The 'events' fields points to midiEvent.
   */
  VstEvents    midiOutEvents;
  /** Last played note information is stored in midiEvent.
   */
  VstMidiEvent midiEvent;

  // version alternative (http://stackoverflow.com/questions/359125/how-to-store-data-in-variable-length-arrays-without-causing-memory-corruption)
  struct VstEvents *evnts;
  VstMidiEvent midiEvnts[MAXMIDIOUTQUEUESIZE];

    int nbFramesWithoutMidiOutEvent;
    #endif //VSTMIDIOUTENABLE
    int syncDefeatNumber;


    // JYG  }
};

#endif
