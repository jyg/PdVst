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

#define _WIN32_WINDOWS 0x410

#include "pdvst.hpp"
#include <malloc.h>
#include <process.h>
#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fstream>
#include <unistd.h>


#ifdef _MSC_VER
#define stat _stat
#endif

extern HINSTANCE hInstance;
extern bool globalDebug;
extern int globalNChannels;
extern int globalNPrograms;
extern int globalNParams;
extern long globalPluginId;
extern int globalNExternalLibs;
extern char globalExternalLib[MAXEXTERNS][MAXSTRLEN];
extern char globalVstParamName[MAXPARAMETERS][MAXSTRLEN];
extern char globalPluginPath[MAXFILENAMELEN];
extern char globalVstPluginPath[MAXFILENAMELEN];  // chemin d'accès dans le dossier vst contenant la dll)
extern char globalPluginName[MAXSTRLEN];
extern char globalPdFile[MAXFILENAMELEN];
extern char globalPureDataPath[MAXFILENAMELEN];
extern char globalHostPdvstPath[MAXFILENAMELEN];
extern bool globalCustomGui;
extern int globalCustomGuiWidth;
extern int globalCustomGuiHeight;

extern bool globalIsASynth;
extern pdvstProgram globalProgram[MAXPROGRAMS];

int pdvst::referenceCount = 0;


   /* change '/' characters to the system's native file separator */
void sys_bashfilename(const char *from, char *to)
{
    char c;
    while ((c = *from++))
    {
#ifdef _WIN32
        if (c == '/') c = '\\';
#endif
        *to++ = c;
    }
    *to = 0;
}
    /* change the system's native file separator to '/' characters  */
void sys_unbashfilename(const char *from, char *to)
{
    char c;
    while ((c = *from++))
    {
#ifdef _WIN32
        if (c == '\\') c = '/';
#endif
        *to++ = c;
    }
    *to = 0;
}
bool fexists(const char *filename)
{
  std::ifstream ifile(filename);
  return ifile;
}


//-------------------------------------------------------------------------------------------------------
AudioEffect* createEffectInstance (audioMasterCallback audioMaster)
{

	return new pdvst (audioMaster);
}

pdvst::pdvst(audioMasterCallback audioMaster)
      :AudioEffectX(audioMaster, globalNPrograms, globalNParams)
{
     // set debug output
    debugFile = fopen("pdvstdebug.txt", "wt");
    // debugFile = NULL;

    // copy global data
    isASynth = globalIsASynth;
    customGui = globalCustomGui;
    customGuiHeight = globalCustomGuiHeight;
    customGuiWidth = globalCustomGuiWidth;
    nChannels = globalNChannels;
    nPrograms = globalNPrograms;
    nParameters = globalNParams;
    pluginId = globalPluginId;
    nExternalLibs = globalNExternalLibs;
    debugLog("name: %s", globalPluginName);
    debugLog("synth: %d", globalIsASynth);

    // VST setup
    setNumInputs(nChannels);
    setNumOutputs(nChannels);
    setUniqueID(pluginId);
    setInitialDelay(PDBLKSIZE * 2);
    canProcessReplacing();
    if (isASynth)
    {
        isSynth();
    }

    int i, j;



    // initialize memory
    vstParamName = new char*[MAXPARAMETERS];
    for (i = 0; i < MAXPARAMETERS; i++)
        vstParamName[i] = new char[MAXSTRLEN];
    memset(vstParam, 0, MAXPARAMETERS * sizeof(float));
    program = new pdvstProgram[MAXPROGRAMS];

    for (i = 0; i < nExternalLibs; i++)
    {
        strcpy(externalLib[i], globalExternalLib[i]);
    }
    debugLog("channels: %d", nChannels);
    audioBuffer = new pdVstBuffer(nChannels);
    for (i = 0; i < MAXPARAMETERS; i++)
    {
        strcpy(vstParamName[i], globalVstParamName[i]);
    }
    strcpy(pluginPath, globalPluginPath);
    strcpy(vstPluginPath, globalVstPluginPath);
    strcpy(pluginName, globalPluginName);
    strcpy(pdFile, globalPdFile);
    debugLog("path: %s", pluginPath);
    debugLog("nParameters = %d", nParameters);
    debugLog("nPrograms = %d", nPrograms);
    for (i = 0; i < nPrograms; i++)
    {
        strcpy(program[i].name, globalProgram[i].name);
        for (j = 0; j < nParameters; j++)
        {
            program[i].paramValue[j] = globalProgram[i].paramValue[j];
        }
        debugLog("    %s", program[i].name);
    }
    // create editor
    if (customGui)
    {
       editor = new pdvstEditor(this);

        debugLog("   -has custom GUI-");
    }
    guiNameUpdated=false;

    #ifdef VSTMIDIOUTENABLE
    // initializing structures for midiout support

    nbFramesWithoutMidiOutEvent=0;

    evnts = (struct VstEvents*)malloc(sizeof(struct VstEvents) +
                                  MAXMIDIOUTQUEUESIZE*sizeof(VstEvent*));
    //initialisation du tableau  midiEvnts[MAXMIDIOUTQUEUESIZE];
    for (int k=0; k<MAXMIDIOUTQUEUESIZE;k++)
    {
        // Constants to all midi messages we send (NoteOn and NoteOff)
        midiEvnts[k].type = kVstMidiType;
        midiEvnts[k].byteSize = sizeof(VstMidiEvent);
        midiEvnts[k].deltaFrames = 0;
        midiEvnts[k].flags  |= kVstMidiEventIsRealtime;
        midiEvnts[k].detune = 0;
        midiEvnts[k].noteOffVelocity = (char)0;
        midiEvnts[k].reserved1 = 0;
        midiEvnts[k].reserved2 = 0;
        midiEvnts[k].midiData[3] = 0;
        midiEvnts[k].noteOffset = 0;

        //cast
        evnts->events[k] = (VstEvent*)&midiEvnts[k];
    }


     #endif //VSTMIDIOUTENABLE



        //timeBeginPeriod(1);
    debugLog("startingPd...");
    // start pd.exe
    startPd();
    debugLog("done");
    //setProgram(curProgram);
    referenceCount++;

     //  {JYG   see pdvst::setProgram below for explanation
    timeFromStartup=GetTickCount();
    //  JYG  }
}

