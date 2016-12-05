# PDVST 0.4
This is the updated version of pdvst (vst 2.4, pd 0.47-1) based on initial work by Joseph Sarlo. PdVst is an extension that allows Pd patches (and pd.exe programm) to run as VST plugins (Windows 32/64 only).

Binaries are here : 

Source code available here : https://github.com/jyg/pure-data/tree/master/pdvst

# How does it work ?

PdVst consists of two main parts : 
* a vst-plugin (pdvst-template32.dll, pdvst-template64.dll) to place in your favorite vst folder.
* a custom external scheduler (vstschedlib.dll) to copy in the pure-data /bin folder

When a PdVst plugin is opened by the host application, a setup file (*.pdv) is read to determine
information about the plugin, such as the Pd patch file to use, the number of parameters, etc...  
An instance of Pd.exe is started and opens the Pd patch file whose name was found in the setup file.

# Installation

This new version doesn't need a custom version of pure-data. You can use 
your current pure-data installation (>= 0.47-1) with your favorite externals). 

* Copy vstschedlib.dll & pthreadGC-3.dll to \bin directory 
   of your current pure-data installation
     
* Copy your plugin .dll file (e.g. Pd_Gain.dll) to the vst plugins directory
   for the application (e.g. c:\Program Files (x86)\Steinberg\Cubase VST\VstPlugins\)

* New way : in the vst plugin directory, create a subdir with the name of your plugin
   (e.g. c:\Program Files (x86)\Steinberg\Cubase VST\VstPlugins\Pd_Gain\).
   This folder will contain the .pdv and .pd files associated to your plugin.
   
* Old way : create a pdvst folder in the directory that contains the 
   application (e.g. Reaper.exe) that you want to use PdVst with.
   (e.g. c:\Program Files\REAPER\pdvst\). This folder will contain the .pdv and .pd files, and
   optionnally a copy of pure-data application  (inside c:\Program Files\REAPER\pdvst\pd\).
   
   NOTE (advanced) : if you choose the new way, you can also include an standalone version of puredata in the vst directory, simply paste it in the subfolder .pd\
   (e.g. c:\Program Files (x86)\Steinberg\Cubase VST\VstPlugins\.pd\) 

# Creating VST Plugins from Pd Patches

* Create a new .pdv setup file (see the .pdv Setup File section). The file
   must be named the same as the plugin (e.g. for a plugin named Pd_Gain you
   would create a file named Pd_Gain.pdv). Place this file and all dependant
   files (.pd files, external Pd library .dll files, etc.) as specified above (§ "Installation").

* Make a copy of the pdvst-template32.dll / pdvst-template64.dll file and
   rename the same as the plugin and .pdv file (e.g. for a plugin named
   Pd_Gain you would copy pdvst-template32.dll to a new file named Pd_Gain.dll).
   Move your new plugin .dll file to the vst plugins folder of the application.
   
# The .pdv Setup File-

This file contains all of the information about your plugin. The format is ASCII  
text with keys and values separated by an '=' character and each key and value 
pair separated by a carriage return. Comments are demarked with a '#' character.
For an example, see Pd_Gain.pdv. 

  -Keys-

    CHANNELS = <integer>
    Number of audio input and output channels. Tested with 2 and 4. 
    Should work with larger values.
    
    PDPATH = <string>
    Path to the installation directory of pure-data program.
    (example : PDPATH = C:\Program Files (x86)\pd-0.47-1\  )
    If this line is not specified, for compatibility with older version, the program will first
    look for pd.exe in <host_exec_dir>\pdvst\pd\bin. 
    If there is not an existing pd installation in <host_exec_dir>\pdvst\, the program will then
    look for pd.exe in <VST_FOLDER>\.pd\ 

    MAIN = <string>
    The .pd file for Pd to open when the plugin is opened. 

    ID = <string[4]>
    The 4-character unique ID for the VST plugin. This is required by VST and 
    just needs to be a unique string of four characters. 

    SYNTH = <TRUE/FALSE>
    Boolean value stating whether this plugin is an instrument (VSTi) or an effect. 
    Both types of plugins can send/receive midi events.
    
    CUSTOMGUI = <TRUE/FALSE>
    Boolean value stating whether the Pd patch uses a custom GUI (e.g. GrIPD). 

    DEBUG = <TRUE/FALSE>
    Boolean value stating whether to display the Pd GUI when the plugin is opened. 

    PARAMETERS = <integer>
    Number of parameters the plugin uses (up to 128). 

    NAMEPARAMETER<integer> = <string>
    Display name for parameters. Used when CUSTOMGUI is false or the VST host doesn't 
    support custom editors. 

    PROGRAM = <string>
    Declares a new VST program and sets the name (up to 128 programs can be declared) 

    PARAMETER<integer> = <float>
    Defines the parameter values for the last declared program. <float> must be 
    between 0 and 1 inclusive. 

# Pd/VST audio/midi Communication

When the plugin is opened, the .pd patch file declared in the .pdv setup file's MAIN key 
will be opened. 

The Pd patch will receive its incoming audio stream from the adc~ object, 
and incoming MIDI data from the Pd objects notein, ctlin, and pgmin. 

Pd patches should output their audio stream to the dac~ object, 
and their midi stream to the noteout, ctlout, pgmout objects.

REMARKS : 
* The midi out feature is still experimental and not fully tested. It should work when host uses ASIO driver, more erratic with mmio. Midi out note on/off have been tested, other midi messages untested but should work.
* Inside puredata plugin, don't use "media/ASIO" or "media/standard" audio menu, you may crash pd & host.
* You can continue to use "media/midi settings" menu to select input and output midi devices independently from host.

# Pd/VST - further communications

For purposes such as GUI interaction and VST automation, your patch may need to communicate 
further with the VST host. Special Pd send/receive symbols can be used in your Pd patch. 
For an example, see the pd-gain.pd file.

* rvstparameter<integer> : Use this symbol to receive parameter values from the VST host. Values will be floats between 0 and 1 inclusive. 

* svstparameter<integer> : Use this symbol to send parameter values to the VST host. Values should be floats between 0 and 1 inclusive. 

* rvstopengui : Use this symbol to receive notification that the patch's GUI should be opened or closed. The value will be either 1 or 0. 
  
* rvstplugname : Use this symbol to receive plug & instance name from host
 
* vstTimeInfo (play head information support) : 

vstTimeInfo.ppqPos, vstTimeInfo.tempo, vstTimeInfo.timeSigNumerator, vstTimeInfo.timeSigDenominator, vstTimeInfo.flags are experimental receivers for getting time infos from host. Names should change in the future.

* EXPERIMENTAL
  guiName : use this symbol to send and signal to the host the name of the gui-extra window to embed (see Pd_Gain(gui) example).
  
Note: for most VST hosts, parameters for VST instruments are recorded as sysex data, so be 
sure to disable any MIDI message filtering in the VST host. 

#CURRENT FEATURES

* Windows x32 and x64 only
* Support for embedding external gui window into host
* multichannel audio in/out support
* integrated vst midi-in, experimental midi-out
* added play head information support (see examples)

#TODO

* optimize plugin loading
* use of chunks for preset saving
