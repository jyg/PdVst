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
#include <windows.h>
#include <math.h>
#include "pdvst.hpp"

static AudioEffect *effect = 0;
bool oome = false;
//HINSTANCE hInstance;
bool globalIsASynth = false;
bool globalDebug = false;
int globalNChannels = 0;
int globalNPrograms = 0;
int globalNParams = 0;
int globalNExternalLibs = 0;
long globalPluginId = 'pdvp';
char globalExternalLib[MAXEXTERNS][MAXSTRLEN];
char globalVstParamName[MAXPARAMS][MAXSTRLEN];
char globalPluginPath[MAXFILENAMELEN];
char globalPluginName[MAXSTRLEN];
char globalPdFile[MAXFILENAMELEN];
char globalPureDataPath[MAXFILENAMELEN];
bool globalCustomGui = false;
bool globalVstEditWindowHide = true;
pdvstProgram globalProgram[MAXPROGRAMS];

char *trimWhitespace(char *str);
void parseSetupFile();


#include "audioeffect.h"

//------------------------------------------------------------------------
/** Must be implemented externally. */
extern AudioEffect* createEffectInstance (audioMasterCallback audioMaster);

extern "C" {
#if defined (__GNUC__) && ((__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1)))
	#define VST_EXPORT	__attribute__ ((visibility ("default")))
#else
	#define VST_EXPORT
#endif

VST_EXPORT AEffect *VSTPluginMain(audioMasterCallback audioMaster)
{
	// get vst version
	if (!audioMaster(0, audioMasterVersion, 0, 0, 0, 0))
        return 0;  // old version

// Create the AudioEffect
	AudioEffect* effect = createEffectInstance (audioMaster);
	if (!effect)
		return 0;

	return effect->getAeffect();
}

// support for old hosts not looking for VSTPluginMain
#if (TARGET_API_MAC_CARBON && __ppc__)
VST_EXPORT AEffect* main_macho (audioMasterCallback audioMaster) { return VSTPluginMain (audioMaster); }
#elif WIN32
VST_EXPORT AEffect* MAIN (audioMasterCallback audioMaster) { return VSTPluginMain (audioMaster); }
#elif BEOS
VST_EXPORT AEffect* main_plugin (audioMasterCallback audioMaster) { return VSTPluginMain (audioMaster); }
#endif

} // extern "C"




#if WIN32
#include <windows.h>
void* hInstance;

extern "C" {

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved)
{
    hInstance = hInst;
    parseSetupFile();
    return 1;
}
} // extern "C"
#endif


char *trimWhitespace(char *str)
{
    char *buf;

    if (strlen(str) > 0)
    {
        buf = str;
        while (isspace(*buf) && (buf - str) <= (int)strlen(str))
            buf++;
        memmove(str, buf, (strlen(buf) + 1) * sizeof(char));
        if (strlen(str) > 0)
        {
            buf = str + strlen(str) - 1;
            while (isspace(*buf) && (buf >= str))
            {
                *buf = 0;
                buf--;
            }
        }
    }
    return (str);
}