pdvst::~pdvst()
{
    int i;

    referenceCount--;
    pdvstData->active = 0;
    CloseHandle(pdvstTransferMutex);
    UnmapViewOfFile(pdvstTransferFileMap);
    CloseHandle(pdvstTransferFileMap);
    for (i = 0; i < MAXPARAMETERS; i++)
        delete vstParamName[i];
    delete vstParamName;
    delete program;
    delete audioBuffer;
    if (debugFile)
    {
        fclose(debugFile);
    }
}

void pdvst::debugLog(char *fmt, ...)
{
    va_list ap;

    if (debugFile)
    {
        va_start(ap, fmt);
        vfprintf(debugFile, fmt, ap);
        va_end(ap);
        putc('\n', debugFile);
        fflush(debugFile);
    }
}

void pdvst::startPd()
{

    int i;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    char commandLineArgs[MAXSTRLEN],
         debugString[MAXSTRLEN],pathToCheck[MAXSTRLEN];

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    sprintf(pdvstTransferMutexName, "mutex%d%x", GetCurrentProcessId(), this);
    sprintf(pdvstTransferFileMapName, "filemap%d%x", GetCurrentProcessId(), this);
    sprintf(vstProcEventName, "vstprocevent%d%x", GetCurrentProcessId(), this);
    sprintf(pdProcEventName, "pdprocevent%d%x", GetCurrentProcessId(), this);
    pdvstTransferMutex = CreateMutex(NULL, 0, pdvstTransferMutexName);
    vstProcEvent = CreateEvent(NULL, TRUE, TRUE, vstProcEventName);
    pdProcEvent = CreateEvent(NULL, TRUE, FALSE, pdProcEventName);
    pdvstTransferFileMap = CreateFileMapping(INVALID_HANDLE_VALUE,
                                             NULL,
                                             PAGE_READWRITE,
                                             0,
                                             sizeof(pdvstTransferData),
                                             pdvstTransferFileMapName);
    pdvstData = (pdvstTransferData *)MapViewOfFile(pdvstTransferFileMap,
                                                   FILE_MAP_ALL_ACCESS,
                                                   0,
                                                   0,
                                                   sizeof(pdvstTransferData));
    pdvstData->active = 1;
    pdvstData->blockSize = PDBLKSIZE;
    pdvstData->nChannels = nChannels;
    pdvstData->sampleRate = (int)getSampleRate();
    pdvstData->nParameters = nParameters;
    pdvstData->guiState.updated = 0;
    pdvstData->guiState.type = FLOAT_TYPE;
    pdvstData->guiState.direction = PD_RECEIVE;
    for (i = 0; i < MAXPARAMETERS; i++)
    {
        pdvstData->vstParameters[i].direction = PD_RECEIVE;
        pdvstData->vstParameters[i].updated = 0;
    }
    if (globalDebug)
    {
        strcpy(debugString, "");
    }
    else
    {
        strcpy(debugString, " -nogui");
    }
    if (strcmp(globalPureDataPath, "") == 0)  // unspecified puredata path. Defaults to globalHostPdvstPath
        {
            sprintf(commandLineArgs, "%s\\pd\\bin\\pd.exe", globalHostPdvstPath);
             if( access( commandLineArgs, F_OK ) != -1 )   // check if file exists
                sprintf(commandLineArgs, "\"%s\\pd\\bin\\pd.exe\"", globalHostPdvstPath);
             else
             {
                 sprintf(commandLineArgs, "%s..\\.pd\\bin\\pd.exe", globalPluginPath);
                  //MessageBox(NULL,commandLineArgs,"commandLineArgs...",MB_OK);
                 if( access( commandLineArgs, F_OK ) != -1 )   // check if file exists
                    sprintf(commandLineArgs, "\"%s..\\.pd\\bin\\pd.exe\"", globalPluginPath);
                 else
                    {
                        sprintf(errorMessage,"pd.exe program not found in \n1: %spd\\bin\\\n2: %s..\\.pd\\bin\\\nCheck your settings in \n%s%s.pdv",
                                globalHostPdvstPath,globalPluginPath,globalPluginPath,globalPluginName);
                        MessageBox(NULL,errorMessage,"ERROR",MB_OK);
                        return;
                    }
             }
        }
    else
        {
            sprintf(commandLineArgs, "%s\\bin\\pd.exe", globalPureDataPath);
            if ( access( commandLineArgs, F_OK ) != -1 )   // file exists
                sprintf(commandLineArgs,"\"%s\\bin\\pd.exe\"", globalPureDataPath);
            else
                 {
                    sprintf(errorMessage,"pd.exe program not found. Check your settings in \n%s%s.pdv \n setup file. (search for the line PDPATH)",
                            globalPluginPath,globalPluginName);
                    MessageBox(NULL,errorMessage,"ERROR",MB_OK);
                    return;
                }
        }



           // if (!fexists(commandLineArgs))
           //   sprintf(commandLineArgs, "\"%s\\pd\\bin\\pd.exe\"", pluginPath);
            // check validity of pd.exe path
         /*
            struct stat statbuf;
            sys_bashfilename(commandLineArgs, pathToCheck);
            if (stat(pathToCheck, &statbuf) < 0)
                // if failed, use defaults
                sprintf(commandLineArgs, "\"%s\\pd\\bin\\pd.exe\"", pluginPath);*/


    sprintf(commandLineArgs,
            "%s%s",
            commandLineArgs,
            debugString);
    sprintf(commandLineArgs,
            "%s -schedlib vstschedlib",
            commandLineArgs);
    sprintf(commandLineArgs,
            "%s -extraflags \"-vstproceventname %s -pdproceventname %s -vsthostid %d -mutexname %s -filemapname %s\"",
            commandLineArgs,
            vstProcEventName,
            pdProcEventName,
            GetCurrentProcessId(),
            pdvstTransferMutexName,
            pdvstTransferFileMapName);
    sprintf(commandLineArgs,
            "%s -outchannels %d -inchannels %d",
            commandLineArgs,
            nChannels,
            nChannels);
    sprintf(commandLineArgs,
            "%s -r %d",
            commandLineArgs,
            (int)getSampleRate());

    sprintf(commandLineArgs,
            "%s -open \"%s%s\"",
            commandLineArgs,
            pluginPath,
            pdFile);
    sprintf(commandLineArgs,
            "%s -path \"%s\"",
            commandLineArgs,
            pluginPath);
    for (i = 0; i < nExternalLibs; i++)
    {
        sprintf(commandLineArgs,
                "%s -lib %s",
                commandLineArgs,
                externalLib[i]);
    }
    debugLog("command line: %s", commandLineArgs);
    suspend();
    CreateProcess(NULL,
                  commandLineArgs,
                  NULL,
                  NULL,
                  0,
                  0,
                  NULL,
                  NULL,
                  &si,
                  &pi);
}

