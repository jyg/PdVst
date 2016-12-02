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
#include <windows.h>
#include <io.h>
#include <stdio.h>
#include "m_pd.h"
#include "s_stuff.h"
#include "../../../pdvstTransfer.h"
#define MAXARGS 1024
#define MAXSTRLEN 1024
#define TIMEUNITPERSEC (32.*441000.)

#define kVstTransportChanged      1
#define	kVstTransportPlaying      2

#define	kVstTransportRecording    8

#ifdef VSTMIDIOUTENABLE
    EXTERN int midi_outhead, midi_outtail;
    typedef struct _midiqelem
    {
        double q_time;
        int q_portno;
        unsigned char q_onebyte;
        unsigned char q_byte1;
        unsigned char q_byte2;
        unsigned char q_byte3;
    } t_midiqelem;

    #ifndef MIDIQSIZE
        #define MIDIQSIZE 1024
    #endif /*MIDIQSIZE  */

    EXTERN t_midiqelem midi_outqueue[MIDIQSIZE];
    //EXTERN int msw_nmidiout;
    EXTERN int msw_nmidiout=1;

#endif // VSTMIDIOUTENABLE


typedef struct _vstParameterReceiver
{
    t_object x_obj;
    t_symbol *x_sym;
}t_vstParameterReceiver;

typedef struct _vstGuiNameReceiver
{
    t_object x_obj;
}t_vstGuiNameReceiver;

t_vstGuiNameReceiver *vstGuiNameReceiver;

t_vstParameterReceiver *vstParameterReceivers[MAXPARAMETERS];

t_class *vstParameterReceiver_class;
t_class *vstGuiNameReceiver_class;

char *pdvstTransferMutexName,
     *pdvstTransferFileMapName,
     *vstProcEventName,
     *pdProcEventName;
pdvstTransferData *pdvstData;
HANDLE vstHostProcess,
       pdvstTransferMutex,
       pdvstTransferFileMap,
       vstProcEvent,
       pdProcEvent;
       pdvstTimeInfo  timeInfo;

int tokenizeCommandLineString(char *clString, char **tokens)
{
    int i, charCount = 0;
    int tokCount= 0;
    int quoteOpen = 0;

    for (i = 0; i < (signed int)strlen(clString); i++)
    {
        if (clString[i] == '"')
        {
            quoteOpen = !quoteOpen;
        }
        else if (clString[i] == ' ' && !quoteOpen)
        {
            tokens[tokCount][charCount] = 0;
            tokCount++;
            charCount = 0;
        }
        else
        {
            tokens[tokCount][charCount] = clString[i];
            charCount++;
        }
    }
    tokens[tokCount][charCount] = 0;
    tokCount++;
    return tokCount;
}

void parseArgs(int argc, char **argv)
{
    int vstHostProcessId;

    while ((argc > 0) && (**argv == '-'))
    {
        if (strcmp(*argv, "-vsthostid") == 0)
        {
            vstHostProcessId = atoi(argv[1]);
            vstHostProcess = OpenProcess(PROCESS_ALL_ACCESS,
                                         FALSE,
                                         vstHostProcessId);
            if (vstHostProcess == NULL)
            {
                exit(1);
            }
            argc -= 2;
            argv += 2;
        }
        if (strcmp(*argv, "-mutexname") == 0)
        {
            pdvstTransferMutexName = argv[1];
            argc -= 2;
            argv += 2;
        }
        if (strcmp(*argv, "-filemapname") == 0)
        {
            pdvstTransferFileMapName = argv[1];
            argc -= 2;
            argv += 2;
        }
        if (strcmp(*argv, "-vstproceventname") == 0)
        {
            vstProcEventName = argv[1];
            argc -= 2;
            argv += 2;
        }
        if (strcmp(*argv, "-pdproceventname") == 0)
        {
            pdProcEventName = argv[1];
            argc -= 2;
            argv += 2;
        }
        else
        {
            argc--;
            argv++;
        }
    }
}

int setPdvstGuiState(int state)
{
    t_symbol *tempSym;

    tempSym = gensym("rvstopengui");
    if (tempSym->s_thing)
    {
        pd_float(tempSym->s_thing, (float)state);
        return 1;
    }
    else
        return 0;
}


int setPdvstPlugName(char* instanceName)
{
    t_symbol *tempSym;
    tempSym = gensym("rvstplugname");
    if (tempSym->s_thing)
    {
        pd_symbol(tempSym->s_thing, gensym(instanceName));
        return 1;
    }
    else
        return 0;
}


