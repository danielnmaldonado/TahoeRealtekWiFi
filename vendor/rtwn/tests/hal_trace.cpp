/* Offline register-transport tests for the actual RTL8192EU USB HAL.
 * The kernel/net80211 types are replaced by a minimal transport shim; the
 * implementation and pinned register tables are compiled unchanged below.
 * No USB device, kernel interface or network is opened by this test.
 */
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <vector>
#define __packed __attribute__((__packed__))
#define IEEE80211_ADDR_LEN 6
#define nitems(a) (sizeof(a)/sizeof((a)[0]))
#include "../r92creg.h"
#define RTW_8192EU_USB_HAL_HPP
#define RTWN_CHIP_92C 0x00000001
#define RTWN_CHIP_92C_1T2R 0x00000002
#define RTWN_CHIP_UMC 0x00000004
#define RTWN_CHIP_UMC_A_CUT 0x00000008
#define RTWN_CHIP_88C 0x00000010
#define RTWN_CHIP_88E 0x00000020
#define RTWN_CHIP_92E 0x00000040
#define RTWN_CHIP_88F 0x00000200
#define RTWN_CHIP_USB 0x80000000
#define RTWN_FLAG_CCK_HIPWR 1
struct device { char dv_xname[16]; };
struct rtwn_ops {
    void *cookie;
    uint8_t (*read_1)(void *,uint16_t);
    uint16_t (*read_2)(void *,uint16_t);
    uint32_t (*read_4)(void *,uint16_t);
    void (*write_1)(void *,uint16_t,uint8_t);
    void (*write_2)(void *,uint16_t,uint16_t);
    void (*write_4)(void *,uint16_t,uint32_t);
    void (*delay_us)(void *,uint32_t);
    int (*write_region)(void *,uint16_t,const uint8_t *,size_t);
    int (*io_error)(void *);
};
struct rtwn_softc {
    rtwn_ops sc_ops;
    device *sc_pdev;
    uint32_t chip,sc_flags;
    uint8_t board_type,crystal_cap;
};
void rtwn_delay(rtwn_softc *s,uint32_t us) { s->sc_ops.delay_us(s->sc_ops.cookie,us); }
int rtwn_io_error(rtwn_softc *s) { return s->sc_ops.io_error(s->sc_ops.cookie); }
#include "../../../kext/RTW8192EUHAL.cpp"