void pdvst::sendGuiAction(int action)
{
   WaitForSingleObject(pdvstTransferMutex, 10);//  if (WaitForSingleObject(pdvstTransferMutex, 100)!= WAIT_TIMEOUT)
   {
        pdvstData->guiState.direction = PD_RECEIVE;
        pdvstData->guiState.type = FLOAT_TYPE;
        pdvstData->guiState.value.floatData = (float)action;
        pdvstData->guiState.updated = 1;
        ReleaseMutex(pdvstTransferMutex);
   }
}

void pdvst::sendPlugName(char * pName )     // pour envoyer le nom du plugin à puredata
{

     WaitForSingleObject(pdvstTransferMutex, 10);//if (WaitForSingleObject(pdvstTransferMutex, 100)!= WAIT_TIMEOUT)
    {
        pdvstData->plugName.direction = PD_RECEIVE;
        pdvstData->plugName.type = STRING_TYPE;
        strcpy(pdvstData->plugName.value.stringData,pName);
        pdvstData->plugName.updated = 1;
        ReleaseMutex(pdvstTransferMutex);
    }
}

void pdvst::setSyncToVst(int value)
{

   if (WaitForSingleObject(pdvstTransferMutex, 10)==WAIT_OBJECT_0)
    {
        if (pdvstData->syncToVst != value)
        {
            pdvstData->syncToVst = value;
        }
        ReleaseMutex(pdvstTransferMutex);
    }

}

