#include "imx8ulp_m33.h"

/*=======================================
    Verilog debug module Start
 ========================================*/

static void verilog_debug_write(void *opaque, hwaddr offset,
        uint64_t value, unsigned size);

static const VMStateDescription verilog_debug_vm = {
    .name = TYPE_VERILOG_DEBUG,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(reg0, verilog_debug_state),
        VMSTATE_UINT32(reg1, verilog_debug_state),
        VMSTATE_UINT32(reg2, verilog_debug_state),
        VMSTATE_UINT32(reg3, verilog_debug_state),
        VMSTATE_END_OF_LIST()
    }
};

static const MemoryRegionOps verilog_debug_ops = {
    .write = verilog_debug_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void verilog_debug_do_print(uint32_t addr)
{
    uint32_t stack_pointer = 0;
    uint32_t fmt_pointer = 0;
    uint8_t fmt_char = 0;
    uint32_t data = 0;
    uint8_t ch = 0;

    stack_pointer = addr;
    cpu_physical_memory_read(stack_pointer, &fmt_pointer, 4);
    stack_pointer += 4;
    fmt_char = 0xAA;

    while (fmt_char != 0) {
        cpu_physical_memory_read(fmt_pointer, &fmt_char, 1);
        fmt_pointer += 1;
        if (fmt_char == '%') {
            cpu_physical_memory_read(fmt_pointer, &fmt_char, 1);
            fmt_pointer += 1;

            switch(fmt_char) {
            case '%':
                printf("%%");
                break;
            case 'X':
            case 'x':
                cpu_physical_memory_read(stack_pointer, &data, 4);
                stack_pointer += 4;
                printf("%x", data);
                break;
            case 'd':
                cpu_physical_memory_read(stack_pointer, &data, 4);
                stack_pointer += 4;
                printf("%d", data);
                break;
            case 'c':
                cpu_physical_memory_read(stack_pointer, &data, 4);
                stack_pointer += 4;
                printf("%c", (uint8_t)data);
                break;
            case 's':
                cpu_physical_memory_read(stack_pointer, &data, 4);
                stack_pointer += 4;
                ch = 0xAA;
                while(ch != 0) {
                    cpu_physical_memory_read(data, &ch, 1);
                    data += 1;
                    printf("%c", ch);
                }
                break;
            default:
                break;
            }
        } else {
            printf("%c", fmt_char);
        }
    }
}

static void verilog_debug_write(void *opaque, hwaddr offset,
        uint64_t value, unsigned size)
{
    static uint32_t cmd_phase = 0;
    static uint32_t cmd_type = 0;
    static uint32_t addr = 0;


    if (cmd_phase == 0) {
        cmd_phase = 1;
        cmd_type = value;
    } else if (cmd_phase == 1) {
        cmd_phase = 2;
        addr = value;
    } else  {
        cmd_phase = 0;
        switch(cmd_type) {
        case VERILOG_PRINT:
            verilog_debug_do_print(addr);
            break;
        default:
            break;
        }
    }
}

static void verilog_debug_init(Object *obj)
{
    verilog_debug_state *s = VERILOG_DEBUG(obj);

    memory_region_init_io(&s->iomem, obj, &verilog_debug_ops, s, TYPE_VERILOG_DEBUG, 0x10);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->iomem);
}


static void verilog_debug_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->vmsd = &verilog_debug_vm;
}

static const TypeInfo verilog_debug_info = {
    .name          = TYPE_VERILOG_DEBUG,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(verilog_debug_state),
    .instance_init = verilog_debug_init,
    .class_init    = verilog_debug_class_init,
};

static void verilog_debug_types(void)
{
    type_register_static(&verilog_debug_info);
}

type_init(verilog_debug_types)

/*=======================================
    FSB module Start
 ========================================*/

static uint64_t imx_fsb_read(void *opaque, hwaddr offset, unsigned size);

static const VMStateDescription imx_fsb_vm = {
    .name = TYPE_IMX_FSB,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32_ARRAY(reg, imx_fsb_state, 256),
        VMSTATE_END_OF_LIST()
    }
};

