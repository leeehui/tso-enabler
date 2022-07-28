#include <linux/module.h>    // included for all kernel modules
#include <linux/kernel.h>    // included for KERN_INFO
#include <linux/init.h>      // included for __init and __exit macros
#include <linux/cpumask.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lakshmanan, zhuowei, bgb");
MODULE_DESCRIPTION("A Simple module for enabling TSO for AsahiLinux");

static unsigned int cpu_mask = 0xFF; // default for all cores
static unsigned long sys_apl_hid9_el1_mask = (1UL << 16)|(1UL << 26)|(1UL << 29);
static unsigned long sys_apl_ehid10_el1_mask = (1UL << 49);
static unsigned int cpu_mask_valid; 
static struct cpumask mask;

#define ACTLR_EL1_EnTSO   (1ULL << 1)


#ifdef __ASSEMBLER__

#define _sys_reg(op0, op1, CRn, CRm, op2) s##op0##_##op1##_c##CRn##_c##CRm##_##op2

#else

#define _sys_reg(op0, op1, CRn, CRm, op2) , _S, op0, op1, CRn, CRm, op2

#endif

#define SYS_APL_HID9_EL1    _sys_reg(3,0,15,9,0)
#define SYS_APL_EHID10_EL1  _sys_reg(3,0,15,10,1)

#define sysop(op) __asm__ volatile(op:::"memory")
#define _concat(a, _1, b, ...) a##b

#define _sr_tkn_S(_0, _1, op0, op1, CRn, CRm, op2) s##op0##_##op1##_c##CRn##_c##CRm##_##op2

#define _sr_tkn(a) a

#define sr_tkn(...) _concat(_sr_tkn, __VA_ARGS__, )(__VA_ARGS__)

#define __mrs(reg)                                                                                 \
    ({                                                                                             \
        u64 val;                                                                                   \
        __asm__ volatile("mrs\t%0, " #reg : "=r"(val));                                            \
        val;                                                                                       \
    })
#define _mrs(reg) __mrs(reg)

#define __msr(reg, val)                                                                            \
    ({                                                                                             \
        u64 __val = (u64)val;                                                                      \
        __asm__ volatile("msr\t" #reg ", %0" : : "r"(__val));                                      \
    })
#define _msr(reg, val) __msr(reg, val)

#define mrs(reg)      _mrs(sr_tkn(reg))
#define msr(reg, val) _msr(sr_tkn(reg), val)
#define msr_sync(reg, val)                                                                         \
    ({                                                                                             \
        _msr(sr_tkn(reg), val);                                                                    \
        sysop("isb");                                                                              \
    })

#define reg_clr(reg, bits)      _msr(sr_tkn(reg), _mrs(sr_tkn(reg)) & ~(bits))
#define reg_set(reg, bits)      _msr(sr_tkn(reg), _mrs(sr_tkn(reg)) | bits)
#define reg_mask(reg, clr, set) _msr(sr_tkn(reg), (_mrs(sr_tkn(reg)) & ~(clr)) | set)

#define reg_clr_sync(reg, bits)                                                                    \
    ({                                                                                             \
        reg_clr(sr_tkn(reg), bits);                                                                \
        sysop("isb");                                                                              \
    })
#define reg_set_sync(reg, bits)                                                                    \
    ({                                                                                             \
        reg_set(sr_tkn(reg), bits);                                                                \
        sysop("isb");                                                                              \
    })
#define reg_mask_sync(reg, clr, set)                                                               \
    ({                                                                                             \
        reg_mask(sr_tkn(reg), clr, set);                                                           \
        sysop("isb");                                                                              \
    })



static void pokeit(void* turnOn) {
	u64 mpidr = read_sysreg(mpidr_el1);
    u64 hid;
    if (mpidr & 0x100) {
        hid = mrs(SYS_APL_HID9_EL1);
        printk(KERN_INFO "SYS_APL_HID9_EL1: 0x%llx\n", hid);
	    if (turnOn) {
            hid |= sys_apl_hid9_el1_mask;
        } else {
            hid &= ~sys_apl_hid9_el1_mask;
        }
        msr_sync(SYS_APL_HID9_EL1, hid);
    } else {
        hid = mrs(SYS_APL_EHID10_EL1);
        printk(KERN_INFO "SYS_APL_EHID10_EL1: 0x%llx\n", hid);
	    if (turnOn) {
            hid |= sys_apl_ehid10_el1_mask;
        } else {
            hid &= ~sys_apl_ehid10_el1_mask;
        }
        msr_sync(SYS_APL_EHID10_EL1, hid);
    }
}

static int __init tsoenabler_subcfg_init(void)
{
    int i;
    unsigned int cpu_mask_original = cpu_mask;
    printk(KERN_INFO "NR_CPUS: %d, cpu_mask: 0x%x\n", NR_CPUS, cpu_mask);
    for (i = 0; i < NR_CPUS && cpu_mask != 0 ; i++, cpu_mask >>= 1) {
        if (cpumask_test_cpu(i, cpu_online_mask)) {
            if (!test_bit(i, (const unsigned long *)&cpu_mask_original)) 
                continue;
            cpumask_set_cpu(i, &mask);
            cpu_mask_valid |= BIT(i);
        } else {
            printk(KERN_WARNING "cpu num %d not in cpu_online_mask, ignore!\n", i);
        }
    }
    printk(KERN_INFO "Enabling TSO subcfg for cores(mask): 0x%x!\n", cpu_mask_valid);
    on_each_cpu_mask(&mask, pokeit, "yes", true);
    return 0;    // Non-zero return means that the module couldn't be loaded.
}

static void __exit tsoenabler_subcfg_cleanup(void)
{
    on_each_cpu_mask(&mask, pokeit, NULL, true);
    printk(KERN_INFO "Disabling TSO subcfg for cores(mask): 0x%x!\n", cpu_mask_valid);
}

module_param(cpu_mask, uint, S_IRUGO);

module_init(tsoenabler_subcfg_init);
module_exit(tsoenabler_subcfg_cleanup);
