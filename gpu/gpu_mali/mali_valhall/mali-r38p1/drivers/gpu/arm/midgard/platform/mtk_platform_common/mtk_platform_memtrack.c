// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <linux/seq_file.h>
#if IS_ENABLED(CONFIG_PROC_FS)
#include <linux/proc_fs.h>
#endif
#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <platform/mtk_platform_common.h>
#include <platform/mtk_platform_common/mtk_platform_memtrack.h>
#include <linux/delay.h>

extern unsigned int (*mtk_get_gpu_memory_usage_fp)(void);

#if IS_ENABLED(CONFIG_PROC_FS)
/* name of the proc entry */
#define	PROC_GPU_MEMORY "gpu_memory"

static DEFINE_MUTEX(memtrack_lock);

static int mtk_gpu_memory_show(struct seq_file *m, void *v)
{
	struct kbase_device *kbdev = (struct kbase_device *)mtk_common_get_kbdev();
	struct kbase_context *kctx;
	unsigned int trylock_count = 0;

	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	lockdep_off();

	mutex_lock(&memtrack_lock);

	while (!mutex_trylock(&kbdev->kctx_list_lock)) {
		if (trylock_count > 3) {
			pr_info("[%s]lock held, bypass memory usage query", __func__);
			seq_printf(m, "<INVALID>");
			goto out_lock_held;
		}
		trylock_count ++;
		udelay(10);
	}

	/* output the total memory usage */
	seq_printf(m, "%-16s  %10u\n",
	           kbdev->devname,
	           atomic_read(&(kbdev->memdev.used_pages)));

	list_for_each_entry(kctx, &kbdev->kctx_list, kctx_list_link) {
		/* output the memory usage and cap for each kctx */
		seq_printf(m, "  %s-0x%p %10u %10u\n",
		           "kctx",
		           kctx,
		           atomic_read(&(kctx->used_pages)),
		           kctx->tgid);
	}

#if IS_ENABLED(CONFIG_MALI_MTK_MEM_ALLOCATE_POLICY)
	seq_printf(m, "[%u] r0 %10u, r1 %10u, waste: %10u/%u --- %d\n", kbdev->memdev.ratio,
		kbdev->memdev.nr_rank[0], kbdev->memdev.nr_rank[1],
		kbdev->memdev.nr_waste_list, kbdev->memdev.waste_target, kbdev->memdev.bDisWA);
#endif

	mutex_unlock(&kbdev->kctx_list_lock);

out_lock_held:
	mutex_unlock(&memtrack_lock);

	lockdep_on();

	return 0;
}
DEFINE_PROC_SHOW_ATTRIBUTE(mtk_gpu_memory);

int mtk_memtrack_procfs_init(struct kbase_device *kbdev, struct proc_dir_entry *parent)
{
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	proc_create(PROC_GPU_MEMORY, 0444, parent, &mtk_gpu_memory_proc_ops);

	return 0;
}

int mtk_memtrack_procfs_term(struct kbase_device *kbdev, struct proc_dir_entry *parent)
{
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	remove_proc_entry(PROC_GPU_MEMORY, parent);

	return 0;
}
#else
int mtk_memtrack_procfs_init(struct kbase_device *kbdev, struct proc_dir_entry *parent)
{
	return 0;
}

int mtk_memtrack_procfs_term(struct kbase_device *kbdev, struct proc_dir_entry *parent)
{
	return 0;
}
#endif /* CONFIG_PROC_FS */

/* Return the total memory usage */
static unsigned int mtk_memtrack_gpu_memory_total(void)
{
	struct kbase_device *kbdev = (struct kbase_device *)mtk_common_get_kbdev();
	unsigned int used_pages;

	if (IS_ERR_OR_NULL(kbdev))
		return 0;

	used_pages = atomic_read(&(kbdev->memdev.used_pages));

	return used_pages * 4096;
}

int mtk_memtrack_init(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	mtk_get_gpu_memory_usage_fp = mtk_memtrack_gpu_memory_total;

	return 0;
}

int mtk_memtrack_term(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	mtk_get_gpu_memory_usage_fp = NULL;

	return 0;
}
