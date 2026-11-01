/* SPDX-License-Identifier: GPL-2.0-only */
#include "RTWUserClient.hpp"
#include "RTWUSBController.hpp"
#include <kern/task.h>
#include <string.h>

#define super IOUserClient
OSDefineMetaClassAndStructors(RTWUserClient, IOUserClient)

namespace {
void wipe(void* memory, size_t size) {
    volatile uint8_t* bytes = static_cast<volatile uint8_t*>(memory);
    while (size--) *bytes++ = 0;
}
}

bool RTWUserClient::initWithTask(task_t owningTask, void* securityToken,
                                UInt32 type, OSDictionary* properties) {
    /* The SDK defines Administrator as "root". The owning task is the
     * credential source; securityToken is not assumed to be a task pointer. */
    if (!owningTask || type != RTW_CONTROL_USER_CLIENT_TYPE ||
        clientHasPrivilege(owningTask, kIOClientPrivilegeAdministrator) != kIOReturnSuccess)
        return false;
    return super::initWithTask(owningTask, securityToken, type, properties);
}

bool RTWUserClient::start(IOService* provider) {
    RTWUSBController* controller = OSDynamicCast(RTWUSBController, provider);
    if (!controller || !super::start(provider)) return false;
    controller->retain();
    controller_ = controller;
    return true;
}

void RTWUserClient::stop(IOService* provider) {
    super::stop(provider);
}

void RTWUserClient::free() {
    if (controller_) {
        controller_->release();
        controller_ = nullptr;
    }
    super::free();
}

IOReturn RTWUserClient::clientClose() {
    (void)terminate();
    return kIOReturnSuccess;
}

IOReturn RTWUserClient::externalMethod(uint32_t selector,
    IOExternalMethodArguments* arguments, IOExternalMethodDispatch* dispatch,
    OSObject* target, void* reference) {
    (void)dispatch; (void)target; (void)reference;
    /* Recheck the caller so transferring an opened connection does not grant
     * a nonadministrator access to the driver's control interface. */
    if (clientHasPrivilege(current_task(), kIOClientPrivilegeAdministrator) != kIOReturnSuccess)
        return kIOReturnNotPrivileged;
    if (isInactive() || !controller_ || controller_->isInactive())
        return kIOReturnNotAttached;
    if (selector >= RTW_CONTROL_SELECTOR_COUNT) return kIOReturnUnsupported;
    const size_t expectedOutput = rtw_control_output_size(selector);
    if (!arguments || arguments->version != kIOExternalMethodArgumentsCurrentVersion ||
        arguments->scalarInputCount || arguments->scalarOutputCount ||
        arguments->asyncWakePort != MACH_PORT_NULL || arguments->asyncReferenceCount ||
        arguments->structureInputDescriptor || arguments->structureOutputDescriptor ||
        arguments->structureVariableOutputData ||
        !arguments->structureOutput || arguments->structureOutputSize != expectedOutput ||
        rtw_control_validate_request(selector, arguments->structureInput,
            arguments->structureInputSize) != RTW_CONTROL_VALID)
        return kIOReturnBadArgument;

    union Request {
        RTWControlRequest request;
        RTWControlJoin join;
    } request{};
    const size_t inputLength = rtw_control_input_size(selector);
    memcpy(&request, arguments->structureInput, inputLength);
    memset(arguments->structureOutput, 0, expectedOutput);
    size_t outputLength = expectedOutput;
    IOReturn result = controller_->controlRequest(selector, &request, inputLength,
        arguments->structureOutput, &outputLength);
    wipe(&request, sizeof(request));
    if (result == kIOReturnSuccess && outputLength != expectedOutput)
        result = kIOReturnInternalError;
    if (result != kIOReturnSuccess) {
        memset(arguments->structureOutput, 0, expectedOutput);
        outputLength = 0;
    }
    arguments->structureOutputSize = static_cast<uint32_t>(outputLength);
    return result;
}
