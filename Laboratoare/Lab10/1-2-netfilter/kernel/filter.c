/*
 * SO2 - Networking Lab (#10)
 *
 * Exercise #1, #2: simple netfilter module
 *
 * Code skeleton.
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/atomic.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/net.h>
#include <linux/in.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/tcp.h>

#include "filter.h"

MODULE_DESCRIPTION("Simple netfilter module");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

#define LOG_LEVEL		KERN_ALERT
#define MY_DEVICE		"filter"

static struct cdev my_cdev;
static atomic_t ioctl_set;
static unsigned int ioctl_set_addr;


/* Test ioctl_set_addr if it has been set.
 */
static inline int test_daddr(unsigned int dst_addr)
{
	/* TODO 2: return non-zero if address has been set
	 * *and* matches dst_addr
	 */
	return ioctl_set_addr && ioctl_set_addr == dst_addr;
}

/* TODO 1: netfilter hook function */
static unsigned int
addr_hook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)
{
	struct tcphdr *tcp_header = tcp_hdr(skb);
	struct iphdr *ip_header = ip_hdr(skb);

	if (tcp_header->syn && !tcp_header->ack
			&& !test_daddr(ip_header->saddr))
		pr_info("addr = %pI4; src_port = %hu; dst_port = %hu\n",
			&ip_header->saddr, ntohs(tcp_header->source),
			ntohs(tcp_header->dest));

	return NF_ACCEPT;
}

static int my_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int my_close(struct inode *inode, struct file *file)
{
	return 0;
}

static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case MY_IOCTL_FILTER_ADDRESS:
		/* TODO 2: set filter address from arg */
		if (copy_from_user(&ioctl_set_addr, (void *)arg,
				sizeof(ioctl_set_addr))) {
			pr_err("copy_from_user failed\n");
			return -EFAULT;
		}
		break;
	default:
		return -ENOTTY;
	}

	return 0;
}

static const struct file_operations my_fops = {
	.owner = THIS_MODULE,
	.open = my_open,
	.release = my_close,
	.unlocked_ioctl = my_ioctl
};

/* TODO 1: define netfilter hook operations structure */
static const struct nf_hook_ops hook_ops = {
	.pf = PF_INET,
	.hook = addr_hook,
	.hooknum = NF_INET_LOCAL_OUT,
	.priority    = NF_IP_PRI_FIRST
};

int __init my_hook_init(void)
{
	int err;

	/* register filter device */
	err = register_chrdev_region(MKDEV(MY_MAJOR, 0), 1, MY_DEVICE);
	if (err != 0)
		return err;

	atomic_set(&ioctl_set, 0);
	ioctl_set_addr = 0;

	/* init & add device */
	cdev_init(&my_cdev, &my_fops);
	cdev_add(&my_cdev, MKDEV(MY_MAJOR, 0), 1);

	/* TODO 1: register netfilter hook */
	err = nf_register_net_hook(&init_net, &hook_ops);
	if (err)
		goto out;

	return 0;

out:
	/* cleanup */
	cdev_del(&my_cdev);
	unregister_chrdev_region(MKDEV(MY_MAJOR, 0), 1);

	return err;
}

void __exit my_hook_exit(void)
{
	/* TODO 1: unregister hook */
	nf_unregister_net_hook(&init_net, &hook_ops);

	/* cleanup device */
	cdev_del(&my_cdev);
	unregister_chrdev_region(MKDEV(MY_MAJOR, 0), 1);
}

module_init(my_hook_init);
module_exit(my_hook_exit);