void pdvst::suspend()
{
    int i;

    setSyncToVst(0);
    SetEvent(vstProcEvent);
    for (i = 0; i < audioBuffer->nChannels; i++)
    {
        memset(audioBuffer->in[i], 0, audioBuffer->size * sizeof(float));
        memset(audioBuffer->out[i], 0, audioBuffer->size * sizeof(float));
    }
    audioBuffer->inFrameCount = audioBuffer->outFrameCount = 0;
    dspActive = false;
    setInitialDelay(PDBLKSIZE * 2);
}

void pdvst::resume()
{
    int i;

    setSyncToVst(1);
    SetEvent(vstProcEvent);
    for (i = 0; i < audioBuffer->nChannels; i++)
    {
        memset(audioBuffer->in[i], 0, audioBuffer->size * sizeof(float));
        memset(audioBuffer->out[i], 0, audioBuffer->size * sizeof(float));
    }
    audioBuffer->inFrameCount = audioBuffer->outFrameCount = 0;
    dspActive = true;
    setInitialDelay(PDBLKSIZE * 2);
    if (isASynth)
    {
        //wantEvents();  deprecated since VST 2.4
    }
}

void pdvst::setProgram(VstInt32 prgmNum)
{
   debugLog("appel de setProgram %d", prgmNum);
   int i;
    if (prgmNum >= 0 && prgmNum < nPrograms)
        {
            curProgram = prgmNum;


   // {JYG    to prevent host call of setProgram to override current param settings
   // see https://www.kvraudio.com/forum/viewtopic.php?p=6391144
           // sprintf(errorMessage,"GetTickCount=%d\ntimeFromStartup=%d",
             //       GetTickCount(),timeFromStartup);
            //MessageBox(NULL, errorMessage,"setProgram",MB_OK);
            if ((GetTickCount()-timeFromStartup) < 1000)
                return;
            else
            {
               for (i = 0; i < nParameters; i++)
                {
                    setParameter(i, program[curProgram].paramValue[i]);
                  //  MessageBox(NULL,"setvalue","setProgram",MB_OK);
                }
            }
   }

}

VstInt32 pdvst::getProgram()
{
   return curProgram;
}

void pdvst::setProgramName(char *name)
{
    strcpy(program[curProgram].name, name);
}

void pdvst::getProgramName(char *name)
{
    strcpy(name, program[curProgram].name);
        //  {JYG   see pdvst::setProgram below for explanation
    timeFromStartup=GetTickCount();
    //  JYG  }
}

void pdvst::setParameter(VstInt32 index, float value)
{
    if (vstParam[index] != value)
    {
        vstParam[index] = value;
        WaitForSingleObject(pdvstTransferMutex, 10);// if (WaitForSingleObject(pdvstTransferMutex, 100)!= WAIT_TIMEOUT)
        {
            pdvstData->vstParameters[index].type = FLOAT_TYPE;
            pdvstData->vstParameters[index].value.floatData = value;
            pdvstData->vstParameters[index].direction = PD_RECEIVE;
            pdvstData->vstParameters[index].updated = 1;
            ReleaseMutex(pdvstTransferMutex);
        }
    }
}

float pdvst::getParameter(VstInt32 index)
{
    if (index >= 0 && index < MAXPARAMETERS)
        return vstParam[index];
    else
        return 0;
}

void pdvst::getParameterName(VstInt32 index, char *label)
{
    if (index >= 0 && index < MAXPARAMETERS)
        strcpy(label, vstParamName[index]);
}

void pdvst::getParameterDisplay(VstInt32 index, char *text)
{
    if (index >= 0 && index < MAXPARAMETERS)
        float2string(vstParam[index], text, 10);
}

void pdvst::getParameterLabel(VstInt32 index, char *label)
{
   strcpy(label, "");
  /* if (index >= 0 && index < MAXPARAMETERS)
        strcpy(label, vstParamName[index]);*/
}

// FIXME
bool pdvst::getEffectName(char* name)
{
	strcpy (name, globalPluginName);
	return true;
}

// FIXME
bool pdvst::getOutputProperties(VstInt32 index, VstPinProperties* properties)
{
	if (1)//index < 2)
	{
		sprintf (properties->label, "pd-vst %1d", index + 1);
		properties->flags = kVstPinIsActive;
		if ((index %2)==0)
			properties->flags |= kVstPinIsStereo;	// make channel 1+2 stereo
		return true;
	}
	return false;
}

