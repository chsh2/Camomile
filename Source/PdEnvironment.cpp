/*
// Copyright (c) 2015 Pierre Guillot.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "PdEnvironment.hpp"
#include "PdInstance.hpp"

extern "C"
{
#include "z_pd.h"
}

namespace pd
{
    // ==================================================================================== //
    //                                      PD                                              //
    // ==================================================================================== //
    
    Environment::Environment() noexcept
    {
        static bool initialized = false;
        if(!initialized)
        {
            z_pd_init();
        }
    }
    
    Environment::~Environment() noexcept
    {
        z_pd_clear();
    }
    
    Environment& Environment::get() noexcept
    {
        static Environment pd;
        return pd;
    }
    
    std::string Environment::getPdVersion()
    {
        return std::to_string(z_pd_version_getmajor()) + "." +
        std::to_string(z_pd_version_getminor()) + "." +
        std::to_string(z_pd_version_getbug());
    }
    
    void Environment::addToSearchPath(std::string const& path) noexcept
    {
        std::lock_guard<std::mutex> guard(Environment::get().m_mutex);
        z_pd_searchpath_add(path.c_str());
    }
    
    void Environment::clearSearchPath() noexcept
    {
        std::lock_guard<std::mutex> guard(Environment::get().m_mutex);
        z_pd_searchpath_clear();
    }
    
    void Environment::lock() noexcept
    {
        Environment::get().m_mutex.lock();
    }
    
    void Environment::unlock() noexcept
    {
        Environment::get().m_mutex.unlock();
    }
    
    Instance Environment::createInstance() noexcept
    {
        lock();
        Instance instance(z_pd_instance_new(1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr));
        unlock();
        return instance;
    }
    
    void Environment::free(Instance& instance)
    {
        lock();
        z_pd_instance_free(reinterpret_cast<z_instance *>(instance.m_ptr));
        unlock();
    }
    
    Tie Environment::createTie(std::string const& name) noexcept
    {
        return Tie(z_pd_get_tie(name.c_str()));
    }
    
    void Environment::send(Tie const& name, float val) noexcept
    {
        z_pd_messagesend_float(reinterpret_cast<z_tie *>(name.ptr), val);
    }
    
    void Environment::sendNote(int channel, int pitch, int velocity)
    {
        z_pd_midisend_noteon(channel, pitch, velocity);
    }
    
    void Environment::sendControlChange(int channel, int controller, int value)
    {
        z_pd_midisend_controlchange(channel, controller, value);
    }
    
    void Environment::sendProgramChange(int channel, int value)
    {
        z_pd_midisend_programchange(channel, value);
    }
    
    void Environment::sendPitchBend(int channel, int value)
    {
        z_pd_midisend_pitchbend(channel, value);
    }
    
    void Environment::sendAfterTouch(int channel, int value)
    {
        z_pd_midisend_aftertouch(channel, value);
    }
    
    void Environment::sendPolyAfterTouch(int channel, int pitch, int value)
    {
        z_pd_midisend_polyaftertouch(channel, pitch, value);
    }
    
    void Environment::sendMidiByte(int port, int byte)
    {
        z_pd_midisend_byte(port, byte);
    }
    
    void Environment::sendSysEx(int port, int byte)
    {
        z_pd_midisend_sysex(port, byte);
    }
    
    void Environment::sendSysRealtime(int port, int byte)
    {
        z_pd_midisend_sysrealtimein(port, byte);
    }
}