static const MemoryRegionOps imx_fsb_ops = {
    .read = imx_fsb_read,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static uint64_t imx_fsb_read(void *opaque, hwaddr offset,
                                   unsigned size)
{

    imx_fsb_state *fsb = (imx_fsb_state *)opaque;
    //printf("%s hwaddr:%lx, size:%d, value:%x\n", __func__, offset, size, fsb->reg[offset / 4]);
    return fsb->reg[offset / 4];
}

static void imx_fsb_init(Object *obj)
{

    imx_fsb_state *s = IMX_FSB(obj);

    memory_region_init_io(&s->iomem, obj, &imx_fsb_ops, s, TYPE_IMX_FSB, 0x800);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->iomem);

    imx_fsb_init_cb(s);
}

static void imx_fsb_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->vmsd = &imx_fsb_vm;
}

static const TypeInfo imx_fsb_info = {
    .name          = TYPE_IMX_FSB,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(imx_fsb_state),
    .instance_init = imx_fsb_init,
    .class_init    = imx_fsb_class_init,
};

static void imx_fsb_types(void)
{
    type_register_static(&imx_fsb_info);
}

type_init(imx_fsb_types)

/*=======================================
    S400 MU Start
 ========================================*/
static uint64_t imx_s400_mu_read(void *opaque, hwaddr offset,
                                   unsigned size);
static void imx_s400_mu_write(void *opaque, hwaddr offset,
                            uint64_t value, unsigned size);

static const VMStateDescription imx_s400_mu_vm = {
    .name = TYPE_IMX_S400_MU,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(ver, imx_s400_mu_state),
        VMSTATE_UINT32(par, imx_s400_mu_state),
        VMSTATE_UINT32(cr, imx_s400_mu_state),
        VMSTATE_UINT32(sr, imx_s400_mu_state),
        VMSTATE_UINT32_ARRAY(reserved0, imx_s400_mu_state, 68),
        VMSTATE_UINT32(tcr, imx_s400_mu_state),
        VMSTATE_UINT32(tsr, imx_s400_mu_state),
        VMSTATE_UINT32(rcr, imx_s400_mu_state),
        VMSTATE_UINT32(rsr, imx_s400_mu_state),
        VMSTATE_UINT32_ARRAY(reserved1, imx_s400_mu_state, 52),
        VMSTATE_UINT32_ARRAY(tr, imx_s400_mu_state, 16),
        VMSTATE_UINT32_ARRAY(reserved2, imx_s400_mu_state, 16),
        VMSTATE_UINT32_ARRAY(rr, imx_s400_mu_state, 16),
        VMSTATE_UINT32_ARRAY(reserved4, imx_s400_mu_state, 14),
        VMSTATE_UINT32(mu_attr, imx_s400_mu_state),
        VMSTATE_END_OF_LIST()
    }
};

static const MemoryRegionOps imx_s400_mu_ops = {
    .read = imx_s400_mu_read,
    .write = imx_s400_mu_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void imx_s400_mu_handle_cmd(imx_s400_mu_state *mu)
{
    uint32_t cid = 0;
    cid = (mu->tr[0] & 0x00ff0000) >> 16;
    printf("%s tr[0]:%x\n", __func__, mu->tr[0]);
    if (cid == 0xc7) {
        printf("%s AHAB_RESET\n", __func__);
        exit(0);
    }
}

static uint64_t imx_s400_mu_read(void *opaque, hwaddr offset,
                                   unsigned size)
{
    uint32_t ret = 0;
    imx_s400_mu_state *mu = (imx_s400_mu_state *)opaque;

    if ((offset == 0x124) || (offset == 0x12C)) {
        ret = 0xFFFFFFFF;
    } else if ((offset >= 0x280) && (offset <= 0x28C)) {
        ret = mu->rr[(offset - 0x280) / 4];
    } else {
        ret = 0;
    }

    return ret;
}

static void imx_s400_mu_write(void *opaque, hwaddr offset,
        uint64_t value, unsigned size)
{
    imx_s400_mu_state *mu = (imx_s400_mu_state *)opaque;

    if ((offset >= 0x200) && (offset <= 0x21C)) {
        imx_s400_mu_handle_cmd(mu);
    }
}

static void imx_s400_mu_init(Object *obj)
{

    imx_s400_mu_state *s = IMX_S400_MU(obj);

    memory_region_init_io(&s->iomem, obj, &imx_s400_mu_ops, s, TYPE_IMX_S400_MU, 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->iomem);
}

static void imx_s400_mu_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->vmsd = &imx_s400_mu_vm;
}

