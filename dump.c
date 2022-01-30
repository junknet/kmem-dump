#include <asm/fcntl.h>
#include <asm/pgtable.h>
#include <asm/processor.h>
#include <asm/uaccess.h>
#include <linux/dcache.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <linux/string.h>

static ulong start = 0;
static int pid = 0;
static ulong end = 0;

module_param(start, ulong, S_IRUGO);
module_param(end, ulong, S_IRUGO);
module_param(pid, int, S_IRUGO);

static int __init main_init(void) {
  ulong data_len = end - start;
  struct task_struct *task;
  char *vaddr;
  int retval = 0;
  int ret;
  //   int i = 0;
  pgd_t *pgd = NULL;
  pud_t *pud = NULL;
  pmd_t *pmd = NULL;
  pte_t *pte = NULL;
  struct file *fp;
  mm_segment_t old_fs;
  loff_t pos;
  struct page *page;

  if (start == 0 || pid == 0) {
    printk("insmod main <start=?> <end=?> <pid=?>\n");
    return 0;
  }

  printk("start:0x%lX, data_len:%ld, pid:%d\n", start, end, pid);

  for_each_process(task) {
    if (task->pid == pid) {
      printk("find task:%s\n", task->comm);
      retval = 1;
      break;
    }
  }

  if (retval == 0) {
    printk("not find task\n");
    return -1;
  }

  pgd = pgd_offset(task->mm, start);
  if (pgd_none(*pgd)) {
    printk("not mapped in pgd\n");
    return -1;
  }

  pud = pud_offset((pgd_t *)pgd, start);
  if (pud_none(*pud)) {
    printk("not mapped in pud\n");
    return -1;
  }

  pmd = pmd_offset(pud, start);
  if (pmd_none(*pmd)) {
    printk("not mapped in pmd\n");
    return -1;
  }

  pte = pte_offset_kernel(pmd, start);
  if (pte_none(*pte)) {
    printk("not mapped in pte\n");
    return -1;
  }
  page = pte_page(*pte);
  vaddr = page_address(page);

  //  kernel dump
  fp = filp_open("/data/local/tmp/dump.kernel", O_RDWR | O_CREAT, 0644);
  if (IS_ERR(fp)) {
    ret = PTR_ERR(fp);
    printk(KERN_INFO "/data/local/tmp/dump.kernel = %d\n", ret);
    return ret;
  }
  old_fs = get_fs();
  set_fs(KERNEL_DS);
  pos = fp->f_pos;
  vfs_write(fp, vaddr, data_len, &pos);
  fp->f_pos = pos;
  set_fs(old_fs);
  filp_close(fp, NULL);
  return 0;
}

static void __exit main_exit(void) {}

module_init(main_init);
module_exit(main_exit);
MODULE_LICENSE("GPL");
