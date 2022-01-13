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

static ulong data_addr = 0;
static int data_len = 0;
static int target_pid = 0;

module_param(data_addr, ulong, S_IRUGO);
module_param(data_len, int, S_IRUGO);
module_param(target_pid, int, S_IRUGO);

static int __init main_init(void) {
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

  if (data_addr == 0 || data_len == 0 || target_pid == 0) {
    printk("insmod main <data_addr=?> <data len=?> <target_pid=?>\n");
    return 0;
  }

  printk("data_addr:0x%lX, data_len:%d, target_pid:%d\n", data_addr, data_len,
         target_pid);

  for_each_process(task) {
    if (task->pid == target_pid) {
      printk("find task:%s\n", task->comm);
      retval = 1;
      break;
    }
  }

  if (retval == 0) {
    printk("not find task\n");
    return -1;
  }

  pgd = pgd_offset(task->mm, data_addr);
  if (pgd_none(*pgd)) {
    printk("not mapped in pgd\n");
    return -1;
  }

  pud = pud_offset((pgd_t *)pgd, data_addr);
  if (pud_none(*pud)) {
    printk("not mapped in pud\n");
    return -1;
  }

  pmd = pmd_offset(pud, data_addr);
  if (pmd_none(*pmd)) {
    printk("not mapped in pmd\n");
    return -1;
  }

  pte = pte_offset_kernel(pmd, data_addr);
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