static const TypeInfo imx_s400_mu_info = {
    .name          = TYPE_IMX_S400_MU,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(imx_s400_mu_state),
    .instance_init = imx_s400_mu_init,
    .class_init    = imx_s400_mu_class_init,
};

static void imx_s400_mu_types(void)
{
    type_register_static(&imx_s400_mu_info);
}

type_init(imx_s400_mu_types)

/*=======================================
    SIM0 Start
 ========================================*/
static uint64_t imx_sim0_read(void *opaque, hwaddr offset,
                                   unsigned size);
static void imx_sim0_write(void *opaque, hwaddr offset,
                            uint64_t value, unsigned size);

static const VMStateDescription imx_sim0_vm = {
    .name = TYPE_IMX_SIM0,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(gpr0, imx_sim0_state),
        VMSTATE_UINT32(gpr1, imx_sim0_state),
        VMSTATE_UINT32(dgo_ctrl0, imx_sim0_state),
        VMSTATE_UINT32(dgo_ctrl1, imx_sim0_state),
        VMSTATE_UINT32_ARRAY(dgo_gp, imx_sim0_state, 9),
        VMSTATE_UINT32(sysctrl0, imx_sim0_state),
        VMSTATE_UINT32(ssram_acc_dis, imx_sim0_state),
        VMSTATE_UINT32(rtd_sysctrl0, imx_sim0_state),
        VMSTATE_UINT32(lpav_per_dom_ctrl, imx_sim0_state),
        VMSTATE_UINT32(lpav_mst_alo_ctrl, imx_sim0_state),
        VMSTATE_UINT32(lpav_slv_alo_ctrl, imx_sim0_state),
        VMSTATE_END_OF_LIST()
    }
};

static const MemoryRegionOps imx_sim0_ops = {
    .read = imx_sim0_read,
    .write = imx_sim0_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static uint64_t imx_sim0_read(void *opaque, hwaddr offset,
                                   unsigned size)
{
    imx_sim0_state *sim0 = (imx_sim0_state *)opaque;
    uint64_t ret = 0;
    if (offset == 0x8) {
        ret = 0xFFFFFFFF;
    } else if ((offset >= 0x10) && (offset < 0x34)) {
        ret = sim0->dgo_gp[(offset - 0x10) / 4];
    } else {
        ret = 0;
    }
    return ret;
}

static void imx_sim0_write(void *opaque, hwaddr offset,
                            uint64_t value, unsigned size)
{
    imx_sim0_state *sim0 = (imx_sim0_state *)opaque;

    if ((offset >= 0x10) && (offset < 0x34)) {
        sim0->dgo_gp[(offset - 0x10) / 4] = value;
    }
}

static void imx_sim0_init(Object *obj)
{
    imx_sim0_state *s = IMX_SIM0(obj);

    memory_region_init_io(&s->iomem, obj, &imx_sim0_ops, s, TYPE_IMX_SIM0, 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->iomem);
}

static void imx_sim0_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->vmsd = &imx_sim0_vm;
}

static const TypeInfo imx_sim0_info = {
    .name = TYPE_IMX_SIM0,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(imx_sim0_state),
    .instance_init = imx_sim0_init,
    .class_init = imx_sim0_class_init,
};

static void imx_sim0_types(void)
{
    type_register_static(&imx_sim0_info);
}

type_init(imx_sim0_types)


/*=======================================
    TSTMR Module Start
 ========================================*/
static const VMStateDescription imx_tstmr_vm = {
    .name = TYPE_IMX_TSTMR,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(tstmr_l, imx_tstmr_state),
        VMSTATE_UINT32(tstmr_h, imx_tstmr_state),
        VMSTATE_END_OF_LIST()
    }
};

static uint64_t imx_tstmr_read(void *opaque, hwaddr offset,
                                   unsigned size)
{
    uint64_t ret = 0;
    imx_tstmr_state *s = (imx_tstmr_state *)opaque;
    switch(offset) {
    case 0:
        ret = s->tstmr_l;
        s->tstmr_l++;
        break;
    case 4:
        break;
    }
    return ret;
}

