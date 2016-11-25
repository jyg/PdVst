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

#ifndef __pdvstTransfer_H
#define __pdvstTransfer_H

#include <stdio.h>
#define VSTMIDIOUTENABLE
#define MAXCHANNELS 16
#define MAXPARAMETERS 128
#define MAXBLOCKSIZE 256
#define MAXSTRINGSIZE 4096
#define MAXMIDIQUEUESIZE 1024

#ifdef VSTMIDIOUTENABLE
    #define MAXMIDIOUTQUEUESIZE 1024
#endif // VSTMIDIOUTENABLE





typedef enum _pdvstParameterDataType
{
	FLOAT_TYPE,
	STRING_TYPE
} pdvstParameterDataType;

typedef enum _pdvstParameterState
{
	PD_SEND,
	PD_RECEIVE
} pdvstParameterState;

typedef enum _pdvstMidiMessageType
{
    NOTE_OFF,
    NOTE_ON,
    KEY_PRESSURE,
    CONTROLLER_CHANGE,
    PROGRAM_CHANGE,
    CHANNEL_PRESSURE,
    PITCH_BEND,
    OTHER
} pdvstMidiMessageType;

typedef union _pdvstParameterData
{
	float floatData;
	char stringData[MAXSTRINGSIZE];
} pdvstParameterData;

typedef struct _pdvstParameter
{
	int updated;
	pdvstParameterDataType type;
	pdvstParameterData value;
	pdvstParameterState direction;
} pdvstParameter;

typedef struct _pdvstMidiMessage
{
    pdvstMidiMessageType messageType;
    int channelNumber;
    char statusByte;
    char dataByte1;
    char dataByte2;
} pdvstMidiMessage;

typedef struct _vstTimeInfo
{
    int updated;
//-------------------------------------------------------------------------------------------------------
	double samplePos;				///< current Position in audio samples (always valid)
	double sampleRate;				///< current Sample Rate in Herz (always valid)
	double nanoSeconds;				///< System Time in nanoseconds (10^-9 second)
	double ppqPos;					///< Musical Position, in Quarter Note (1.0 equals 1 Quarter Note)
	double tempo;					///< current Tempo in BPM (Beats Per Minute)
	double barStartPos;				///< last Bar Start Position, in Quarter Note
	double cycleStartPos;			///< Cycle Start (left locator), in Quarter Note
	double cycleEndPos;				///< Cycle End (right locator), in Quarter Note
	int timeSigNumerator;		///< Time Signature Numerator (e.g. 3 for 3/4)
	int timeSigDenominator;	///< Time Signature Denominator (e.g. 4 for 3/4)
	int smpteOffset;			///< SMPTE offset (in SMPTE subframes (bits; 1/80 of a frame)). The current SMPTE position can be calculated using #samplePos, #sampleRate, and #smpteFrameRate.
	int smpteFrameRate;		///< @see VstSmpteFrameRate
	int samplesToNextClock;	///< MIDI Clock Resolution (24 Per Quarter Note), can be negative (nearest clock)
	int flags;					///< @see VstTimeInfoFlags
//-------------------------------------------------------------------------------------------------------
}pdvstTimeInfo;


typedef struct _pdvstTransferData
{
	int active;
	int syncToVst;
	int nChannels;
	int sampleRate;
	int blockSize;
	int nParameters;
    int midiQueueSize;
    int midiQueueUpdated;
	float samples[MAXCHANNELS][MAXBLOCKSIZE];
	pdvstParameter vstParameters[MAXPARAMETERS];
    pdvstMidiMessage midiQueue[MAXMIDIQUEUESIZE];
	pdvstParameter guiState;
	pdvstParameter plugName;  // transmitted by host
	pdvstParameter guiName;   // transmitted by pd : name of gui window to be embedded
    #ifdef VSTMIDIOUTENABLE
    int midiOutQueueSize;
    int midiOutQueueUpdated;
	pdvstMidiMessage midiOutQueue[MAXMIDIOUTQUEUESIZE];

    #endif // VSTMIDIOUTENABLE
    pdvstTimeInfo  hostTimeInfo;

} pdvstTransferData;

#endif
