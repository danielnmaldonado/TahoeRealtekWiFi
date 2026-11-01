// SPDX-License-Identifier: BSD-3-Clause
int RTWUSBController::acRead(void*p,uint16_t a,unsigned width,uint32_t*out) {
    if(!out||(width!=1&&width!=2&&width!=4))return EINVAL;
    auto*s=owner(p);*out=s->readRegister(a,(uint8_t)width);
    return ioError(p);
}
int RTWUSBController::acWrite(void*p,uint16_t a,unsigned width,uint32_t value) {
    if(width!=1&&width!=2&&width!=4)return EINVAL;
    return owner(p)->writeRegister(a,(uint8_t)width,value);
}
rtw_8814au_speed RTWUSBController::acSpeed() const {
    if(!rxPipe_)return RTW_8814AU_FULL_SPEED;
    /* getSpeed returns tInternalUSBHostConnectionSpeed, not the public
     * tIOUSBHostConnectionSpeed enumeration (their numeric values differ). */
    if(rxPipe_->getSpeed()>=kUSBHostConnectionSpeedSuper)return RTW_8814AU_SUPER_SPEED;
    return rxPipe_->getSpeed()==kUSBHostConnectionSpeedHigh?RTW_8814AU_HIGH_SPEED:RTW_8814AU_FULL_SPEED;
}
int RTWUSBController::acBulk(void*p,uint8_t qsel,const uint8_t*bytes,size_t length) {
    auto*s=owner(p);
    if(!bytes||!length||length>4137||(qsel!=RTW_8814AU_QSEL_BEACON&&qsel!=RTW_8814AU_QSEL_H2C))return EINVAL;
    if(ioError(p)||!s->txPipes_[0]||!s->txMPS_[0])return ENXIO;
    /* Linux8814 rqpn maps reserved pages/H2C to HIGH, first bulk OUT.
     * Only internal firmware transport uses this synchronous callback. */
    const size_t wire=length+(length%s->txMPS_[0]==0?1:0);
    auto*memory=IOBufferMemoryDescriptor::withCapacity(wire,kIODirectionOut);
    if(!memory||!memory->getBytesNoCopy()){if(memory)memory->release();return ENOMEM;}
    memcpy(memory->getBytesNoCopy(),bytes,length);
    if(wire!=length)static_cast<uint8_t*>(memory->getBytesNoCopy())[length]=0;
    auto result=memory->prepare(kIODirectionOut);uint32_t actual=0;
    if(result==kIOReturnSuccess){result=s->txPipes_[0]->io(memory,(uint32_t)wire,actual,1000);memory->complete(kIODirectionOut);}
    memory->release();
    if(result!=kIOReturnSuccess||actual!=wire){s->fault(result==kIOReturnSuccess?kIOReturnUnderrun:result);return EIO;}
    return 0;
}
