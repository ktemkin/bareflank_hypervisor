/*
 * Bareflank Hypervisor
 *
 * Copyright (C) 2015 Assured Information Security, Inc.
 * Author: Rian Quinn        <quinnr@ainfosec.com>
 * Author: Brendan Kerrigan  <kerriganb@ainfosec.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <platform.h>

#include <debug.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/kallsyms.h>
#include <linux/mm.h>

typedef void (*vmalloc_sync_all_fn)(void);

void *
platform_alloc(int64_t len)
{
    void *addr = NULL;

    if (len == 0)
    {
        ALERT("platform_alloc: invalid length\n");
        return addr;
    }

    addr = vmalloc(len);

    if (addr == NULL)
        ALERT("platform_alloc: failed to vmalloc mem: %lld\n", len);

    return addr;
}

void *
platform_alloc_exec(int64_t len)
{
    void *addr = NULL;

    if (len == 0)
    {
        ALERT("platform_alloc_exec: invalid length\n");
        return addr;
    }

    addr = __vmalloc(len, GFP_KERNEL, PAGE_KERNEL_EXEC);

    if (addr == NULL)
        ALERT("platform_alloc_exec: failed to vmalloc executable mem: %lld\n", len);

    return addr;
}

struct page_t
platform_alloc_page(void)
{
    struct page_t pg = {0};

    pg.virt = vmalloc(PAGE_SIZE);
    pg.phys = (void *)page_to_phys(vmalloc_to_page(pg.virt));
    pg.size = PAGE_SIZE;

    if (pg.virt == NULL || pg.phys == NULL)
        ALERT("platform_alloc_page: failed to vmalloc page\n");

    return pg;
}

void
platform_free(void *addr)
{
    if (addr == NULL)
    {
        ALERT("platform_free: invalid address %p\n", addr);
        return;
    }

    vfree(addr);
}

void
platform_free_exec(void *addr, int64_t len)
{
    if (addr == NULL)
    {
        ALERT("platform_free_exec: invalid address %p\n", addr);
        return;
    }

    vfree(addr);
}

void
platform_free_page(struct page_t pg)
{
    if (pg.virt == 0)
    {
        ALERT("platform_free_page: invalid address %p\n", pg.virt);
        return;
    }

    vfree(pg.virt);
}

void *
platform_get_page_directory(void)
{
    struct mm_struct *idle_context = (struct mm_struct *)kallsyms_lookup_name("init_mm");
    if (idle_context == NULL)
    {
        ALERT("platform_get_page_directory: could not find the system idle context; something's really wrong\n");
        return NULL;
    }

    /* Before we use any page tables, we should make sure that all page tables
     * have their vmalloc region filled in. */
    vmalloc_sync_all_fn sync_page_directories =
      (vmalloc_sync_all_fn)kallsyms_lookup_name("vmalloc_sync_all");
    if (sync_page_directories == NULL)
    {
        ALERT("platform_get_page_directory: could not locate function to sync all page tables!\n");
        return NULL;
    }


    /* Return the page directory for the idle process, which should have all of
     * the kernel mapped in without lots of stray mappings, and which will be
     * available for the lifetime of the VMM. */
    sync_page_directories();
    return virt_to_phys(idle_context->pgd);
}
