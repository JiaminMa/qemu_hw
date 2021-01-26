#ifndef IMX8ULP_M33_H
#define IMX8ULP_M33_H

#include "qemu/osdep.h"
#include "qemu/units.h"
#include "qapi/error.h"
#include "qemu/error-report.h"
#include "hw/arm/boot.h"
#include "hw/arm/armv7m.h"
#include "hw/or-irq.h"
#include "hw/boards.h"
#include "exec/address-spaces.h"
#include "sysemu/sysemu.h"
#include "hw/misc/unimp.h"
#include "hw/char/cmsdk-apb-uart.h"
#include "hw/misc/tz-mpc.h"
#include "hw/misc/tz-msc.h"
#include "hw/arm/armsse.h"
#include "hw/ssi/pl022.h"
#include "hw/core/split-irq.h"
#include "migration/vmstate.h"

/*=======================================
    Verilog debug module
 ========================================*/
#define TYPE_VERILOG_DEBUG "verilog_debug"
#define VERILOG_PRINT       0x41

typedef struct {
    SysBusDevice parent_obj;

    qemu_irq irq;
    MemoryRegion iomem;
    uint32_t reg0;
    uint32_t reg1;
    uint32_t reg2;
    uint32_t reg3;
} verilog_debug_state;

#define VERILOG_DEBUG(obj) \
    OBJECT_CHECK(verilog_debug_state, (obj), TYPE_VERILOG_DEBUG)

/*=======================================
    FSB module Start
 ========================================*/
#define TYPE_IMX_FSB "imx_fsb"

typedef struct {
    SysBusDevice parent_obj;

    qemu_irq irq;
    MemoryRegion iomem;

    uint32_t reg[256];
} imx_fsb_state;

#define IMX_FSB(obj) \
    OBJECT_CHECK(imx_fsb_state, (obj), TYPE_IMX_FSB)

extern void imx_fsb_init_cb(imx_fsb_state *fsb);

/*=======================================
    S400 MU Start
 ========================================*/

#define TYPE_IMX_S400_MU "imx_s400_mu"

typedef struct {
    SysBusDevice parent_obj;

    qemu_irq irq;
    MemoryRegion iomem;

    uint32_t ver;               //< 00h: Version ID Register
    uint32_t par;               //< 04h: Parameter Register
    uint32_t cr;                //< 08h: Control Register
    uint32_t sr;                //< 0Ch: Status Register
    uint32_t reserved0[68];     //< 10h - 11Fh: Reserved Register
    uint32_t tcr;               //< 120h: Transimit Control Register
    uint32_t tsr;               //< 124h: Transimit Status Register
    uint32_t rcr;               //< 128h: Receive Control Register
    uint32_t rsr;               //< 12Ch: Receive Status Register
    uint32_t reserved1[52];     //< 130h - 1FFh: Reserved Register
    uint32_t tr[16];            //< 200h - 23Fh: Transimit Register
    uint32_t reserved2[16];     //< 240h - 27Fh: Reserved Register
    uint32_t rr[16];            //< 280h: Receive Register
    uint32_t reserved4[14];     //< 288h - 2BCh
    uint32_t mu_attr;           //< 2C0h S4MUA Master Attributes register

} imx_s400_mu_state;


#define IMX_S400_MU(obj) \
    OBJECT_CHECK(imx_s400_mu_state, (obj), TYPE_IMX_S400_MU)

/*=======================================
    SIM0 Start
 ========================================*/
#define TYPE_IMX_SIM0 "imx_sim0"

typedef struct {
    SysBusDevice parent_obj;
    qemu_irq irq;
    MemoryRegion iomem;

    uint32_t gpr0;               /* 0h:  HW RGeneral Purpose Register 0 */
    uint32_t gpr1;               /* 4h:  HW General Purpose Register 1 */
    uint32_t dgo_ctrl0;          /* 8h:  RTD SIM DGO Control Register 0 */
    uint32_t dgo_ctrl1;          /* Ch:  RTD SIM DGO Control Register 1 */
    uint32_t dgo_gp[9];          /* 10h: RTD SIM DGO General Purpose Register 0 */
    uint32_t sysctrl0;           /* 34h: Realtime Domains System Control Register 0 */
    uint32_t ssram_acc_dis;      /* 38h: System Shared RAM Access Disable Register */
    uint32_t rtd_sysctrl0;       /* 3Ch: Realtime Domain System Control Register 0 */
    uint32_t lpav_per_dom_ctrl;  /* 40h: Low-Power Audio-Video Peripheral Domain Control */
    uint32_t lpav_mst_alo_ctrl;  /* 44h: LPAV Master Allocation Control Register */
    uint32_t lpav_slv_alo_ctrl;  /* 48h: LPAV Slave Allocation Control Register */
} imx_sim0_state;

