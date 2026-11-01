//
//  CTimeout.cpp
//  AppleIntelWifiAdapter
//
//  Created by 钟先耀 on 2020/1/30.
//  Copyright © 2020 钟先耀. All rights reserved.
//

/*
* Copyright (C) 2020  钟先耀
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#include <sys/CTimeout.hpp>
#include <IOKit/IOWorkLoop.h>

void CTimeout::timeoutOccurred(OSObject* owner, IOTimerEventSource* timer)
{
    CTimeout* timeout=OSDynamicCast(CTimeout,owner);
    if(!timeout||!timer||!timeout->isPending||!timeout->to_func)return;
    // A callback is allowed to cancel/free/rearm its own timeout.
    timeout->retain();timer->retain();
    auto function=timeout->to_func;
    void* argument=timeout->to_arg;
    timeout->isPending=false;
    function(argument);
    timer->release();timeout->release();
}

IOReturn CTimeout::timeout_add_msec(OSObject*,void* arg0,void* arg1,void* arg2,void*)
{
    auto** holder=static_cast<CTimeout**>(arg0);
    auto* loop=static_cast<IOWorkLoop*>(arg1);
    if(!holder||!*holder||!loop||!arg2)return kIOReturnBadArgument;
    const int milliseconds=*static_cast<int*>(arg2);
    CTimeout* timeout=*holder;
    if(milliseconds<0||!timeout->to_func)return kIOReturnBadArgument;
    if(!timeout->tm) {
        auto* timer=IOTimerEventSource::timerEventSource(timeout,&CTimeout::timeoutOccurred);
        if(!timer)return kIOReturnNoMemory;
        const IOReturn result=loop->addEventSource(timer);
        if(result!=kIOReturnSuccess){timer->release();return result;}
        timeout->tm=timer;
    }else if(timeout->tm->getWorkLoop()!=loop) {
        return kIOReturnBadArgument;
    }
    timeout->tm->enable();
    const IOReturn result=timeout->tm->setTimeoutMS(static_cast<UInt32>(milliseconds));
    timeout->isPending=result==kIOReturnSuccess;
    return result;
}

IOReturn CTimeout::timeout_del(OSObject*,void* arg0,void*,void*,void*)
{
    auto** holder=static_cast<CTimeout**>(arg0);
    if(!holder||!*holder)return kIOReturnSuccess;
    CTimeout* timeout=*holder;
    if(timeout->tm)timeout->tm->cancelTimeout();
    timeout->isPending=false;
    return kIOReturnSuccess;
}

IOReturn CTimeout::timeout_free(OSObject*,void* arg0,void*,void*,void*)
{
    auto** holder=static_cast<CTimeout**>(arg0);
    if(!holder||!*holder)return kIOReturnSuccess;
    CTimeout* timeout=*holder;
    *holder=nullptr;
    timeout->isPending=false;
    if(timeout->tm) {
        auto* timer=timeout->tm;timeout->tm=nullptr;
        timer->cancelTimeout();timer->disable();
        if(auto* loop=timer->getWorkLoop())loop->removeEventSource(timer);
        timer->release();
    }
    timeout->to_func=nullptr;timeout->to_arg=nullptr;
    timeout->release();
    return kIOReturnSuccess;
}

IOReturn CTimeout::timeout_set(OSObject*,void* arg0,void* arg1,void* arg2,void*)
{
    auto** holder=static_cast<CTimeout**>(arg0);
    if(!holder||!arg1)return kIOReturnBadArgument;
    if(!*holder) {
        auto* timeout=new CTimeout;
        if(!timeout)return kIOReturnNoMemory;
        if(!timeout->init()){timeout->release();return kIOReturnNoMemory;}
        *holder=timeout;
    }
    CTimeout* timeout=*holder;
    if(timeout->tm)timeout->tm->cancelTimeout();
    timeout->isPending=false;
    timeout->to_func=reinterpret_cast<void(*)(void*)>(arg1);
    timeout->to_arg=arg2;
    return kIOReturnSuccess;
}

IOReturn CTimeout::timeout_pending(OSObject*,void* arg0,void*,void*,void*)
{
    auto** holder=static_cast<CTimeout**>(arg0);
    return holder&&*holder&&(*holder)->isPending?kIOReturnSuccess:kIOReturnError;
}
