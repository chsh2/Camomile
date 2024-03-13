/* 
helmholtz~, Pure Data class interfacing to class Helmholtz, a time domain
period length tracker. Class Helmholtz uses the Specially Normalized Autocorrelation
function (SNAC), invented by Philip Mcleod.

Based on Pure Data by Miller Puckette and others. Licensed under three-clause BSD
license.

Katja Vetter, Feb 2012.

*/
#include "m_pd.h"
#include "Helmholtz.h"

#define DEFFIDELITY 0.95    // default fidelity threshold for reporting pitch result
#define DEFMAXFREQ 1000.    // default maximum frequency for reporting pitch result

#if defined MSW                                         // when compiling for Windows	
#define EXPORT __declspec(dllexport)
#elif __GNUC__ >= 4										// else, when compiling with GCC 4.0 or higher
#define EXPORT __attribute__((visibility("default")))	
#endif
#ifndef EXPORT 
#define EXPORT											// empty definition for other cases
#endif


t_class *helmholtz_class;

typedef struct
{
    t_object x_obj;
    t_float f;
    t_clock *periodclock;
    t_outlet *periodoutlet;
    t_outlet *fidelityoutlet;
    t_float freq;
    t_float fidelity;
    t_float fidelitythreshold;
    t_float samplerate;
    t_float maxfreq;
    Helmholtz *phelmholtz;
}t_helmholtz;


static void helmholtz_tick(t_helmholtz *x)
{
    outlet_float(x->periodoutlet, x->freq);
    outlet_float(x->fidelityoutlet, x->fidelity);
}


static t_int *helmholtz_perform(t_int *w)
{
    t_helmholtz *x = (t_helmholtz*)w[1];
    t_sample *in = (t_sample*)w[2];
    t_sample *out = (t_sample*)w[3];
    t_int blocksize = (int)w[4];
    t_float freq, fidelity;
    
    // send and receive one block of samples
    x->phelmholtz->iosamples(in, out, blocksize);
    
    // get freq and fidelity values
    freq = x->samplerate / x->phelmholtz->getperiod();
    fidelity = x->phelmholtz->getfidelity();
    
    // if there is new pitch info, send messages to outlets
    if((x->freq != freq) 
        && (fidelity > x->fidelitythreshold)
        && (freq < x->maxfreq))
    {
        x->freq = freq;
        x->fidelity = fidelity;
        clock_delay(x->periodclock, 0);
    }
    
    return(w+5);
}
        

static void helmholtz_dsp(t_helmholtz *x, t_signal **sp)
{
    x->samplerate = (t_float)sp[0]->s_sr;
    dsp_add(helmholtz_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
    
}


static void *helmholtz_new(t_floatarg framesize, t_floatarg overlap)
{
    t_helmholtz *x = (t_helmholtz*)pd_new(helmholtz_class);
    outlet_new(&x->x_obj, &s_signal);
    x->periodoutlet = outlet_new(&x->x_obj, &s_float);
    x->fidelityoutlet = outlet_new(&x->x_obj, &s_float);
    x->phelmholtz = new Helmholtz();
    x->phelmholtz->setframesize(framesize);
    x->phelmholtz->setoverlap(overlap);
    x->freq = 0.;
    x->fidelity = 0.;
    x->fidelitythreshold = DEFFIDELITY;
    x->maxfreq = DEFMAXFREQ;
    x->periodclock = clock_new(x, (t_method)helmholtz_tick);
    return(x);
}


static void helmholtz_free(t_helmholtz *x)
{
    delete x->phelmholtz;
    if(x->periodclock) clock_free(x->periodclock);
}


static void helmholtz_framesize(t_helmholtz *x, t_floatarg framesize)
{
    x->phelmholtz->setframesize((int)framesize);
}


static void helmholtz_overlap(t_helmholtz *x, t_floatarg overlap)
{
    x->phelmholtz->setoverlap((int)overlap);
}


static void helmholtz_bias(t_helmholtz *x, t_floatarg bias)
{
    x->phelmholtz->setbias(bias);
}


static void helmholtz_fidelity(t_helmholtz *x, t_floatarg fidelity)
{
    if(fidelity > 1.) fidelity = 1.;
    if(fidelity < 0.) fidelity = 0.;
    x->fidelitythreshold = fidelity;
}


static void helmholtz_maxfreq(t_helmholtz *x, t_floatarg maxfreq)
{
    if(maxfreq > 2000.) maxfreq = 2000.;
    if(maxfreq < 0.) maxfreq = 0.;
    x->maxfreq = maxfreq;
}


static void helmholtz_sens(t_helmholtz *x, t_floatarg dB)
{
    t_float rms;
    
    if(dB > 100.) dB = 100.;
    if(dB < 0.) dB = 0.;
    
    rms = dbtorms(dB);
    x->phelmholtz->setminRMS(dbtorms(dB));
}


extern "C" EXPORT void helmholtz_tilde_setup(void)
{ 
    helmholtz_class = class_new(gensym("helmholtz~"), (t_newmethod)helmholtz_new,
        (t_method)helmholtz_free, sizeof(t_helmholtz), 
        CLASS_DEFAULT, A_DEFFLOAT, A_DEFFLOAT, A_NULL);
    CLASS_MAINSIGNALIN(helmholtz_class, t_helmholtz, f);
    class_addmethod(helmholtz_class, (t_method)helmholtz_dsp, gensym("dsp"),
        (t_atomtype) 0);
    class_addmethod(helmholtz_class, (t_method)helmholtz_framesize, gensym("framesize"), A_FLOAT, 0);
    class_addmethod(helmholtz_class, (t_method)helmholtz_overlap, gensym("overlap"), A_FLOAT, 0);
    class_addmethod(helmholtz_class, (t_method)helmholtz_bias, gensym("bias"), A_FLOAT, 0);
    class_addmethod(helmholtz_class, (t_method)helmholtz_fidelity, gensym("fidelity"), A_FLOAT, 0);
    class_addmethod(helmholtz_class, (t_method)helmholtz_maxfreq, gensym("maxfreq"), A_FLOAT, 0);
    class_addmethod(helmholtz_class, (t_method)helmholtz_sens, gensym("sens"), A_FLOAT, 0);
    post("helmholtz~ pitch tracker version 1.0, written by Katja Vetter");
    post("helmholtz~ uses SNAC function as invented by Philip McLeod");
}
    
    
