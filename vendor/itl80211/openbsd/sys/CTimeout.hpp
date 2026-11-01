//
//  CTimeout.h
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

#ifndef CTimeout_h
#define CTimeout_h

#include <IOKit/IOTimerEventSource.h>
#include <libkern/c++/OSObject.h>

class CTimeout : public OSObject {
    OSDeclareDefaultStructors(CTimeout)
    
public:
    static void timeoutOccurred(OSObject* owner, IOTimerEventSource* timer);
    
    static IOReturn timeout_add_msec(OSObject *target, void *arg0, void *arg1, void *arg2, void *arg3);
    
    static IOReturn timeout_del(OSObject *target, void *arg0, void *arg1, void *arg2, void *arg3);
    
    static IOReturn timeout_free(OSObject *target, void *arg0, void *arg1, void *arg2, void *arg3);
    
    static IOReturn timeout_set(OSObject *target, void *arg0, void *arg1, void *arg2, void *arg3);
    
    static IOReturn timeout_pending(OSObject *target, void *arg0, void *arg1, void *arg2, void *arg3);
    
public:
    IOTimerEventSource* tm = nullptr;
    void (*to_func)(void *) = nullptr;        /* function to call */
    void *to_arg = nullptr;                /* function argument */
    bool isPending = false;
};

#endif /* CTimeout_h */