static const MemoryRegionOps imx_tstmr_ops = {
    .read = imx_tstmr_read,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void imx_tstmr_init(Object *obj)
{

    imx_tstmr_state *s = IMX_TSTMR(obj);

    memory_region_init_io(&s->iomem, obj, &imx_tstmr_ops, s, TYPE_IMX_TSTMR, 0x400);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->iomem);
}


static void imx_tstmr_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->vmsd = &imx_tstmr_vm;
}

static const TypeInfo imx_tstmr_info = {
    .name          = TYPE_IMX_TSTMR,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(imx_fsb_state),
    .instance_init = imx_tstmr_init,
    .class_init    = imx_tstmr_class_init,
};

static void imx_tstmr_types(void)
{
    type_register_static(&imx_tstmr_info);
}

type_init(imx_tstmr_types)

/*=======================================
    ARG Module Start
 ========================================*/
imx8ulp_arg_t g_imx8ulp_arg;

static void imx8ulp_arg_handle_fuse(char *fuse_str);
static uint32_t imx8ulp_arg_hex2dec(char *hex);

void imx_fsb_init_cb(imx_fsb_state *fsb)
{
    uint32_t i = 0;
    /* BANK3 8 words*/
    for (i = 0; i < 8; i++) {
        fsb->reg[0 + i] = g_imx8ulp_arg.fuse[24 + i];
    }

    /* BANK4 8 words */
    for (i = 0; i < 8; i++) {
        fsb->reg[8 + i] = g_imx8ulp_arg.fuse[32 + i];
    }

    /* BANK5 8 words */
    for (i = 0; i < 8; i++) {
        fsb->reg[64 + i] = g_imx8ulp_arg.fuse[40 + i];
    }

    /* BANK6 8 words */
    for (i = 0; i < 8; i++) {
        fsb->reg[72 + i] = g_imx8ulp_arg.fuse[48 + i];
    }

    /* BANK28-31 32 words */
    for (i = 0; i < 32; i++) {
        //printf("%s: m33 rom patch:%x\n", __func__, g_imx8ulp_arg.fuse[224 + i]);
        fsb->reg[96 + i] = g_imx8ulp_arg.fuse[224 + i];
    }

    /* BANK37-44 64 words */
    for (i = 0; i < 64; i++) {
        fsb->reg[128 + i] = g_imx8ulp_arg.fuse[296 + i];
    }
}

static uint32_t imx8ulp_arg_hex2dec(char *hex)
{
    uint32_t ret = 0;
    uint32_t len = strlen(hex);
    uint32_t i = 0;
    for (i = 2; i < len; i++) {
        if ((hex[i] >= '0') && (hex[i] <= '9'))
            ret = ret * 16 + (hex[i] - '0');
        else
            ret = ret * 16 + (hex[i] - 'a' + 10);
    }
    return ret;
}

static void imx8ulp_arg_handle_fuse(char *fuse_str)
{
    char *key;
    char *value;
    key = strtok(fuse_str, "=");
    value = strtok(NULL, "\n");
    char idx_buf[12];
    uint32_t fuse_pos;
    uint32_t pos_idx = 0;

    while (*key != '_') {
        if ((*key >= '0') && (*key <= '9')) {
            idx_buf[pos_idx++] = *key;
        }
        key++;
    }
    idx_buf[pos_idx] = 0;
    fuse_pos = atoi(idx_buf) * 8;

    pos_idx = 0;
    while (*key != 0) {
        if ((*key >= '0') && (*key <= '9')) {
            idx_buf[pos_idx++] = *key;
        }
        key++;
    }
    idx_buf[pos_idx] = 0;
    fuse_pos += atoi(idx_buf);
    g_imx8ulp_arg.fuse[fuse_pos] = imx8ulp_arg_hex2dec(value);
}