VstInt32 pdvst::canDo(char* text)
{
    //if (isASynth)
    if (1)
    {
	    if (!strcmp (text, "sendVstEvents"))
		    return 1;
	    if (!strcmp (text, "sendVstMidiEvent"))
		    return 1;
        if (!strcmp (text, "receiveVstEvents"))
            return 1;
	    if (!strcmp (text, "receiveVstMidiEvent"))
		    return 1;
    }
	return -1;
}

void pdvst::process(float **input, float **output, VstInt32 sampleFrames)
{
   #ifdef VSTMIDIOUTENABLE

     WaitForSingleObject(pdvstTransferMutex, 10);//if (WaitForSingleObject(pdvstTransferMutex, 100)!= WAIT_TIMEOUT)
    {
        if (pdvstData->midiOutQueueUpdated)
        {
            // Faire une loop pour parcourir la midiqueue et recopier
            //dans la structure midiOutEvents
            int midiOutQueueSize=pdvstData->midiOutQueueSize;
            for (int ka=0;ka<midiOutQueueSize;ka++)
            {
                midiEvnts[ka].midiData[0] =  pdvstData->midiOutQueue[ka].statusByte;
                midiEvnts[ka].midiData[1] = pdvstData->midiOutQueue[ka].dataByte1;
                midiEvnts[ka].midiData[2] = pdvstData->midiOutQueue[ka].dataByte2;
            }
            evnts->numEvents=midiOutQueueSize;

            sendVstEventsToHost(evnts);  /// TODO : rajouter un test de réussite
            pdvstData->midiOutQueueUpdated=0;
            pdvstData->midiOutQueueSize=0;

             evnts->numEvents=0;
        }
      ReleaseMutex(pdvstTransferMutex);
    }

    #endif // VSTMIDIOUTENABLE


  /*      // Get play head information
      VstTimeInfo * info = getTimeInfo(
      kVstTempoValid       |
      kVstPpqPosValid      |
      kVstTransportPlaying |
      kVstTransportChanged |
      kVstTransportCycleActive |
      kVstTransportRecording   |
      kVstAutomationWriting    |
      kVstAutomationReading    |
      0);

   /*    if(infos)
        {
            m_playing_list.setFloat(0, m_playinfos.isPlaying);
            m_playing_list.setFloat(1, m_playinfos.timeInSeconds);
            sendMessageAnything(m_patch_tie, s_playing, m_playing_list);
            m_measure_list.setFloat(0, m_playinfos.bpm);
            m_measure_list.setFloat(1, m_playinfos.timeSigNumerator);
            m_measure_list.setFloat(2, m_playinfos.timeSigDenominator);
            m_measure_list.setFloat(3, m_playinfos.ppqPosition);
            m_measure_list.setFloat(4, m_playinfos.ppqPositionOfLastBarStart);
sendMessageAnything(m_patch_tie, s_measure, m_measure_list);

*/

  int i, j, k, l;
    int framesOut = 0;

    if (!dspActive)
    {
        resume();
    }
    else
    {
        setSyncToVst(1);
    }


    for (i = 0; i < sampleFrames; i++)
    {
        for (j = 0; j < audioBuffer->nChannels; j++)
        {
            audioBuffer->in[j][audioBuffer->inFrameCount] = input[j][i];
        }
        (audioBuffer->inFrameCount)++;
        // if enough samples to process then do it
        if (audioBuffer->inFrameCount >= PDBLKSIZE)
        {
            audioBuffer->inFrameCount = 0;
            updatePdvstParameters();
            // wait for pd process event
            //   { JYG
            //WaitForSingleObject(pdProcEvent, 10000);///int couldSync = WaitForSingleObject(pdProcEvent, 100)!= WAIT_TIMEOUT;
            WaitForSingleObject(pdProcEvent,10);
            //WaitForSingleObject(pdProcEvent, 10000);
            /// JYG: 10000 était une valeur trop élevée qui faisait planter ableton live 8.04, va savoir pourquoi
            // JYG }
            ///if (couldSync)
                ResetEvent(pdProcEvent);

            for (k = 0; k < PDBLKSIZE; k++)
            {
                for (l = 0; l < audioBuffer->nChannels; l++)
                {
                    while (audioBuffer->outFrameCount >= audioBuffer->size)
                    {
                        audioBuffer->resize(audioBuffer->size * 2);
                    }
                    // get pd processed samples
                    ///if (couldSync)
                    {
                    // get pd processed samples
                        audioBuffer->out[l][audioBuffer->outFrameCount] = pdvstData->samples[l][k];
                        // put new samples in for processing
                        pdvstData->samples[l][k] = audioBuffer->in[l][k];
                    }

                }
                (audioBuffer->outFrameCount)++;
            }
            ///if (couldSync)
                pdvstData->sampleRate = (int)getSampleRate();
            // signal vst process event
            SetEvent(vstProcEvent);
        }
    }
    // output pd processed samples
    for (i = 0; i < sampleFrames; i++)
    {
        for (j = 0; j < audioBuffer->nChannels; j++)
        {
            if (audioBuffer->outFrameCount > 0)
            {
                output[j][i] = audioBuffer->out[j][i];
            }
            else
            {
                output[j][i] = 0;
            }
        }
        if (audioBuffer->outFrameCount > 0)
        {
            audioBuffer->outFrameCount--;
            framesOut++;
        }
    }
    // shift any remaining buffered out samples
    if (audioBuffer->outFrameCount > 0)
    {
        for (i = 0; i < audioBuffer->nChannels; i++)
        {
            memmove(&(audioBuffer->out[i][0]),
                    &(audioBuffer->out[i][framesOut]),
                    (audioBuffer->outFrameCount) * sizeof(float));
        }
    }
}

