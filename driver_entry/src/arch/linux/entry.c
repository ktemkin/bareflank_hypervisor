
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/vmalloc.h>
#include <linux/kallsyms.h>
#include <linux/cpumask.h>
#include <linux/sched.h>

#include <debug.h>
#include <common.h>
#include <platform.h>
#include <driver_entry_interface.h>

/* ========================================================================== */
/* Global                                                                     */
/* ========================================================================== */

int32_t g_module_length = 0;

int32_t g_num_files = 0;
char *files[MAX_NUM_MODULES] = {0};

typedef long (*set_affinity_fn)(pid_t, const struct cpumask *);
static set_affinity_fn set_cpu_affinity;

typedef void (*vmalloc_sync_all_fn)(void);
static vmalloc_sync_all_fn __vmalloc_sync_all;

/* The memory management context for the hypervisor. */
struct mm_struct * g_mmu_context = NULL;

/* ========================================================================== */
/* Misc Device                                                                */
/* ========================================================================== */

static int
dev_open(struct inode *inode, struct file *file)
{
    DEBUG("dev_open succeeded\n");
    return 0;
}

static int
dev_release(struct inode *inode, struct file *file)
{
    DEBUG("dev_release succeeded\n");
    return 0;
}

int32_t
ioctl_add_module(char *file)
{
    char *buf;
    int32_t ret;

    if (g_num_files >= MAX_NUM_MODULES)
    {
        ALERT("IOCTL_ADD_MODULE: too many modules have been loaded\n");
        return BF_IOCTL_ERROR_ADD_MODULE_FAILED;
    }

    /*
     * On Linux, we are not given a size for the IOCTL. Appearently
     * it is common practice to seperate this information into two
     * different IOCTLs, which is what we do here. This however means
     * that we have to store state, so userspace has to be careful
     * to send these IOCTLs in the correct order.
     *
     * Linux also does not copy userspace memory for use, so we need
     * to do this ourselves. As a result, we alloc memory for the
     * buffer that userspace is providing us so that we can copy this
     * memory from userspace as needed.
     */

    buf = platform_alloc(g_module_length);
    if (buf == NULL)
    {
        ALERT("IOCTL_ADD_MODULE: failed to allocate memory for the module\n");
        return BF_IOCTL_ERROR_ADD_MODULE_FAILED;
    }

    ret = copy_from_user(buf, file, g_module_length);
    if (ret != 0)
    {
        ALERT("IOCTL_ADD_MODULE: failed to copy memory from userspace\n");
        goto failed;
    }

    ret = common_add_module(buf, g_module_length);
    if (ret != BF_SUCCESS)
    {
        ALERT("IOCTL_ADD_MODULE: failed to add module\n");
        goto failed;
    }

    files[g_num_files] = buf;
    g_num_files++;

    DEBUG("IOCTL_ADD_MODULE: succeeded\n");
    return BF_IOCTL_SUCCESS;

failed:

    vfree(buf);

    DEBUG("IOCTL_ADD_MODULE: failed\n");
    return BF_IOCTL_ERROR_ADD_MODULE_FAILED;
}

int32_t
ioctl_add_module_length(int32_t len)
{
    g_module_length = len;

    DEBUG("IOCTL_ADD_MODULE_LENGTH: succeeded\n");
    return BF_IOCTL_SUCCESS;
}

static int
__fill_in_page_tables_for(struct mm_struct *mm)
{
    /* Unused, but we want this in the signature, as we'll need it if we ever
     * allocate something from outside of the vmalloc area. */
    (void)mm;

    /* Fill in page tables for every region of memory that's being passed to
     * the VMM. Note that this only works if everything allocated in platform.c
     * is allocated from the vmalloc region. (If anything were to be allocated
     * from outside of the vmalloc region, you'd need to use apply_to_page_range
     * on that region to achieve the same effect. */
    __vmalloc_sync_all();
    return BF_SUCCESS;
}