static void imx8ulp_arg_parse(void)
{
    FILE *arg = fopen("run.arg", "r");
    char buf[ARG_BUF_SIZE] = {0};
    char *key;
    char *value;

    printf("%s entry\n", __func__);

    while (!feof(arg)) {
        fgets(buf, ARG_BUF_SIZE, arg);
        key = strtok(buf, "=");
        value = strtok(NULL, "\n");

        if (strcmp(key, "C_ARG +") == 0) {
            imx8ulp_arg_handle_fuse(value);
        }

        if (value != NULL) {
            if (strstr(value, "+BOOT_INTERNAL") != NULL) {
                g_imx8ulp_arg.bt_mode = 2;
                printf("Boot Mode is %x\n", g_imx8ulp_arg.bt_mode);
            } else if (strstr(value, "BT_CFG_PIN_M33") != NULL) {
                key = strtok(value, "=");
                value = strtok(NULL, "\n");
                g_imx8ulp_arg.m33_bt_cfg = imx8ulp_arg_hex2dec(value);
                printf("M33 BT CFG:%x\n", g_imx8ulp_arg.m33_bt_cfg);
            } else if (strstr(value, "BT_CFG_PIN_A35") != NULL) {
                key = strtok(value, "=");
                value = strtok(NULL, "\n");
                g_imx8ulp_arg.a35_bt_cfg = imx8ulp_arg_hex2dec(value);
                printf("A35 BT CFG:%x\n", g_imx8ulp_arg.a35_bt_cfg);
            }
        }

    }
}

/*=======================================
    IMX8ULP CM33 CORE module Start
 ========================================*/
static qemu_irq get_sse_irq_in(IMX8ULP_M33_MachineState *mms, int irqno)
{

    assert(irqno < IMX8ULP_M33_NUMIRQ);

    return qdev_get_gpio_in_named(DEVICE(&mms->iotkit), "EXP_IRQ", irqno);
}

static MemoryRegion *make_unimp_dev(IMX8ULP_M33_MachineState *mms,
        void *opaque,
        const char *name, hwaddr size)
{
    /* Initialize, configure and realize a TYPE_UNIMPLEMENTED_DEVICE,
     * and return a pointer to its MemoryRegion.
     */
    UnimplementedDeviceState *uds = opaque;

    sysbus_init_child_obj(OBJECT(mms), name, uds,
            sizeof(UnimplementedDeviceState),
            TYPE_UNIMPLEMENTED_DEVICE);
    qdev_prop_set_string(DEVICE(uds), "name", name);
    qdev_prop_set_uint64(DEVICE(uds), "size", size);
    object_property_set_bool(OBJECT(uds), true, "realized", &error_fatal);
    return sysbus_mmio_get_region(SYS_BUS_DEVICE(uds), 0);
}

static MemoryRegion *make_mpc(IMX8ULP_M33_MachineState *mms, void *opaque,
        const char *name, hwaddr size)
{
    TZMPC *mpc = opaque;
    int i = mpc - &mms->ssram_mpc[0];
    MemoryRegion *ssram = &mms->ssram[i];
    MemoryRegion *upstream;
    char *mpcname = g_strdup_printf("%s-mpc", name);
    static uint32_t ramsize[] = {
        0x00030000,
        0x00080000,
        0x00040000,
    };
    static uint32_t rambase[] = {
        0x10000000,
        0x30000000,
        0x1ffc0000,
    };

    memory_region_init_ram(ssram, NULL, name, ramsize[i], &error_fatal);
    //memory_region_init_ram_device_ptr(ssram, NULL, name, ramsize[i], &error_fatal);

    sysbus_init_child_obj(OBJECT(mms), mpcname, mpc, sizeof(mms->ssram_mpc[0]),
            TYPE_TZ_MPC);
    object_property_set_link(OBJECT(mpc), OBJECT(ssram),
            "downstream", &error_fatal);
    object_property_set_bool(OBJECT(mpc), true, "realized", &error_fatal);
    /* Map the upstream end of the MPC into system memory */
    upstream = sysbus_mmio_get_region(SYS_BUS_DEVICE(mpc), 1);
    memory_region_add_subregion(get_system_memory(), rambase[i], upstream);
    /* and connect its interrupt to the IoTKit */
    qdev_connect_gpio_out_named(DEVICE(mpc), "irq", 0,
            qdev_get_gpio_in_named(DEVICE(&mms->iotkit),
                "mpcexp_status", i));

    g_free(mpcname);
    /* Return the register interface MR for our caller to map behind the PPC */
    return sysbus_mmio_get_region(SYS_BUS_DEVICE(mpc), 0);
}