int setPdvstFloatParameter(int index, float value)
{
    t_symbol *tempSym;
    char string[1024];

    sprintf(string, "rvstparameter%d", index);
    tempSym = gensym(string);
    if (tempSym->s_thing)
    {
        pd_float(tempSym->s_thing, value);
        return 1;
    }
    else
    {
        return 0;
    }
}

void sendPdVstFloatParameter(t_vstParameterReceiver *x, t_float floatValue)
{
    int index;

    index = atoi(x->x_sym->s_name + strlen("svstParameter"));
    WaitForSingleObject(pdvstTransferMutex, INFINITE);
    pdvstData->vstParameters[index].type = FLOAT_TYPE;
    pdvstData->vstParameters[index].direction = PD_SEND;
    pdvstData->vstParameters[index].updated = 1;
    pdvstData->vstParameters[index].value.floatData = floatValue;
    ReleaseMutex(pdvstTransferMutex);
}

void sendPdVstGuiName(t_vstGuiNameReceiver *x, t_symbol *symbolValue)
{
    WaitForSingleObject(pdvstTransferMutex, INFINITE);
    pdvstData->guiName.type = STRING_TYPE;
    pdvstData->guiName.direction = PD_SEND;
    pdvstData->guiName.updated = 1;
    strcpy(pdvstData->guiName.value.stringData,symbolValue->s_name);

    ReleaseMutex(pdvstTransferMutex);

}

void makePdvstParameterReceivers()
{
    int i;
    char string[1024];

    for (i = 0; i < MAXPARAMETERS; i++)
    {
        vstParameterReceivers[i] = (t_vstParameterReceiver *)pd_new(vstParameterReceiver_class);
        sprintf(string, "svstparameter%d", i);
        vstParameterReceivers[i]->x_sym = gensym(string);
        pd_bind(&vstParameterReceivers[i]->x_obj.ob_pd, gensym(string));
    }
}

void makePdvstGuiNameReceiver()
{
        vstGuiNameReceiver = (t_vstGuiNameReceiver *)pd_new(vstGuiNameReceiver_class);
        pd_bind(&vstGuiNameReceiver->x_obj.ob_pd, gensym("guiName"));

}



void send_dacs(void)
{
    int i, j, sampleCount, nChannels, blockSize;
    t_sample *soundin, *soundout;

    soundin = get_sys_soundin();
    soundout = get_sys_soundout();
    nChannels = pdvstData->nChannels;
    blockSize = pdvstData->blockSize;
    if (blockSize == *(get_sys_schedblocksize()))
    {
        sampleCount = 0;
        for (i = 0; i < nChannels; i++)
        {
            for (j = 0; j < blockSize; j++)
            {
                soundin[sampleCount] = pdvstData->samples[i][j];
                pdvstData->samples[i][j] = soundout[sampleCount];
                soundout[sampleCount] = 0;
                sampleCount++;
            }
        }
    }
}

void scheduler_tick( void)
{
    sys_addhist(0);
    send_dacs();
    sys_setmiditimediff(0, 1e-6 * *(get_sys_schedadvance()));
    sys_addhist(1);
    /// {  JYG
    //sched_tick(*(get_sys_time()) + *(get_sys_time_per_dsp_tick()));
    sched_tick();
    ///  JYG }
    sys_addhist(2);
    sys_pollmidiqueue();
    sys_pollgui();
    sys_addhist(3);
}
/*
int HFClock()
{
    LARGE_INTEGER freq, now;

    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&now);
    return (int)((now.QuadPart * 1000) / freq.QuadPart);
}
*/