#define IMX_SIM0(obj) \
    OBJECT_CHECK(imx_sim0_state, (obj), TYPE_IMX_SIM0)

/*=======================================
    TSTMR Module Start
 ========================================*/

#define TYPE_IMX_TSTMR  "imx_tstmr"

typedef struct {
    SysBusDevice parent_obj;
    qemu_irq irq;
    MemoryRegion iomem;

    uint32_t tstmr_l;
    uint32_t tstmr_h;
} imx_tstmr_state;

#define IMX_TSTMR(obj) \
    OBJECT_CHECK(imx_tstmr_state, (obj), TYPE_IMX_TSTMR)


/*=======================================
    ARG Module Start
 ========================================*/
typedef struct imx8ulp_arg_tag {

    uint32_t fuse[512];
    uint16_t m33_bt_cfg;
    uint16_t a35_bt_cfg;
    uint32_t bt_mode;
} imx8ulp_arg_t;

extern imx8ulp_arg_t g_imx8ulp_arg;
#define ARG_BUF_SIZE 1024

/*=======================================
    IMX8ULP CM33 CORE module Start
 ========================================*/
#define IMX8ULP_M33_NUMIRQ  92
#define VERILOG_DEBUG_START 0x3004EFF0
#define IMX_FSB_START       0x37010800
#define IMX_FSB_LOW_START   0x37010000
#define IMX_S400_MU_START   0x37040000
#define IMX_CGC0_START      0x3802f000
#define IMX_PCC0_START      0x38030000  
#define IMX_PCC1_START      0x38090000
#define IMX_ROMCP0_START    0x3803A000 
#define IMX_SIM0_S_START    0x3802B000
#define IMX_TSTMR_START     0x3802AC00
#define IMX_CMC0_START   0x38025000

typedef struct {
    MachineClass parent;
    const char *armsse_type;
} IMX8ULP_MachineClass;

typedef struct {
    MachineState parent;

    ARMSSE iotkit;
    MemoryRegion psram;
    MemoryRegion ssram[3];
    MemoryRegion ssram1_m;
    MemoryRegion cmc0;
    MemoryRegion fsb_low;
    MemoryRegion tstmr0;

    TZPPC ppc[5];
    TZMPC ssram_mpc[3];
    PL022State spi[5];
    UnimplementedDeviceState i2c[4];

    UnimplementedDeviceState cgc0;
    UnimplementedDeviceState pcc0;
    UnimplementedDeviceState pcc1;
    UnimplementedDeviceState romcp0;
    UnimplementedDeviceState upower;

    TZMSC msc[4];
    CMSDKAPBUART uart[5];
    SplitIRQ sec_resp_splitter;
    qemu_or_irq uart_irq_orgate;
    SplitIRQ cpu_irq_splitter[IMX8ULP_M33_NUMIRQ];
} IMX8ULP_M33_MachineState;

#define TYPE_IMX8ULP_MACHINE "imx8ulp"
#define TYPE_IMX8ULP_M33_MACHINE MACHINE_TYPE_NAME("imx8ulp-m33")

#define IMX8ULP_MACHINE(obj) \
    OBJECT_CHECK(IMX8ULP_M33_MachineState, obj, TYPE_IMX8ULP_MACHINE)
#define IMX8ULP_MACHINE_GET_CLASS(obj) \
    OBJECT_GET_CLASS(IMX8ULP_MachineClass, obj, TYPE_IMX8ULP_MACHINE)
#define IMX8ULP_MACHINE_CLASS(klass) \
    OBJECT_CLASS_CHECK(IMX8ULP_MachineClass, klass, TYPE_IMX8ULP_MACHINE)

/* Main SYSCLK frequency in Hz */
#define SYSCLK_FRQ 24000000

typedef MemoryRegion *MakeDevFn(IMX8ULP_M33_MachineState *mms, void *opaque,
        const char *name, hwaddr size);

typedef struct PPCPortInfo {
    const char *name;
    MakeDevFn *devfn;
    void *opaque;
    hwaddr addr;
    hwaddr size;
} PPCPortInfo;

typedef struct PPCInfo {
    const char *name;
    PPCPortInfo ports[TZ_NUM_PORTS];
} PPCInfo;

#endif