struct Write { uint16_t address; unsigned width; uint32_t value; };
struct Block { uint16_t address; std::vector<uint8_t> bytes; };
struct Trace {
    std::map<uint16_t,uint32_t> regs;
    std::vector<Write> writes;
    std::vector<Block> blocks;
    std::vector<uint32_t> delays;
    int error=0;
    bool completeLLT=true,completePower=true;
    unsigned writesUntilError=0,readsUntilError=0,readCalls=0;
};
static uint32_t read(Trace &t,uint16_t a) {
    ++t.readCalls;if(t.readsUntilError && --t.readsUntilError==0)t.error=EIO;
    uint32_t v=t.regs[a];
    if(a==R92E_AUTO_LLT && t.completeLLT) v&=~R92E_AUTO_LLT_EN;
    if(a==R92C_APS_FSMCO && t.completePower) v=(v|R92C_APS_FSMCO_SUS_HOST)&~R92C_APS_FSMCO_APFM_ONMAC;
    return v;
}
static uint8_t r1(void*p,uint16_t a) {return (uint8_t)read(*(Trace*)p,a);}
static uint16_t r2(void*p,uint16_t a) {return (uint16_t)read(*(Trace*)p,a);}
static uint32_t r4(void*p,uint16_t a) {return read(*(Trace*)p,a);}
static void write(Trace&t,uint16_t a,unsigned w,uint32_t v) {
    t.regs[a]=v;t.writes.push_back({a,w,v});
    if(t.writesUntilError && --t.writesUntilError==0)t.error=EIO;
}
static void w1(void*p,uint16_t a,uint8_t v){write(*(Trace*)p,a,1,v);}
static void w2(void*p,uint16_t a,uint16_t v){write(*(Trace*)p,a,2,v);}
static void w4(void*p,uint16_t a,uint32_t v){write(*(Trace*)p,a,4,v);}
static void delay(void*p,uint32_t us){((Trace*)p)->delays.push_back(us);}
static int err(void*p){return ((Trace*)p)->error;}
static int block(void*p,uint16_t a,const uint8_t*b,size_t n){
    auto&t=*(Trace*)p;if(t.error)return t.error;
    t.blocks.push_back({a,std::vector<uint8_t>(b,b+n)});return 0;
}
static rtwn_softc context(Trace&t) {
    static device dev={{'t','r','a','c','e',0}};
    rtwn_softc s={};s.sc_pdev=&dev;s.chip=RTWN_CHIP_USB|RTWN_CHIP_92E;s.crystal_cap=0x20;
    s.sc_ops={&t,r1,r2,r4,w1,w2,w4,delay,block,err};return s;
}
static bool wrote(const Trace&t,uint16_t a,unsigned w,uint32_t v){
    for(auto&e:t.writes)if(e.address==a&&e.width==w&&e.value==v)return true;return false;
}
int main(){
    {Trace t;auto s=context(t);assert(rtw92eu_power_on(&s)==0);
     assert(wrote(t,R92E_LDO_SWR_CTRL,1,0x83));
     const uint32_t blocks=R92C_CR_HCI_TXDMA_EN|R92C_CR_HCI_RXDMA_EN|R92C_CR_TXDMA_EN|R92C_CR_RXDMA_EN|R92C_CR_PROTOCOL_EN|R92C_CR_SCHEDULE_EN|R92C_CR_ENSEC|R92C_CR_CALTMR_EN;
     assert(wrote(t,R92C_CR,2,blocks));}
    {Trace t;t.regs[R92C_SYS_CFG]=R92E_SYS_CFG_SPSLDO_SEL;auto s=context(t);
     assert(rtw92eu_power_on(&s)==0);assert(wrote(t,R92E_LDO_SWR_CTRL,1,0xc3));}
    {Trace t;t.completePower=false;auto s=context(t);assert(rtw92eu_power_on(&s)==ETIMEDOUT);}
    for(unsigned endpoints=1;endpoints<=3;endpoints++){
        Trace t;auto s=context(t);assert(rtw92eu_dma_init(&s,endpoints)==0);
        assert(wrote(t,R92C_TXPKTBUF_BCNQ_BDNY,1,249));
        assert(wrote(t,R92C_TRXFF_BNDY+2,2,R92E_MAX_RX_DMA_SIZE-1));
        assert(t.regs[R92C_RQPN_NPQ]==(endpoints>=2?16U:0U));
        assert(rtw92eu_dma_init(&s,0)==EINVAL);assert(rtw92eu_dma_init(&s,4)==EINVAL);
    }
    {Trace t;t.completeLLT=false;auto s=context(t);assert(rtw92eu_dma_init(&s,3)==ETIMEDOUT);}
    {Trace t;auto s=context(t);assert(rtw92eu_mac_init(&s)==0);
     assert(t.writes.size()==nitems(rtl8192eu_mac));assert(t.writes.size()==102);}
    {Trace t;auto s=context(t);assert(rtw92eu_bb_init(&s)==0);
     size_t agc=0;for(auto&w:t.writes)if(w.address==R92C_OFDM0_AGCRSSITABLE)agc++;
     assert(agc==129); /* one BB default plus 128 AGC entries */assert(wrote(t,R92C_AFE_XTAL_CTRL,4,0x000f81fb));
     assert(wrote(t,R92C_OFDM0_AGCCORE1(0),4,0x00040022));}
    for(bool highSpeed:{false,true}){Trace t;auto s=context(t);
     // The previous owner may have left DMA mode selected. USB aggregation
     // must explicitly clear only mode bit 31 and preserve the other 7 bits.
     t.regs[R92C_RXDMA_AGG_PG_TH+3]=0xd5;
     t.regs[R92C_RXDMA_AGG_PG_TH]=0xb0;
     assert(rtw92eu_aggr_init(&s,highSpeed)==0);
     assert(wrote(t,R92E_RXDMA_PRO,1,highSpeed?0x1e:0x2e));
     assert(t.regs[R92C_RXDMA_AGG_PG_TH+3]==0x55);
     assert(t.regs[R92C_RXDMA_AGG_PG_TH]==0xb6);assert(wrote(t,R92C_RXDMA_AGG_PG_TH+1,1,32));}
    for(unsigned fail=1;fail<=8;++fail){Trace t;auto s=context(t);t.writesUntilError=fail;
     assert(rtw92eu_aggr_init(&s,true)==EIO);assert(t.writes.size()==fail);}
    // Every failed read stops before writing a value derived from that read.
    for(unsigned fail=1;fail<=6;++fail){Trace t;auto s=context(t);t.readsUntilError=fail;
     const unsigned writesBeforeRead[]={0,1,3,4,5,7};
     assert(rtw92eu_aggr_init(&s,true)==EIO);assert(t.readCalls==fail);
     assert(t.writes.size()==writesBeforeRead[fail-1]);}
    {Trace t;auto s=context(t);uint8_t bytes[4096];for(size_t i=0;i<sizeof(bytes);i++)bytes[i]=(uint8_t)i;
     assert(rtw92eu_fw_loadpage(&s,7,bytes,sizeof(bytes))==0);size_t count=0;
     for(auto&b:t.blocks){assert(b.address==R92C_FW_START_ADDR+count);assert(b.bytes.size()<=254);assert(!memcmp(b.bytes.data(),bytes+count,b.bytes.size()));count+=b.bytes.size();}
     assert(count==sizeof(bytes));assert(rtw92eu_fw_loadpage(&s,8,bytes,1)==EINVAL);
     assert(rtw92eu_fw_loadpage(&s,0,bytes,4097)==EINVAL);}
    {Trace t;t.writesUntilError=1;auto s=context(t);assert(rtw92eu_mac_init(&s)==EIO);}
    {Trace t;t.error=EIO;auto s=context(t);uint8_t b=1;assert(rtw92eu_fw_loadpage(&s,0,&b,1)==EIO);}
    puts("RTL8192EU HAL: offline power, DMA, MAC/BB/AGC, aggregation, firmware and I/O error traces passed");
    return 0;
}