int scheduler()
{
    int i,
        blockTime,

        active = 1;
    DWORD vstHostProcessStatus = 0;


    vstParameterReceiver_class = class_new(gensym("vstParameterReceiver"),
                                           0,
                                           0,
                                           sizeof(t_vstParameterReceiver),
                                           0,
                                           0);

    class_addfloat(vstParameterReceiver_class, (t_method)sendPdVstFloatParameter);
    makePdvstParameterReceivers();

    vstGuiNameReceiver_class = class_new(gensym("vstGuiNameReceiver"),
                                           0,
                                           0,
                                           sizeof(t_vstGuiNameReceiver),
                                           0,
                                           0);

    class_addsymbol(vstGuiNameReceiver_class,(t_method)sendPdVstGuiName);
    makePdvstGuiNameReceiver();

    *(get_sys_time_per_dsp_tick()) = (TIMEUNITPERSEC) * \
                                     ((double)*(get_sys_schedblocksize())) / \
                                     *(get_sys_dacsr());
    sys_clearhist();
    //if (*(get_sys_sleepgrain()) < 1000)   // JYG
    if (*(get_sys_sleepgrain()) < 100)
    {
        *(get_sys_sleepgrain()) = *(get_sys_schedadvance()) / 4;
    }
    if (*(get_sys_sleepgrain()) < 100)
    {
        *(get_sys_sleepgrain()) = 100;
    }
    else if (*(get_sys_sleepgrain()) > 5000)
    {
        *(get_sys_sleepgrain()) = 5000;
    }
    sys_initmidiqueue();

    #ifdef VSTMIDIOUTENABLE
    pdvstData->midiOutQueueSize=0;
    #endif // VSTMIDIOUTENABLE
    while (active)
    {
        WaitForSingleObject(pdvstTransferMutex, INFINITE);
        active = pdvstData->active;
        // check sample rate
        if (pdvstData->sampleRate != (int)sys_getsr())
        {
           post("check samplerate");
            sys_setchsr(pdvstData->nChannels,
                        pdvstData->nChannels,
                        pdvstData->sampleRate);
        }
        // check for vstParameter changed
        if (pdvstData->guiState.direction == PD_RECEIVE && \
            pdvstData->guiState.updated)
        {
            if(setPdvstGuiState((int)pdvstData->guiState.value.floatData))
                pdvstData->guiState.updated=0;
        }
                // JYG {  check for vstplug instance name
        if (pdvstData->plugName.direction == PD_RECEIVE && \
            pdvstData->plugName.updated)
        {
             if (setPdvstPlugName((char*)pdvstData->plugName.value.stringData))
                pdvstData->plugName.updated=0;
        }

        if (pdvstData->hostTimeInfo.updated)
        {
            pdvstData->hostTimeInfo.updated=0;
            t_symbol *tempSym;

            if (timeInfo.flags!=pdvstData->hostTimeInfo.flags)
            {
                timeInfo.flags=pdvstData->hostTimeInfo.flags;

                tempSym = gensym("vstTimeInfo.flags");
                if (tempSym->s_thing)
                {
                    pd_float(tempSym->s_thing, (float)timeInfo.flags);
                }
                else
                {
                    timeInfo.flags=0;
                    pdvstData->hostTimeInfo.updated=1;  // keep flag as updated
                 }

            }

            if ((timeInfo.flags&(kVstTransportChanged|kVstTransportPlaying|kVstTransportRecording))||(timeInfo.ppqPos!=(float)pdvstData->hostTimeInfo.ppqPos))
            {
                timeInfo.ppqPos=(float)pdvstData->hostTimeInfo.ppqPos;

                tempSym = gensym("vstTimeInfo.ppqPos");
                if (tempSym->s_thing)
                {
                    pd_float(tempSym->s_thing, timeInfo.ppqPos);
                }
                 else
                {
                    timeInfo.ppqPos=0.;
                    pdvstData->hostTimeInfo.updated=1;  // keep flag as updated
                 }
            }
            if ((timeInfo.flags&kVstTransportChanged)|(timeInfo.tempo!=(float)pdvstData->hostTimeInfo.tempo))
            {
                timeInfo.tempo=(float)pdvstData->hostTimeInfo.tempo;

                tempSym = gensym("vstTimeInfo.tempo");
                if (tempSym->s_thing)
                {
                    pd_float(tempSym->s_thing, timeInfo.tempo);
                }
                 else
                 {
                    timeInfo.tempo=0.;
                    pdvstData->hostTimeInfo.updated=1;  // keep flag as updated
                 }
            }

            if ((timeInfo.flags&kVstTransportChanged)|timeInfo.timeSigNumerator!=pdvstData->hostTimeInfo.timeSigNumerator)
            {
                timeInfo.timeSigNumerator=pdvstData->hostTimeInfo.timeSigNumerator;

                tempSym = gensym("vstTimeInfo.timeSigNumerator");
                if (tempSym->s_thing)
                {
                    pd_float(tempSym->s_thing, (float)timeInfo.timeSigNumerator);
                }
                 else
                {
                   timeInfo.timeSigNumerator=0;
                    pdvstData->hostTimeInfo.updated=1;  // keep flag as updated
                 }
            }
            if ((timeInfo.flags&kVstTransportChanged)| timeInfo.timeSigDenominator!=pdvstData->hostTimeInfo.timeSigDenominator)
            {
                timeInfo.timeSigDenominator=pdvstData->hostTimeInfo.timeSigDenominator;

                tempSym = gensym("vstTimeInfo.timeSigDenominator");
                if (tempSym->s_thing)
                {
                    pd_float(tempSym->s_thing, (float)timeInfo.timeSigDenominator);
                }
                 else
                {
                   timeInfo.timeSigDenominator=0;
                    pdvstData->hostTimeInfo.updated=1;  // keep flag as updated
                 }
            }

        }



            // JYG }
        for (i = 0; i < pdvstData->nParameters; i++)
        {
            if (pdvstData->vstParameters[i].direction == PD_RECEIVE && \
                pdvstData->vstParameters[i].updated)
            {
                if (pdvstData->vstParameters[i].type == FLOAT_TYPE)
                {
                    if (setPdvstFloatParameter(i,
                                  pdvstData->vstParameters[i].value.floatData))
                    {
                        pdvstData->vstParameters[i].updated = 0;
                    }
                }
            }
        }
        // check for new midi-in message (VSTi)
        if (pdvstData->midiQueueUpdated)
        {
            for (i = 0; i < pdvstData->midiQueueSize; i++)
            {
                if (pdvstData->midiQueue[i].messageType == NOTE_ON)
                {
                    inmidi_noteon(0,
                                  pdvstData->midiQueue[i].channelNumber,
                                  pdvstData->midiQueue[i].dataByte1,
                                  pdvstData->midiQueue[i].dataByte2);
                }
                else if (pdvstData->midiQueue[i].messageType == NOTE_OFF)
                {
                    inmidi_noteon(0,
                                  pdvstData->midiQueue[i].channelNumber,
                                  pdvstData->midiQueue[i].dataByte1,
                                  0);
                }
                else if (pdvstData->midiQueue[i].messageType == CONTROLLER_CHANGE)
                {
                    inmidi_controlchange(0,
                                         pdvstData->midiQueue[i].channelNumber,
                                         pdvstData->midiQueue[i].dataByte1,
                                         pdvstData->midiQueue[i].dataByte2);
                }
                else if (pdvstData->midiQueue[i].messageType == PROGRAM_CHANGE)
                {
                    inmidi_programchange(0,
                                         pdvstData->midiQueue[i].channelNumber,
                                         pdvstData->midiQueue[i].dataByte1);
                }
                else if (pdvstData->midiQueue[i].messageType == PITCH_BEND)
                {
                    int value = (((int)pdvstData->midiQueue[i].dataByte2) * 16) + \
                                (int)pdvstData->midiQueue[i].dataByte1;

                    inmidi_pitchbend(0,
                                     pdvstData->midiQueue[i].channelNumber,
                                     value);
                }
                else if (pdvstData->midiQueue[i].messageType == CHANNEL_PRESSURE)
                {
                    inmidi_aftertouch(0,
                                      pdvstData->midiQueue[i].channelNumber,
                                      pdvstData->midiQueue[i].dataByte1);
                }
                else if (pdvstData->midiQueue[i].messageType == KEY_PRESSURE)
                {
                    inmidi_polyaftertouch(0,
                                          pdvstData->midiQueue[i].channelNumber,
                                          pdvstData->midiQueue[i].dataByte1,
                                          pdvstData->midiQueue[i].dataByte2);
                }
                else if (pdvstData->midiQueue[i].messageType == OTHER)
                {
                    // FIXME: what to do?
                }
                else
                {
                   post("pdvstData->midiQueue error"); // FIXME: error?
                }
            }
            pdvstData->midiQueueSize = 0;
            pdvstData->midiQueueUpdated = 0;
        }

        /// flush vstmidi out messages here
#ifdef VSTMIDIOUTENABLE
        if (msw_nmidiout==0)
        {
            int i=pdvstData->midiOutQueueSize;    // si la midiOutQueue n'a pas été vidée, rajouter des éléments à la fin de celle-ci
            while (midi_outhead != midi_outtail)
            {
               int statusType = midi_outqueue[midi_outtail].q_byte1 & 0xF0;
               int statusChannel = midi_outqueue[midi_outtail].q_byte1 & 0x0F;

                // faut il gérer midi_outqueue[midi_outtail].q_onebyte  ?

                ///copie de la pile midi_outqueue dans la pile vst midiOutQueue
                pdvstData->midiOutQueue[i].channelNumber= statusChannel;
                pdvstData->midiOutQueue[i].statusByte = midi_outqueue[midi_outtail].q_byte1;
                pdvstData->midiOutQueue[i].dataByte1=  midi_outqueue[midi_outtail].q_byte2;
                pdvstData->midiOutQueue[i].dataByte2= midi_outqueue[midi_outtail].q_byte3;

                if (statusType == 0x90)
                    if (midi_outqueue[midi_outtail].q_byte3==0)
                    // note off
                    {
                        pdvstData->midiOutQueue[i].messageType = NOTE_OFF;
                        pdvstData->midiOutQueue[i].statusByte = statusChannel|0x80;
                    //    post("note_off[%d] : %d",i,midi_outqueue[midi_outtail].q_byte2);

                    }

                    else
                    {
                        // note on
                        pdvstData->midiOutQueue[i].messageType = NOTE_ON;
                    //post("note_on[%d] : %d",i,midi_outqueue[midi_outtail].q_byte2);

                    }
                else if (statusType == 0xA0)
                    // key pressure
                    pdvstData->midiOutQueue[i].messageType  = KEY_PRESSURE;
                else if (statusType == 0xB0)
                    // controller change
                    pdvstData->midiOutQueue[i].messageType  = CONTROLLER_CHANGE;
                else if (statusType == 0xC0)
                    // program change
                    pdvstData->midiOutQueue[i].messageType = PROGRAM_CHANGE;
                else if (statusType == 0xD0)
                    // channel pressure
                    pdvstData->midiOutQueue[i].messageType = CHANNEL_PRESSURE;
                else if (statusType == 0xE0)
                    // pitch bend
                    pdvstData->midiOutQueue[i].messageType = PITCH_BEND;
                else
                    // something else
                    pdvstData->midiOutQueue[i].messageType  = OTHER;


                midi_outtail  = (midi_outtail + 1 == MIDIQSIZE ? 0 : midi_outtail + 1);
                i  = i + 1;
                if (i>= MAXMIDIOUTQUEUESIZE)
                    break;

            }
                if (i>0)
                {
                    pdvstData->midiOutQueueSize=i;
                    pdvstData->midiOutQueueUpdated=1;
                }
                else
                {
                    pdvstData->midiOutQueueSize=0;
                    pdvstData->midiOutQueueUpdated=0;
                }
        }
#endif  // VSTMIDIOUTENABLE

        // run at approx. real-time
        blockTime = (int)((float)(pdvstData->blockSize) / \
                          (float)pdvstData->sampleRate * 1000.0);
        if (blockTime < 1)
        {
            blockTime = 1;
        }
        if (pdvstData->syncToVst)
        {
            ReleaseMutex(pdvstTransferMutex);
            if (WaitForSingleObject(vstProcEvent, 1000) == WAIT_TIMEOUT)
            {
                // we have probably lost sync by now (1 sec)
                WaitForSingleObject(pdvstTransferMutex, 100);
                pdvstData->syncToVst = 0;
                ReleaseMutex(pdvstTransferMutex);
            }
            ResetEvent(vstProcEvent);
            scheduler_tick();
            SetEvent(pdProcEvent);
        }
        else
        {
            ReleaseMutex(pdvstTransferMutex);
            scheduler_tick();
            Sleep(blockTime);
        }
        GetExitCodeProcess(vstHostProcess, &vstHostProcessStatus);
        if (vstHostProcessStatus != STILL_ACTIVE)
        {
            active = 0;
        }
    }
    return 1;
}