int32_t
ioctl_start_vmm(void)
{
    int ret;

    if (g_mmu_context != NULL)
    {
        ALERT("IOCTL_START_VMM: we already seem to have an MMU context for the VMM; has it been started?\n");
        return BF_ERROR_VMM_ALREADY_STARTED;
    }

    /* Borrow the MM context from the current process. This holds a reference
     * to the context's page tables, ensuring they stay available for hypervisor.
     * Ideally, this will be replaced once EPT support is in. */
    g_mmu_context = get_task_mm(current);
    if(!g_mmu_context)
    {
        ALERT("IOCTL_START_VMM: couldn't find a memory context in which to run!\n");
        return BF_ERROR_UNKNOWN;
    }

    /* In some cases, Linux dynamically fills in the page tables for a given process
     * as faults occur (e.g. in vmalloc_fault). In these cases, we have physical pages
     * for each of our virtual addresses, but the page tables haven't been populated, yet.
     *
     * Before we can pass a MM context to our hypervisor, we'll need to fully flesh out
     * the hypervisor-used pages-- so all of the mappings are fully valid*/
    ret = __fill_in_page_tables_for(g_mmu_context);
    if(ret != BF_SUCCESS)
    {
        ALERT("IOCTL_START_VMM: could not create page table for vmm: %d\n", ret);
        return ret;
    }

    ret = common_start_vmm();
    if (ret != BF_SUCCESS)
    {
        ALERT("IOCTL_START_VMM: failed to start vmm: %d\n", ret);
        return ret;
    }

    DEBUG("IOCTL_START_VMM: succeeded\n");
    return BF_IOCTL_SUCCESS;
}

int32_t
ioctl_stop_vmm(void)
{
    int i;
    int ret;

    ret = common_stop_vmm();
    if (ret != BF_SUCCESS)
        ALERT("IOCTL_STOP_VMM: failed to stop vmm: %d\n", ret);

    for (i = 0; i < g_num_files; i++)
        platform_free(files[i]);

    if(g_mmu_context)
      mmput(g_mmu_context);

    g_mmu_context = NULL;
    g_num_files = 0;

    DEBUG("IOCTL_STOP_VMM: succeeded\n");
    return BF_IOCTL_SUCCESS;
}

int32_t
ioctl_dump_vmm(void)
{
    int ret;

    ret = common_dump_vmm();
    if (ret != BF_SUCCESS)
        ALERT("IOCTL_DUMP_VMM: failed to dump vmm: %d\n", ret);

    DEBUG("IOCTL_DUMP_VMM: succeeded\n");
    return BF_IOCTL_SUCCESS;
}

static long
dev_unlocked_ioctl(struct file *file,
                   unsigned int cmd,
                   unsigned long arg)
{
    const struct cpumask *cpu0 = cpumask_of(0);
    set_cpu_affinity(current->pid, cpu0);

    switch (cmd)
    {
        case IOCTL_ADD_MODULE:
            return ioctl_add_module((char *)arg);

        case IOCTL_ADD_MODULE_LENGTH:
            return ioctl_add_module_length((int32_t)arg);

        case IOCTL_START_VMM:
            return ioctl_start_vmm();

        case IOCTL_STOP_VMM:
            return ioctl_stop_vmm();

        case IOCTL_DUMP_VMM:
            return ioctl_dump_vmm();

        default:
            return -EINVAL;
    }
}

static struct file_operations fops =
{
    .open = dev_open,
    .release = dev_release,
    .unlocked_ioctl = dev_unlocked_ioctl,
};

static struct miscdevice bareflank_dev =
{
    MISC_DYNAMIC_MINOR,
    "bareflank",
    &fops
};

/* ========================================================================== */
/* Entry                                                                      */
/* ========================================================================== */

int
dev_init(void)
{
    int ret;

    set_cpu_affinity = (set_affinity_fn)kallsyms_lookup_name("sched_setaffinity");
    if (set_cpu_affinity == NULL)
    {
        ALERT("Failed to locate sched_setaffinity, to avoid problems, not continuing to load bareflank\n");
        return -1;
    }

    __vmalloc_sync_all = (vmalloc_sync_all_fn)kallsyms_lookup_name("vmalloc_sync_all");
    if (__vmalloc_sync_all == NULL)
    {
        ALERT("Failed to locate vmalloc_sync_all, to avoid problems, not continuing to load bareflank\n");
        return -1;
    }

    if ((ret = misc_register(&bareflank_dev)) != 0)
    {
        ALERT("misc_register failed\n");
        return ret;
    }

    if ((ret = common_init()) != 0)
    {
        ALERT("common_init failed\n");
        return ret;
    }

    DEBUG("dev_init succeeded");
    return 0;
}

void
dev_exit(void)
{
    common_fini();
    misc_deregister(&bareflank_dev);

    DEBUG("dev_exit succeeded\n");
    return;
}

module_init(dev_init);
module_exit(dev_exit);

MODULE_LICENSE("GPL");