DWORD WINAPI NotifySyncError(LPVOID lpParam) {
    MessageBoxA(NULL, "Le processus PureData met du temps à répondre", "PdVST", MB_OK);
    return 0;
}

void pdvst::processReplacing(float **input, float **output, VstInt32 sampleFrames)
{
   #ifdef VSTMIDIOUTENABLE

    if (WaitForSingleObject(pdvstTransferMutex, 10)==WAIT_OBJECT_0)
    {
        if (pdvstData->midiOutQueueUpdated)
        {
            // Faire une loop pour parcourir la midiqueue et recopier
            //dans la structure midiOutEvents
            int midiOutQueueSize=pdvstData->midiOutQueueSize;
            for (int ka=0;ka<midiOutQueueSize;ka++)
            {
                midiEvnts[ka].midiData[0] =  pdvstData->midiOutQueue[ka].statusByte;
                midiEvnts[ka].midiData[1] = pdvstData->midiOutQueue[ka].dataByte1;
                midiEvnts[ka].midiData[2] = pdvstData->midiOutQueue[ka].dataByte2;
            }
            evnts->numEvents=midiOutQueueSize;

            sendVstEventsToHost(evnts);  // rajouter un test de réussite
            pdvstData->midiOutQueueUpdated=0;
            pdvstData->midiOutQueueSize=0;

             evnts->numEvents=0;
        }
        else
        {
            // Envoyer périodiquement un sendVstEventsToHost(no_event) pour signifier
            // à Ableton Live qu'on est bien vivant et qu'on envoie du midi
            // (tous les 120 tours de buffer)
            nbFramesWithoutMidiOutEvent++;
            if (nbFramesWithoutMidiOutEvent>120)
            {
                nbFramesWithoutMidiOutEvent=0;
                midiEvnts[0].midiData[0]=0;
                midiEvnts[0].midiData[1]=0;
                midiEvnts[0].midiData[2]=0;
                evnts->numEvents=1;
                sendVstEventsToHost(evnts);
            }

        }

      ReleaseMutex(pdvstTransferMutex);
    }

    #endif // VSTMIDIOUTENABLE


       // Get play head information
      VstTimeInfo * infos = getTimeInfo(
      kVstTempoValid       |
      kVstPpqPosValid      |
      kVstTransportPlaying |
      kVstTransportChanged |
      kVstTransportCycleActive |
      kVstTransportRecording   |
      kVstAutomationWriting    |
      kVstAutomationReading    |
      0);

      if(infos)
        {
            if (WaitForSingleObject(pdvstTransferMutex, 10)==WAIT_OBJECT_0)
            {

                pdvstData->hostTimeInfo.updated=1;
                //copying vstTimeInfo into pdvst transfer data structure
                pdvstData->hostTimeInfo.samplePos=infos->samplePos;				///< current Position in audio samples (always valid)
                pdvstData->hostTimeInfo.sampleRate=infos->sampleRate;				///< current Sample Rate in Herz (always valid)
                pdvstData->hostTimeInfo.nanoSeconds=infos->nanoSeconds;				///< System Time in nanoseconds (10^-9 second)
                pdvstData->hostTimeInfo.ppqPos=infos->ppqPos;					///< Musical Position, in Quarter Note (1.0 equals 1 Quarter Note)
                pdvstData->hostTimeInfo.tempo=infos->tempo;					///< current Tempo in BPM (Beats Per Minute)
                pdvstData->hostTimeInfo.barStartPos=infos->barStartPos;				///< last Bar Start Position, in Quarter Note
                pdvstData->hostTimeInfo.cycleStartPos=infos->cycleStartPos;			///< Cycle Start (left locator), in Quarter Note
                pdvstData->hostTimeInfo.cycleEndPos=infos->cycleEndPos;				///< Cycle End (right locator), in Quarter Note
                pdvstData->hostTimeInfo.timeSigNumerator=(int)infos->timeSigNumerator;		///< Time Signature Numerator (e.g. 3 for 3/4)
                pdvstData->hostTimeInfo.timeSigDenominator=(int)infos->timeSigDenominator;	///< Time Signature Denominator (e.g. 4 for 3/4)
                pdvstData->hostTimeInfo.smpteOffset=(int)infos->smpteOffset;			///< SMPTE offset (in SMPTE subframes (bits; 1/80 of a frame)). The current SMPTE position can be calculated using #samplePos, #sampleRate, and #smpteFrameRate.
                pdvstData->hostTimeInfo.smpteFrameRate=(int)infos->smpteFrameRate;		///< @see VstSmpteFrameRate
                pdvstData->hostTimeInfo.samplesToNextClock=(int)infos->samplesToNextClock;	///< MIDI Clock Resolution (24 Per Quarter Note), can be negative (nearest clock)
                pdvstData->hostTimeInfo.flags=(int)infos->flags;

                ReleaseMutex(pdvstTransferMutex);
            }
        }

    int i, j, k, l;
    int framesOut = 0;

    if (!dspActive)
    {
        resume();
    }
    else
    {
        setSyncToVst(1);
    }


    for (i = 0; i < sampleFrames; i++)
    {
        for (j = 0; j < audioBuffer->nChannels; j++)
        {
            audioBuffer->in[j][audioBuffer->inFrameCount] = input[j][i];
        }
        (audioBuffer->inFrameCount)++;
        // if enough samples to process then do it
        if (audioBuffer->inFrameCount >= PDBLKSIZE)
        {
            audioBuffer->inFrameCount = 0;
            updatePdvstParameters();
            // wait for pd process event
            //   { JYG
            int gotPdProcEvent = (WaitForSingleObject(pdProcEvent, 10)==WAIT_OBJECT_0);
           // WaitForSingleObject(pdProcEvent,100);
            //WaitForSingleObject(pdProcEvent, 10000);
            // JYG: 10000 était une valeur trop élevée qui faisait planter ableton live
            // JYG }
            if (gotPdProcEvent)
                {
                    ResetEvent(pdProcEvent);
                    syncDefeatNumber=0;
                }
            else if (syncDefeatNumber==50)
            {
                //afficher messageErreur
                    //CreateThread(NULL, 0, &NotifySyncError, NULL, 0, NULL);
                    //startPd();
                    syncDefeatNumber++;

            }

                else
                    syncDefeatNumber++;

            for (k = 0; k < PDBLKSIZE; k++)
            {
                for (l = 0; l < audioBuffer->nChannels; l++)
                {
                    while (audioBuffer->outFrameCount >= audioBuffer->size)
                    {
                        audioBuffer->resize(audioBuffer->size * 2);
                    }
                    // get pd processed samples
                    if (gotPdProcEvent)
                    {
                    // get pd processed samples
                        audioBuffer->out[l][audioBuffer->outFrameCount] = pdvstData->samples[l][k];
                        // put new samples in for processing
                        pdvstData->samples[l][k] = audioBuffer->in[l][k];
                    }

                }
                (audioBuffer->outFrameCount)++;
            }
            pdvstData->sampleRate = (int)getSampleRate();
            // signal vst process event
            SetEvent(vstProcEvent);
        }
    }
    // output pd processed samples
    for (i = 0; i < sampleFrames; i++)
    {
        for (j = 0; j < audioBuffer->nChannels; j++)
        {
            if (audioBuffer->outFrameCount > 0)
            {
                output[j][i] = audioBuffer->out[j][i];
            }
            else
            {
                output[j][i] = 0;
            }
        }
        if (audioBuffer->outFrameCount > 0)
        {
            audioBuffer->outFrameCount--;
            framesOut++;
        }
    }
    // shift any remaining buffered out samples
    if (audioBuffer->outFrameCount > 0)
    {
        for (i = 0; i < audioBuffer->nChannels; i++)
        {
            memmove(&(audioBuffer->out[i][0]),
                    &(audioBuffer->out[i][framesOut]),
                    (audioBuffer->outFrameCount) * sizeof(float));
        }
    }

}

