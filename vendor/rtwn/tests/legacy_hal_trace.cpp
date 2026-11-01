/* Compile the actual USB HAL with the established offline transport shim.
 * These tests catch accidentally sending 92E power/LLT/table sequences to
 * legacy CU/88E silicon. No hardware or network access is performed. */
#define main unused_92e_trace_main
#include "hal_trace.cpp"
#undef main

static uint32_t legacyRead(Trace &t, uint16_t a) {
    uint32_t v = read(t, a);
    if (a == R92C_APS_FSMCO && t.completePower)
        v |= R92C_APS_FSMCO_PFM_ALDN;
    if (a == R92C_LLT_INIT && t.completeLLT)
        v &= ~R92C_LLT_INIT_OP_M;
    return v;
}
static uint8_t lr1(void *p, uint16_t a) { return (uint8_t)legacyRead(*(Trace *)p,a); }
static uint16_t lr2(void *p, uint16_t a) { return (uint16_t)legacyRead(*(Trace *)p,a); }
static uint32_t lr4(void *p, uint16_t a) { return legacyRead(*(Trace *)p,a); }
static rtwn_softc legacyContext(Trace &t, uint32_t chip) {
    auto s=context(t);s.chip=RTWN_CHIP_USB|chip;
    s.sc_ops.read_1=lr1;s.sc_ops.read_2=lr2;s.sc_ops.read_4=lr4;
    return s;
}
int main() {
    for (auto chip : {RTWN_CHIP_92C, RTWN_CHIP_88C, RTWN_CHIP_88E}) {
        { Trace t;auto s=legacyContext(t,chip);assert(rtw92eu_power_on(&s)==0);
          assert(!wrote(t,R92E_LDO_SWR_CTRL,1,0x83));
          if(chip==RTWN_CHIP_88E) assert(wrote(t,R92C_CR,2,0));
          else {assert(wrote(t,R92C_SPS0_CTRL,1,0x2b));assert(wrote(t,0xfe10,1,0x19));} }
        { Trace t;t.completePower=false;auto s=legacyContext(t,chip);
          assert(rtw92eu_power_on(&s)==ETIMEDOUT); }
        for(unsigned endpoints=1;endpoints<=3;++endpoints) {
            Trace t;auto s=legacyContext(t,chip);assert(rtw92eu_dma_init(&s,endpoints)==0);
            unsigned count=0;for(auto&w:t.writes)if(w.address==R92C_LLT_INIT)++count;
            assert(count==(chip==RTWN_CHIP_88E?177U:256U));
            assert(!wrote(t,R92E_AUTO_LLT,4,R92E_AUTO_LLT_EN));
            assert(wrote(t,R92C_TXPKTBUF_BCNQ_BDNY,1,chip==RTWN_CHIP_88E?169U:249U));
            assert(MS(t.regs[R92C_LLT_INIT],R92C_LLT_INIT_DATA)==(chip==RTWN_CHIP_88E?169U:249U));
        }
        { Trace t;t.completeLLT=false;auto s=legacyContext(t,chip);
          assert(rtw92eu_dma_init(&s,3)==ETIMEDOUT); }
        { Trace t;t.completeLLT=false;t.writesUntilError=1;auto s=legacyContext(t,chip);
          assert(rtw92eu_dma_init(&s,3)==EIO);assert(t.writes.size()==1); }
        { Trace t;auto s=legacyContext(t,chip);assert(rtw92eu_mac_init(&s)==0);
          assert(t.writes.size()==(chip==RTWN_CHIP_88E?nitems(rtl8188eu_mac)+1:nitems(rtl8192cu_mac)));
          assert(rtw92eu_bb_init(&s)==0);
          assert(wrote(t,0x800,4,chip==RTWN_CHIP_92C?0x80040002U:0x80040000U));
          assert(wrote(t,0x90c,4,chip==RTWN_CHIP_92C?0x81121313U:0x81121111U));
          if(chip==RTWN_CHIP_88E)assert(wrote(t,0xc50,4,0x69553422));
          assert(rtw92eu_aggr_init(&s,true)==0);
          assert(wrote(t,R92C_RXDMA_AGG_PG_TH,1,48));
          assert(wrote(t,chip==RTWN_CHIP_88E?R92C_RXDMA_AGG_PG_TH+1:R92C_USB_DMA_AGG_TO,1,4)); }
        { Trace t;auto s=legacyContext(t,chip);uint8_t data[4096]{};
          assert(rtw92eu_fw_loadpage(&s,0,data,sizeof(data))==0);
          assert(t.blocks[0].bytes.size()==196U); }
        { Trace t;t.error=EIO;auto s=legacyContext(t,chip);
          assert(rtw92eu_power_on(&s)==EIO);assert(rtw92eu_dma_init(&s,3)==EIO);
          assert(rtw92eu_mac_init(&s)==EIO);assert(rtw92eu_bb_init(&s)==EIO);
          assert(rtw92eu_aggr_init(&s,true)==EIO);assert(t.writes.empty()); }
    }
    { Trace t;auto s=legacyContext(t,0);assert(rtw92eu_power_on(&s)==EINVAL);assert(t.writes.empty()); }
    puts("Legacy CU/88E HAL: actual power, manual LLT/DMA, MAC/BB, aggregation, firmware and error traces passed");
}
