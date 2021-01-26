#include "qemu/osdep.h"
#include "qapi/error.h"
#include "hw/arm/boot.h"
#include "hw/boards.h"
#include "qemu/log.h"
#include "exec/address-spaces.h"
#include "sysemu/sysemu.h"
#include "hw/arm/armv7m.h"
#include "hw/char/pl011.h"
#include "hw/irq.h"
#include "migration/vmstate.h"
#include "cpu.h"

#define TYPE_TEST_IP "my_test_ip"

#define MY_SOC_FLASH_START  (0x0)
#define MY_SOC_FLASH_SIZE   (4 * 1024 * 1024) //< 4M

#define MY_SOC_SRAM_START    (0x20000000)
#define MY_SOC_SRAM_SIZE     (16 * 1024 * 1024) //<16M

#define PL011_UART0_START   (0x40000000)
#define PL011_UART0_IRQn    (0)

#define MY_TEST_IP_START    (0x40001000)

#define NUM_IRQ_LINES 64

typedef struct {
    SysBusDevice parent_obj;

    qemu_irq irq;
    MemoryRegion iomem;
    uint32_t id0;       //T
    uint32_t id1;       //E
    uint32_t id2;       //S
    uint32_t id3;       //T
    uint32_t id4;       //
    uint32_t id5;       //I
    uint32_t id6;       //P
    uint32_t test_reg;
} my_test_ip_state;

my_test_ip_state test_ip;

static void mysoc_init(MachineState *ms)
{
    DeviceState *nvic;

    MemoryRegion *sram = g_new(MemoryRegion, 1);
    MemoryRegion *flash = g_new(MemoryRegion, 1);
    MemoryRegion *system_memory = get_system_memory();

    /* Flash programming is done via the SCU, so pretend it is ROM.  */
    memory_region_init_ram(flash, NULL, "mysoc.flash", MY_SOC_FLASH_SIZE, &error_fatal);
    memory_region_set_readonly(flash, true);
    memory_region_add_subregion(system_memory, MY_SOC_FLASH_START, flash);

    memory_region_init_ram(sram, NULL, "mysoc.sram", MY_SOC_SRAM_SIZE, &error_fatal);
    memory_region_add_subregion(system_memory, MY_SOC_SRAM_START, sram);

    nvic = qdev_create(NULL, TYPE_ARMV7M);
    qdev_prop_set_uint32(nvic, "num-irq", NUM_IRQ_LINES);
    qdev_prop_set_string(nvic, "cpu-type", ms->cpu_type);
    qdev_prop_set_bit(nvic, "enable-bitband", true);
    object_property_set_link(OBJECT(nvic), OBJECT(get_system_memory()), "memory", &error_abort);

    /* This will exit with an error if the user passed us a bad cpu_type */
    qdev_init_nofail(nvic);

    pl011_luminary_create(PL011_UART0_START , qdev_get_gpio_in(nvic, PL011_UART0_IRQn), serial_hd(0));

    sysbus_create_simple(TYPE_TEST_IP, MY_TEST_IP_START, NULL);

    armv7m_load_kernel(ARM_CPU(first_cpu), ms->kernel_filename, MY_SOC_FLASH_SIZE);
}

static void mysoc_class_init(ObjectClass *oc, void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);
    printf("%s entry\n", __func__);

    mc->desc = "My SOC Cortex M4";
    mc->init = mysoc_init;
    mc->ignore_memory_transaction_failures = true;
    mc->default_cpu_type = ARM_CPU_TYPE_NAME("cortex-m4");
}

static const TypeInfo mysoc_type = {
    .name = MACHINE_TYPE_NAME("mysoc_evb"),
    .parent = TYPE_MACHINE,
    .class_init = mysoc_class_init,
};

static void mysoc_evb_init(void)
{
    type_register_static(&mysoc_type);
}

type_init(mysoc_evb_init)


/* A custom char dev */

#define TEST_IP(obj) \
    OBJECT_CHECK(my_test_ip_state, (obj), TYPE_TEST_IP)


static const VMStateDescription my_test_ip_vm = {
    .name = "my_test_ip",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(id0, my_test_ip_state),
        VMSTATE_UINT32(id1, my_test_ip_state),
        VMSTATE_UINT32(id2, my_test_ip_state),
        VMSTATE_UINT32(id3, my_test_ip_state),
        VMSTATE_UINT32(id4, my_test_ip_state),
        VMSTATE_UINT32(id5, my_test_ip_state),
        VMSTATE_UINT32(id6, my_test_ip_state),
        VMSTATE_UINT32(test_reg, my_test_ip_state),
        VMSTATE_END_OF_LIST()
    }
};

static uint64_t my_test_ip_read(void *opaque, hwaddr offset,
                                   unsigned size)
{
    uint64_t ret = 0;
    my_test_ip_state *s = (my_test_ip_state *)opaque;
    printf("%s hwaddr:%lx, size:%x\n", __func__, offset, size);
    switch (offset) {
    case 0x0:
        ret = s->id0;
        break;
    case 0x4:
        ret = s->id1;
        break;
    case 0x8:
        ret = s->id2;
        break;
    case 0xc:
        ret = s->id3;
        break;
    case 0x10:
        ret = s->id4;
        break;
    case 0x14:
        ret = s->id5;
        break;
    case 0x18:
        ret = s->id6;
        break;
    case 0x1c:
        ret = s->test_reg;
        break;
    }
    return ret;
}

static void my_test_ip_write(void *opaque, hwaddr offset,
                                uint64_t value, unsigned size)
{

    my_test_ip_state *s = (my_test_ip_state *)opaque;
    printf("%s hwaddr:%lx, size:%x, value:%lx\n", __func__, offset, size, value);
    switch(offset){
    case 0x0:
    case 0x4:
    case 0x8:
    case 0xc:
    case 0x10:
    case 0x14:
    case 0x18:
        printf("%s: cannot write the read only register\n", __func__);
        break;
    case 0x1c:
        s->test_reg = value;
        break;
    }
}

static const MemoryRegionOps my_test_ip_ops = {
    .read = my_test_ip_read,
    .write = my_test_ip_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void my_test_ip_init(Object *obj)
{
    printf("%s \n", __func__);

    my_test_ip_state *s = TEST_IP(obj);

    memory_region_init_io(&s->iomem, obj, &my_test_ip_ops, s, TYPE_TEST_IP, 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->iomem);

    s->id0 = 0x54;
    s->id1 = 0x45;
    s->id2 = 0x53;
    s->id3 = 0x54;
    s->id4 = 0x20;
    s->id5 = 0x49;
    s->id6 = 0x50;
    s->test_reg = 0;
}

static void my_test_ip_class_init(ObjectClass *klass, void *data)
{
    printf("%s \n", __func__);
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->vmsd = &my_test_ip_vm;
}

static const TypeInfo my_test_ip = {
    .name          = TYPE_TEST_IP,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(my_test_ip_state),
    .instance_init = my_test_ip_init,
    .class_init    = my_test_ip_class_init,
};

static void my_test_ip_types(void)
{
    printf("%s \n", __func__);
    type_register_static(&my_test_ip);
}

type_init(my_test_ip_types)