static MemoryRegion *make_spi(IMX8ULP_M33_MachineState *mms, void *opaque,
        const char *name, hwaddr size)
{
    PL022State *spi = opaque;
    int i = spi - &mms->spi[0];
    SysBusDevice *s;

    sysbus_init_child_obj(OBJECT(mms), name, spi, sizeof(mms->spi[0]),
            TYPE_PL022);
    object_property_set_bool(OBJECT(spi), true, "realized", &error_fatal);
    s = SYS_BUS_DEVICE(spi);
    sysbus_connect_irq(s, 0, get_sse_irq_in(mms, 51 + i));
    return sysbus_mmio_get_region(s, 0);
}

static void imx8ulp_m33_common_init(MachineState *machine)
{
    IMX8ULP_M33_MachineState *mms = IMX8ULP_MACHINE(machine);
    IMX8ULP_MachineClass *mmc = IMX8ULP_MACHINE_GET_CLASS(mms);
    MachineClass *mc = MACHINE_GET_CLASS(machine);
    MemoryRegion *system_memory = get_system_memory();
    DeviceState *iotkitdev;
    DeviceState *dev_splitter;
    int i;

    if (strcmp(machine->cpu_type, mc->default_cpu_type) != 0) {
        error_report("This board can only be used with CPU %s",
                mc->default_cpu_type);
        exit(1);
    }

    sysbus_init_child_obj(OBJECT(machine), "iotkit", &mms->iotkit,
            sizeof(mms->iotkit), mmc->armsse_type);
    iotkitdev = DEVICE(&mms->iotkit);
    object_property_set_link(OBJECT(&mms->iotkit), OBJECT(system_memory),
            "memory", &error_abort);
    qdev_prop_set_uint32(iotkitdev, "EXP_NUMIRQ", IMX8ULP_M33_NUMIRQ);
    qdev_prop_set_uint32(iotkitdev, "MAINCLK", SYSCLK_FRQ);
    object_property_set_bool(OBJECT(&mms->iotkit), true, "realized",
            &error_fatal);


    /* The sec_resp_cfg output from the IoTKit must be split into multiple
     * lines, one for each of the PPCs we create here, plus one per MSC.
     */
    object_initialize_child(OBJECT(machine), "sec-resp-splitter",
            &mms->sec_resp_splitter,
            sizeof(mms->sec_resp_splitter),
            TYPE_SPLIT_IRQ, &error_abort, NULL);
    object_property_set_int(OBJECT(&mms->sec_resp_splitter),
            ARRAY_SIZE(mms->ppc) + ARRAY_SIZE(mms->msc),
            "num-lines", &error_fatal);
    object_property_set_bool(OBJECT(&mms->sec_resp_splitter), true,
            "realized", &error_fatal);
    dev_splitter = DEVICE(&mms->sec_resp_splitter);
    qdev_connect_gpio_out_named(iotkitdev, "sec_resp_cfg", 0,
            qdev_get_gpio_in(dev_splitter, 0));

    /* The IoTKit sets up much of the memory layout, including
     * the aliases between secure and non-secure regions in the
     * address space. The FPGA itself contains:
     *
     * 0x00000000..0x003fffff  SSRAM1
     * 0x00400000..0x007fffff  alias of SSRAM1
     * 0x28000000..0x283fffff  4MB SSRAM2 + SSRAM3
     * 0x40100000..0x4fffffff  AHB Master Expansion 1 interface devices
     * 0x80000000..0x80ffffff  16MB PSRAM
     */

    memory_region_allocate_system_memory(&mms->psram,
            NULL, "mps.ram", 16 * MiB);
    memory_region_add_subregion(system_memory, 0x80000000, &mms->psram);

    /* The overflow IRQs for all UARTs are ORed together.
     * Tx, Rx and "combined" IRQs are sent to the NVIC separately.
     * Create the OR gate for this.
     */
    object_initialize_child(OBJECT(mms), "uart-irq-orgate",
            &mms->uart_irq_orgate, sizeof(mms->uart_irq_orgate),
            TYPE_OR_IRQ, &error_abort, NULL);
    object_property_set_int(OBJECT(&mms->uart_irq_orgate), 10, "num-lines",
            &error_fatal);
    object_property_set_bool(OBJECT(&mms->uart_irq_orgate), true,
            "realized", &error_fatal);
    qdev_connect_gpio_out(DEVICE(&mms->uart_irq_orgate), 0,
            get_sse_irq_in(mms, 15));

    /* Most of the devices in the FPGA are behind Peripheral Protection
     * Controllers. The required order for initializing things is:
     *  + initialize the PPC
     *  + initialize, configure and realize downstream devices
     *  + connect downstream device MemoryRegions to the PPC
     *  + realize the PPC
     *  + map the PPC's MemoryRegions to the places in the address map
     *    where the downstream devices should appear
     *  + wire up the PPC's control lines to the IoTKit object
     */

    const PPCInfo ppcs[] = { {
        .name = "apb_ppcexp0",
        .ports = {
            {"cgc0", make_unimp_dev, &mms->cgc0, IMX_CGC0_START, 0x1000},
            {"pcc0", make_unimp_dev, &mms->pcc0, IMX_PCC0_START, 0x1000},
            {"pcc1", make_unimp_dev, &mms->pcc1, IMX_PCC1_START, 0x1000},
            {"romcp0", make_unimp_dev, &mms->romcp0, IMX_ROMCP0_START, 0x1000},
            {"upower", make_unimp_dev, &mms->upower, 0x28350000, 0x1000},
            { "ssram-0", make_mpc, &mms->ssram_mpc[0], 0x58007000, 0x1000 },
            { "ssram-1", make_mpc, &mms->ssram_mpc[1], 0x58008000, 0x1000 },
            { "ssram-2", make_mpc, &mms->ssram_mpc[2], 0x58009000, 0x1000 },
        },
    }, {
        .name = "apb_ppcexp1",
        .ports = {
            { "spi0", make_spi, &mms->spi[0], 0x40205000, 0x1000 },
            { "i2c0", make_unimp_dev, &mms->i2c[0], 0x40207000, 0x1000 },
        },
    },
    };

    for (i = 0; i < ARRAY_SIZE(ppcs); i++) {
        const PPCInfo *ppcinfo = &ppcs[i];
        TZPPC *ppc = &mms->ppc[i];
        DeviceState *ppcdev;
        int port;
        char *gpioname;

        sysbus_init_child_obj(OBJECT(machine), ppcinfo->name, ppc,
                sizeof(TZPPC), TYPE_TZ_PPC);
        ppcdev = DEVICE(ppc);

        for (port = 0; port < TZ_NUM_PORTS; port++) {
            const PPCPortInfo *pinfo = &ppcinfo->ports[port];
            MemoryRegion *mr;
            char *portname;

            if (!pinfo->devfn) {
                continue;
            }

            mr = pinfo->devfn(mms, pinfo->opaque, pinfo->name, pinfo->size);
            portname = g_strdup_printf("port[%d]", port);
            object_property_set_link(OBJECT(ppc), OBJECT(mr),
                    portname, &error_fatal);
            g_free(portname);
        }

        object_property_set_bool(OBJECT(ppc), true, "realized", &error_fatal);

        for (port = 0; port < TZ_NUM_PORTS; port++) {
            const PPCPortInfo *pinfo = &ppcinfo->ports[port];

            if (!pinfo->devfn) {
                continue;
            }
            sysbus_mmio_map(SYS_BUS_DEVICE(ppc), port, pinfo->addr);

            gpioname = g_strdup_printf("%s_nonsec", ppcinfo->name);
            qdev_connect_gpio_out_named(iotkitdev, gpioname, port,
                    qdev_get_gpio_in_named(ppcdev,
                        "cfg_nonsec",
                        port));
            g_free(gpioname);
            gpioname = g_strdup_printf("%s_ap", ppcinfo->name);
            qdev_connect_gpio_out_named(iotkitdev, gpioname, port,
                    qdev_get_gpio_in_named(ppcdev,
                        "cfg_ap", port));
            g_free(gpioname);
        }

        gpioname = g_strdup_printf("%s_irq_enable", ppcinfo->name);
        qdev_connect_gpio_out_named(iotkitdev, gpioname, 0,
                qdev_get_gpio_in_named(ppcdev,
                    "irq_enable", 0));
        g_free(gpioname);
        gpioname = g_strdup_printf("%s_irq_clear", ppcinfo->name);
        qdev_connect_gpio_out_named(iotkitdev, gpioname, 0,
                qdev_get_gpio_in_named(ppcdev,
                    "irq_clear", 0));
        g_free(gpioname);
        gpioname = g_strdup_printf("%s_irq_status", ppcinfo->name);
        qdev_connect_gpio_out_named(ppcdev, "irq", 0,
                qdev_get_gpio_in_named(iotkitdev,
                    gpioname, 0));
        g_free(gpioname);

        qdev_connect_gpio_out(dev_splitter, i,
                qdev_get_gpio_in_named(ppcdev,
                    "cfg_sec_resp", 0));
    }

    imx8ulp_arg_parse();
    /* create the verilog debug */
    sysbus_create_simple(TYPE_VERILOG_DEBUG, VERILOG_DEBUG_START, NULL);
    sysbus_create_simple(TYPE_IMX_FSB, IMX_FSB_START, NULL);
    sysbus_create_simple(TYPE_IMX_S400_MU, IMX_S400_MU_START, NULL);
    sysbus_create_simple(TYPE_IMX_SIM0, IMX_SIM0_S_START, NULL);
    sysbus_create_simple(TYPE_IMX_TSTMR, IMX_TSTMR_START, NULL);

    memory_region_allocate_system_memory(&mms->cmc0, NULL, "cmc0.ram", 0x1000);
    memory_region_add_subregion(system_memory, IMX_CMC0_START, &mms->cmc0);
    uint32_t tmp = (g_imx8ulp_arg.bt_mode << 30) | (g_imx8ulp_arg.m33_bt_cfg);
    cpu_physical_memory_write(IMX_CMC0_START + 0xA0, &tmp, 4);

    memory_region_allocate_system_memory(&mms->fsb_low, NULL, "fsb_low.ram", 0x800);
    memory_region_add_subregion(system_memory, IMX_FSB_LOW_START, &mms->fsb_low);
    tmp = 2;
    cpu_physical_memory_write(IMX_FSB_LOW_START + 0x41c, &tmp, 4);


    armv7m_load_kernel(ARM_CPU(first_cpu), machine->kernel_filename, 0x400000);
}