VstInt32 pdvst::processEvents(VstEvents* ev)
{
    VstMidiEvent* event;
    char* midiData;
    long statusType;
    long statusChannel;

       WaitForSingleObject(pdvstTransferMutex, 10);/// if (WaitForSingleObject(pdvstTransferMutex, 100)!= WAIT_TIMEOUT)
        // valeur d'attente courte (1000 au lieu de 10000 dans pdvst 0.2)

    {
        for (long i = 0; i < ev->numEvents; i++)
        {
            if ((ev->events[i])->type != kVstMidiType)
            {
                continue;
            }
            event = (VstMidiEvent*)ev->events[i];
            midiData = event->midiData;
            statusType = midiData[0] & 0xF0;
            statusChannel = midiData[0] & 0x0F;
            pdvstData->midiQueue[pdvstData->midiQueueSize].statusByte = midiData[0];
            pdvstData->midiQueue[pdvstData->midiQueueSize].dataByte1 = midiData[1];
            pdvstData->midiQueue[pdvstData->midiQueueSize].dataByte2 = midiData[2];
            if (statusType == 0x80)
            {
                // note off
                pdvstData->midiQueue[pdvstData->midiQueueSize].channelNumber = statusChannel;
                pdvstData->midiQueue[pdvstData->midiQueueSize].messageType = NOTE_OFF;
            }
            else if (statusType == 0x90)
            {
                // note on
                pdvstData->midiQueue[pdvstData->midiQueueSize].channelNumber = statusChannel;
                pdvstData->midiQueue[pdvstData->midiQueueSize].messageType = NOTE_ON;
            }
            else if (statusType == 0xA0)
            {
                // key pressure
                pdvstData->midiQueue[pdvstData->midiQueueSize].channelNumber = statusChannel;
                pdvstData->midiQueue[pdvstData->midiQueueSize].messageType = KEY_PRESSURE;
            }
            else if (statusType == 0xB0)
            {
                // controller change
                pdvstData->midiQueue[pdvstData->midiQueueSize].channelNumber = statusChannel;
                pdvstData->midiQueue[pdvstData->midiQueueSize].messageType = CONTROLLER_CHANGE;
            }
            else if (statusType == 0xC0)
            {
                // program change
                pdvstData->midiQueue[pdvstData->midiQueueSize].channelNumber = statusChannel;
                pdvstData->midiQueue[pdvstData->midiQueueSize].messageType = PROGRAM_CHANGE;
            }
            else if (statusType == 0xD0)
            {
                // channel pressure
                pdvstData->midiQueue[pdvstData->midiQueueSize].channelNumber = statusChannel;
                pdvstData->midiQueue[pdvstData->midiQueueSize].messageType = CHANNEL_PRESSURE;
            }
            else if (statusType == 0xE0)
            {
                // pitch bend
                pdvstData->midiQueue[pdvstData->midiQueueSize].channelNumber = statusChannel;
                pdvstData->midiQueue[pdvstData->midiQueueSize].messageType = PITCH_BEND;
            }
            else
            {
                // something else
                pdvstData->midiQueue[pdvstData->midiQueueSize].messageType = OTHER;
            }
            pdvstData->midiQueueSize++;
            event++;
        }
        pdvstData->midiQueueUpdated = 1;
        ReleaseMutex(pdvstTransferMutex);
        return 1;
    }
    ///else
        return 0;
}

