/* Copyright 2008 Miller Puckette.  Berkeley license; see the
file LICENSE.txt in this distribution. */

/* A plug-in scheduler that turns Pd into a filter that inputs and
outputs audio and messages. */

/* todo:
    fix schedlib code to use extent2
    figure out about  if (sys_externalschedlib) { return; } in s_audio.c
    make buffer size ynamically growable

*/
#define EXT_MIDI_SUPPORT
//#define NO_OUTPUT
#include "../../src/m_pd.h"
#include "../../src/s_stuff.h"
#include "../../src/m_imp.h"
#include <stdio.h>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

#include "binarymsg.c"

#if defined(__linux__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__)\
     || defined(__GNU__)
void glob_watchdog(t_pd *dummy);

static void pollwatchdog( void)
{
    static int sched_diddsp, sched_nextpingtime;
    sched_diddsp++;
    if (!sys_havegui() && sys_hipriority &&
        (sched_diddsp - sched_nextpingtime > 0))
    {
        glob_watchdog(0);
            /* ping every 2 seconds */
        sched_nextpingtime = sched_diddsp +
            2 * (int)(STUFF->st_dacsr /(double)STUFF->st_schedblocksize);
    }
}
#endif

static t_class *pd_ambinary_class;
#define BUFSIZE 65536
static char *ascii_inbuf;

static int readasciimessage(t_binbuf *b)
{
    int fill = 0, c;

    binbuf_clear(b);
    while ((c = getchar()) != EOF)
    {
        if (c == ';')
        {
            binbuf_text(b, ascii_inbuf, fill);
            return (1);
        }
        else if (fill < BUFSIZE)
            ascii_inbuf[fill++] = c;
        else if (fill == BUFSIZE)
            fprintf(stderr, "pd-extern: input buffer overflow\n");
    }
    return (0);
}

static int readbinmessage(t_binbuf *b)
{
    binbuf_clear(b);
    while (1)
    {
        t_atom at;
        if (!pd_tilde_getatom(&at, stdin))
        {
            fprintf(stderr, "pd-extern: EOF on input\n");
            return (0);
        }
        else if (at.a_type == A_SEMI)
        {
      //   fprintf(stderr, "A_SEMI\n");
         return (1);
        }

        else

                binbuf_add(b, 1, &at);

    }
}

int pd_extern_sched(char *flags)
{
     post("Bienvenue dans pdsched");
     post("Entrée dans pd_extern_sched");
    int naudioindev, audioindev[MAXAUDIOINDEV], chindev[MAXAUDIOINDEV];
    int naudiooutdev, audiooutdev[MAXAUDIOOUTDEV], choutdev[MAXAUDIOOUTDEV];
    int i, j, rate, advance, callback, chin, chout, fill = 0, c, blocksize,
        useascii = 0;
    t_binbuf *b = binbuf_new();

    sys_get_audio_params(&naudioindev, audioindev, chindev,
        &naudiooutdev, audiooutdev, choutdev, &rate, &advance, &callback,
            &blocksize);

    chin = (naudioindev < 1 ? 0 : chindev[0]);
    chout = (naudiooutdev < 1 ? 0 : choutdev[0]);
     post("naudioin=%d naudioout=%d",chin,chout);
     post("advance=%d",advance);
    if (!flags || flags[0] != 'a')
    {
        post("mode binary");
            /* signal to stdout object to do binary by attaching an object
            to an obscure symbol name */
        pd_ambinary_class = class_new(gensym("pd~"), 0, 0, sizeof(t_pd),
            CLASS_PD, 0);
        pd_bind(&pd_ambinary_class, gensym("#pd_binary_stdio"));
            /* On Windows, set stdin and out to "binary" mode */
#ifdef _WIN32
        setmode(fileno(stdout),O_BINARY);
        setmode(fileno(stdin),O_BINARY);
#endif
    }
    else
    {
        if (!(ascii_inbuf = getbytes(BUFSIZE)))
            return (1);
        useascii = 1;
    }
    /* fprintf(stderr, "Pd plug-in scheduler called, chans %d %d, sr %d\n",
        chin, chout, (int)rate); */
    post("appel de sys_setchsr....");
    sys_setchsr(chin, chout, rate);
    post("done.");
    sys_audioapi = API_NONE;
    while (useascii ? readasciimessage(b) : readbinmessage(b) )
    {
        t_atom *ap = binbuf_getvec(b);
        int n = binbuf_getnatom(b);
        if (n > 0 && ap[0].a_type == A_FLOAT)
        {
          //  fprintf(stderr, "A_FLOAT, n_atom=%d sample=%f\n",n,atom_getfloat(ap));
 #ifdef EXT_MIDI_SUPPORT

          /// add external device midi support
            sys_setmiditimediff(0, 1e-6 * sys_schedadvance);
            sys_addhist(1);
            /// /////////////////
 #endif // EXT_MIDI_SUPPORT


            /* a list -- take it as incoming signals. */
            int chan, nchan = n/DEFDACBLKSIZE;
            t_sample *fp;
            for (i = chan = 0, fp = STUFF->st_soundin; chan < nchan; chan++)
                for (j = 0; j < DEFDACBLKSIZE; j++)
                    *fp++ = atom_getfloat(ap++);
            for (; chan < chin; chan++)
                for (j = 0; j < DEFDACBLKSIZE; j++)
                    *fp++ = 0;
            sched_tick();
#ifdef EXT_MIDI_SUPPORT

          /// add external device midi support
            sys_addhist(2);
            sys_pollmidiqueue();
            /// ///////////////////

#endif // EXT_MIDI_SUPPORT

            sys_pollgui();
#ifdef EXT_MIDI_SUPPORT


            /// add external device midi support
            sys_addhist(3); ///
#endif // EXT_MIDI_SUPPORT

#if defined(__linux__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__)\
     || defined(__GNU__)
            pollwatchdog();
#endif
#ifndef NO_OUTPUT
           if (useascii)
                printf(";\n");
            else putchar(A_SEMI);
            for (i = chout*DEFDACBLKSIZE, fp = STUFF->st_soundout; i--;
                fp++)
            {
                if (useascii)
                    printf("%g\n", *fp);
                else pd_tilde_putfloat(*fp, stdout);
                *fp = 0;
            }
            if (useascii)
                printf(";\n");
            else putchar(A_SEMI);
            fflush(stdout);
        }
        else if (n > 1 && ap[0].a_type == A_SYMBOL)
        {
            t_pd *whom = ap[0].a_w.w_symbol->s_thing;
            if (!whom)
                error("%s: no such object", ap[0].a_w.w_symbol->s_name);
            else if (ap[1].a_type == A_SYMBOL)
                typedmess(whom, ap[1].a_w.w_symbol, n-2, ap+2);
            else pd_list(whom, 0, n-1, ap+1);
  #endif
        }
    }
    binbuf_free(b);
    return (0);
}