static void imx8ulp_m33_idau_check(IDAUInterface *ii, uint32_t address,
        int *iregion, bool *exempt, bool *ns, bool *nsc)
{
    /*
     * The MPS2 TZ FPGA images have IDAUs in them which are connected to
     * the Master Security Controllers. Thes have the same logic as
     * is used by the IoTKit for the IDAU connected to the CPU, except
     * that MSCs don't care about the NSC attribute.
     */
    int region = extract32(address, 28, 4);

    *ns = !(region & 1);
    *nsc = false;
    /* 0xe0000000..0xe00fffff and 0xf0000000..0xf00fffff are exempt */
    *exempt = (address & 0xeff00000) == 0xe0000000;
    *iregion = region;
}

static void imx8ulp_class_init(ObjectClass *oc, void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);
    IDAUInterfaceClass *iic = IDAU_INTERFACE_CLASS(oc);

    mc->init = imx8ulp_m33_common_init;
    iic->check = imx8ulp_m33_idau_check;
}

static void imx8ulp_m33_class_init(ObjectClass *oc, void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);
    IMX8ULP_MachineClass *mmc = IMX8ULP_MACHINE_CLASS(oc);

    mc->desc = "ARM MPS2 with AN505 FPGA image for Cortex-M33";
    mc->default_cpus = 1;
    mc->min_cpus = mc->default_cpus;
    mc->max_cpus = mc->default_cpus;
    mc->default_cpu_type = ARM_CPU_TYPE_NAME("cortex-m33");
    mmc->armsse_type = TYPE_IOTKIT;
}

static const TypeInfo imx8ulp_info = {
    .name = TYPE_IMX8ULP_MACHINE,
    .parent = TYPE_MACHINE,
    .abstract = true,
    .instance_size = sizeof(IMX8ULP_M33_MachineState),
    .class_size = sizeof(IMX8ULP_MachineClass),
    .class_init = imx8ulp_class_init,
    .interfaces = (InterfaceInfo[]) {
        { TYPE_IDAU_INTERFACE },
        { }
    },
};

static const TypeInfo imx8ulp_m33_info = {
    .name = TYPE_IMX8ULP_M33_MACHINE,
    .parent = TYPE_IMX8ULP_MACHINE,
    .class_init = imx8ulp_m33_class_init,
};

static void imx8ulp_machine_init(void)
{
    type_register_static(&imx8ulp_info);
    type_register_static(&imx8ulp_m33_info);
}

type_init(imx8ulp_machine_init);