void pdvst::updatePdvstParameters()
{
    int i;


    WaitForSingleObject(pdvstTransferMutex, 10);
   // WaitForSingleObject(pdvstTransferMutex, 10000);///if (WaitForSingleObject(pdvstTransferMutex, 100)!= WAIT_TIMEOUT)
    {
        for (i = 0; i < pdvstData->nParameters; i++)
        {
            if (pdvstData->vstParameters[i].direction == PD_SEND && \
                pdvstData->vstParameters[i].updated)
            {
                if (pdvstData->vstParameters[i].type == FLOAT_TYPE)
                {
                    //beginEdit (i);
                    setParameterAutomated(i,
                                          pdvstData->vstParameters[i].value.floatData);
                    //endEdit(i);
                    updateDisplay ();
                }
                pdvstData->vstParameters[i].updated = 0;
            }
        }

        if (pdvstData->guiName.direction == PD_SEND && \
            pdvstData->guiName.updated)
        {
            if (pdvstData->guiName.type == STRING_TYPE)
            {
                strcpy(guiName, pdvstData->guiName.value.stringData);

                pdvstData->guiName.updated=0;
                // signal to vesteditor that guiname is updated
                guiNameUpdated=true;
            }




        }

        ReleaseMutex(pdvstTransferMutex);
    }

}

pdVstBuffer::pdVstBuffer(int nChans)
{
    int i;

    nChannels = nChans;
    in = (float **)malloc(nChannels * sizeof(float *));
    out = (float **)malloc(nChannels * sizeof(float *));
    for (i = 0; i < nChannels; i++)
    {
        in[i] = (float *)calloc(DEFPDVSTBUFFERSIZE,
                                sizeof(float));
        out[i] = (float *)calloc(DEFPDVSTBUFFERSIZE,
                                 sizeof(float));
    }
    size = DEFPDVSTBUFFERSIZE;
}

pdVstBuffer::~pdVstBuffer()
{
    int i;

    for (i = 0; i < nChannels; i++)
    {
        free(in[i]);
        free(out[i]);
    }
}

void pdVstBuffer::resize(int newSize)
{
    int i;

    for (i = 0; i < nChannels; i++)
    {
        in[i] = (float *)realloc(in[i], newSize * sizeof(float));
        out[i] = (float *)realloc(out[i], newSize * sizeof(float));
    }
    size = newSize;
}


