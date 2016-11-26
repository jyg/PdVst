# PDVST 
Revival of J.Sarlo's pdvst project

for using pd + externals + native guis  as vst plugin

binaries are here : https://sourceforge.net/projects/jygsdownloads/files/pdvst/

# How does it work ?

The pdvst project has two part : 
1) a vst-plugin (pdvst-template.dll), compiled within code:blocks with project file pdvst-template.cbp
2) a custom version of puredata with external scheduler (vstschedlib.dll), compiled with modified makefile, inside subfolder "pure-data" 
(https://github.com/jyg/pure-data/tree/master/pdvst/pure-data)

#CURRENT FEATURES

-windows x32 and x64 only

-multichannel audio in/out support

-integrated vst midi-in, experimental midi-out (v 0.0.33)

#TODO

-proper github project integration

-(re)enable midi out support via midi device menu

-add play head information support

-optimize plugin loading

-use of chunks for preset saving
