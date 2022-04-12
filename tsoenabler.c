#include <linux/module.h>    // included for all kernel modules
#include <linux/kernel.h>    // included for KERN_INFO
#include <linux/init.h>      // included for __init and __exit macros
#include <linux/cpumask.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lakshmanan, zhuowei, bgb");
MODULE_DESCRIPTION("A Simple module for enabling TSO for AsahiLinux");

static unsigned int cpu_mask = 0xFF; // default for all cores
static unsigned int cpu_mask_valid; 
static struct cpumask mask;

#define ACTLR_EL1_EnTSO   (1ULL << 1)

static void pokeit(void* turnOn) {
	u64 actlr = read_sysreg(actlr_el1);
	if (turnOn) {
		actlr |= ACTLR_EL1_EnTSO;
	} else {
		actlr &= ~ACTLR_EL1_EnTSO;
	}
	write_sysreg(actlr, actlr_el1);
}

static int __init tsoenabler_init(void)
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
    printk(KERN_INFO "Enabling TSO for cores(mask): 0x%x!\n", cpu_mask_valid);
    on_each_cpu_mask(&mask, pokeit, "yes", true);
    return 0;    // Non-zero return means that the module couldn't be loaded.
}

static void __exit tsoenabler_cleanup(void)
{
    on_each_cpu_mask(&mask, pokeit, NULL, true);
    printk(KERN_INFO "Disabling TSO for cores(mask): 0x%x!\n", cpu_mask_valid);
}

module_param(cpu_mask, uint, S_IRUGO);

module_init(tsoenabler_init);
module_exit(tsoenabler_cleanup);