int pd_extern_sched(char *flags)
{
    int i, argc;
    char *argv[MAXARGS];

    for (i = 0; i < MAXARGS; i++)
    {
        argv[i] = (char *)malloc(MAXSTRLEN * sizeof(char));
    }
    argc = tokenizeCommandLineString(flags, argv);
    parseArgs(argc, argv);
    pdvstTransferMutex = OpenMutex(MUTEX_ALL_ACCESS, 0, pdvstTransferMutexName);
    vstProcEvent = OpenEvent(EVENT_ALL_ACCESS, 0, vstProcEventName);
    pdProcEvent = OpenEvent(EVENT_ALL_ACCESS, 0, pdProcEventName);
    pdvstTransferFileMap = OpenFileMapping(FILE_MAP_ALL_ACCESS,
                                           0,
                                           pdvstTransferFileMapName);
    pdvstData = (pdvstTransferData *)MapViewOfFile(pdvstTransferFileMap,
                                                   FILE_MAP_ALL_ACCESS,
                                                   0,
                                                   0,
                                                   sizeof(pdvstTransferData));
    WaitForSingleObject(pdvstTransferMutex, INFINITE);
    *(get_sys_schedadvance()) = *(get_sys_main_advance()) * 1000;
    post("appel sys_setchsr dans _main");
    sys_setchsr(pdvstData->nChannels,
                pdvstData->nChannels,
                pdvstData->sampleRate);
    ReleaseMutex(pdvstTransferMutex);
    timeBeginPeriod(1);
    scheduler();
    UnmapViewOfFile(pdvstData);
    CloseHandle(pdvstTransferFileMap);
    CloseHandle(pdvstTransferMutex);
    for (i = 0; i < MAXARGS; i++)
    {
        free(argv[i]);
    }
    return (0);
}