void parseSetupFile()
{
    FILE *setupFile;
    char setupFileName[MAXFILENAMELEN];
    char tFileName[MAXFILENAMELEN];
    char line[MAXSTRLEN];
    char param[MAXSTRLEN];
    char value[MAXSTRLEN];
    int i, equalPos, progNum = -1;

    // find filepaths
    GetModuleFileName((HMODULE)hInstance,
					  (LPTSTR)tFileName,
					  (DWORD)MAXFILENAMELEN);
    if (strrchr(tFileName, '\\'))
    {
        strcpy(globalPluginName, strrchr(tFileName, '\\') + 1);
    }
    else
    {
        strcpy(globalPluginName, tFileName);
    }
    GetModuleFileName(NULL,
					  (LPTSTR)tFileName,
					  (DWORD)MAXFILENAMELEN);
    if (strrchr(tFileName, '\\'))
    {
        strcpy(globalPluginPath, tFileName);
		*(strrchr(globalPluginPath, '\\') + 1) = 0;
		sprintf(globalPluginPath, "%spdvst\\", globalPluginPath);
    }
    else
    {
        strcpy(globalPluginPath, tFileName);
    }
    // remove .dll extension
    if (strstr(strlwr(globalPluginName), ".dll"))
        *(strstr(strlwr(globalPluginName), ".dll")) = 0;
    sprintf(setupFileName,
            "%s%s%s",
            globalPluginPath,
            globalPluginName,
            SETUPFILEEXT);
    // initialize program info
    strcpy(globalProgram[0].name, "Default");
    memset(globalProgram[0].paramValue, 0, MAXPARAMS * sizeof(float));
    // initialize parameter info
    globalNParams = 0;
    for (i = 0; i < MAXPARAMS; i++)
        strcpy(globalVstParamName[i], "<unnamed>");
    globalNPrograms = 1;
    // parse the setup file
    setupFile = fopen(setupFileName, "r");
    if (setupFile) {
        while (fgets(line, sizeof(line), setupFile))
        {
            equalPos = strchr(line, '=') - line;
            if (equalPos > 0 && line[0] != '#')
            {
                strcpy(param, line);
                param[equalPos] = 0;
                strcpy(value, line + equalPos + 1);
                strcpy(param, trimWhitespace(strlwr(param)));
                strcpy(value, trimWhitespace(value));
                // number of channels
                if (strcmp(param, "channels") == 0)
                    globalNChannels = atoi(value);

                // main PD patch
                if (strcmp(param, "main") == 0)
                {
                     strcpy(globalPdFile, strlwr(value));

                }
                if (strcmp(param, "pdpath") == 0)
                 {
                     strcpy(globalPureDataPath, strlwr(value));

                }
                // vst plugin ID
                if (strcmp(param, "id") == 0)
                {
                    globalPluginId = 0;
                    for (i = 0; i < 4; i++)
                        globalPluginId += (long)pow((double)16,(int) (i * 2)) * value[3 - i];
                }
                // is vst instrument
                if (strcmp(param, "synth") == 0)
                {
                    if (strcmp(strlwr(value), "true") == 0)
                    {
                        globalIsASynth = true;
                    }
                    else if (strcmp(strlwr(value), "false") == 0)
                    {
                        globalIsASynth = false;
                    }
                }
                // external libraries
                if (strcmp(param, "lib") == 0)
                {
                    while (strlen(value) > 0)
                    {
                        if (strchr(value, ',') == NULL)
                        {
                            strcpy(globalExternalLib[globalNExternalLibs], value);
                            value[0] = 0;
                        }
                        else
                        {
                            int commaIndex = strchr(value, ',') - value;
                            strncpy(globalExternalLib[globalNExternalLibs],
                                    value,
                                    commaIndex);
                            memmove(value,
                                    value + commaIndex + 1,
                                    (strlen(value) - commaIndex) * sizeof(char));
                            strcpy(value, trimWhitespace(value));
                        }
                        globalNExternalLibs++;
                    }
                }
                // has custom gui
                if (strcmp(param, "customgui") == 0)
                {
                    if (strcmp(strlwr(value), "true") == 0)
                    {
                        globalCustomGui = true;
                        // As we don't know how to embed any pd-tk-window inside vst-editor HWND,
                        // we use some trickery (hide vst-editor, popup pd-patch window on top)
                        globalVstEditWindowHide = true;
                    }
                    else if (strcmp(strlwr(value), "false") == 0)
                    {
                        globalCustomGui = false;
                        globalVstEditWindowHide = false;
                    }
                    else if (strcmp(strlwr(value), "legacy") == 0)
                           // With some vst-hosts the native vst-editor has to be used
                    {
                        globalCustomGui = true;
                        globalVstEditWindowHide = false;
                    }

                }
				// debug (show Pd GUI)
				if (strcmp(param, "debug") == 0)
                {
                    if (strcmp(strlwr(value), "true") == 0)
                    {
                        globalDebug = true;
                    }
                    else if (strcmp(strlwr(value), "false") == 0)
                    {
                        globalDebug = false;
                    }
                }
                // number of parameters
                if (strcmp(param, "parameters") == 0)
                {
                    int numParams = atoi(value);

                    if (numParams >= 0 && numParams < MAXPARAMS)
                        globalNParams = numParams;
                }
                // parameters names
                if (strstr(param, "nameparameter") == \
                        param && globalNPrograms < MAXPARAMS)
                {
                    int paramNum = atoi(param + strlen("nameparameter"));

                    if (paramNum < MAXPARAMS && paramNum >= 0)
                        strcpy(globalVstParamName[paramNum], value);
                }
                // program name
                if (strcmp(param, "program") == 0 && \
                    globalNPrograms < MAXPROGRAMS)
                {
                    progNum++;
                    strcpy(globalProgram[progNum].name, value);
                    globalNPrograms = progNum + 1;
                }
                // program parameters
                if (strstr(param, "parameter") == \
                    param && globalNPrograms < MAXPROGRAMS &&
                    !isalpha(param[strlen("parameter")]))
                {
                    int paramNum = atoi(param + strlen("parameter"));

                    if (paramNum < MAXPARAMS && paramNum >= 0)
                        globalProgram[progNum].paramValue[paramNum] = \
                                                             (float)atof(value);
                }
            }
        }
    }
    fclose(setupFile);
}
