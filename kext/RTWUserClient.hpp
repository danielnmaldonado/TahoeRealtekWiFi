/* SPDX-License-Identifier: GPL-2.0-only */
#pragma once

#include <IOKit/IOUserClient.h>
#include "RTWControl.h"

class RTWUSBController;

/* Synchronous, administrator-only fixed-structure control plane. The owning
 * controller serializes these operations on its workloop/command gate. */
class RTWUserClient final : public IOUserClient {
    OSDeclareDefaultStructors(RTWUserClient)
public:
    bool initWithTask(task_t owningTask, void* securityToken, UInt32 type,
                      OSDictionary* properties) override;
    bool start(IOService* provider) override;
    void stop(IOService* provider) override;
    void free() override;
    IOReturn clientClose() override;
    IOReturn externalMethod(uint32_t selector, IOExternalMethodArguments* arguments,
                            IOExternalMethodDispatch* dispatch = nullptr,
                            OSObject* target = nullptr, void* reference = nullptr) override;

private:
    /* Retained until free(): an external call can never observe a freed
     * provider while termination races with a request. */
    RTWUSBController* controller_ = nullptr;
};
