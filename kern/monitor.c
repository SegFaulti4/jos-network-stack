/* Simple command-line kernel monitor useful for
* controlling the kernel and exploring the system interactively. */

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/env.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/tsc.h>
#include <kern/timer.h>
#include <kern/env.h>
#include <kern/trap.h>
#include <kern/pmap.h>
#include <kern/kclock.h>

#include <kern/e1000.h>
#include <kern/udp.h>
#include <kern/http.h>
#include <kern/traceopt.h>

#define WHITESPACE "\t\r\n "
#define MAXARGS    16

/* Functions implementing monitor commands */
int mon_help(int argc, char **argv, struct Trapframe *tf);
int mon_kerninfo(int argc, char **argv, struct Trapframe *tf);
int mon_backtrace(int argc, char **argv, struct Trapframe *tf);
int mon_dumpcmos(int argc, char **argv, struct Trapframe *tf);
int mon_start(int argc, char **argv, struct Trapframe *tf);
int mon_stop(int argc, char **argv, struct Trapframe *tf);
int mon_frequency(int argc, char **argv, struct Trapframe *tf);
int mon_memory(int argc, char **argv, struct Trapframe *tf);
int mon_pagetable(int argc, char **argv, struct Trapframe *tf);
int mon_virt(int argc, char **argv, struct Trapframe *tf);
int mon_e1000_recv(int argc, char **argv, struct Trapframe *tf);
int mon_e1000_tran(int argc, char **argv, struct Trapframe *tf);
int mon_eth_recv(int argc, char **argv, struct Trapframe *tf);
int mon_http_test(int argc, char **argv, struct Trapframe *tf);

struct Command {
   const char *name;
   const char *desc;
   /* return -1 to force monitor to exit */
   int (*func)(int argc, char **argv, struct Trapframe *tf);
};

static struct Command commands[] = {
       {"help", "Display this list of commands", mon_help},
       {"kerninfo", "Display information about the kernel", mon_kerninfo},
       {"backtrace", "Print stack backtrace", mon_backtrace},
       {"dumpcmos", "Print CMOS contents", mon_dumpcmos},
       {"timer_start", "Start timer", mon_start},
       {"timer_stop", "Stop timer", mon_stop},
       {"timer_freq", "Count processor frequency", mon_frequency},
       {"memory", "Memory", mon_memory},
       {"pagetable", "Dump page table", mon_pagetable},
       {"virt", "Dump virtual list", mon_virt},
       {"e1000_recv", "Test e1000 receive", mon_e1000_recv},
       {"eth_recv", "Test eth receive", mon_eth_recv},
       {"e1000_tran", "Test e1000 transmit", mon_e1000_tran},
       {"http_test", "Test http parsing", mon_http_test}
};
#define NCOMMANDS (sizeof(commands) / sizeof(commands[0]))

/* Implementations of basic kernel monitor commands */

int
mon_help(int argc, char **argv, struct Trapframe *tf) {
   for (size_t i = 0; i < NCOMMANDS; i++)
       cprintf("%s - %s\n", commands[i].name, commands[i].desc);
   return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf) {
   extern char _head64[], entry[], etext[], edata[], end[];

   cprintf("Special kernel symbols:\n");
   cprintf("  _head64 %16lx (virt)  %16lx (phys)\n", (unsigned long)_head64, (unsigned long)_head64);
   cprintf("  entry   %16lx (virt)  %16lx (phys)\n", (unsigned long)entry, (unsigned long)entry - KERN_BASE_ADDR);
   cprintf("  etext   %16lx (virt)  %16lx (phys)\n", (unsigned long)etext, (unsigned long)etext - KERN_BASE_ADDR);
   cprintf("  edata   %16lx (virt)  %16lx (phys)\n", (unsigned long)edata, (unsigned long)edata - KERN_BASE_ADDR);
   cprintf("  end     %16lx (virt)  %16lx (phys)\n", (unsigned long)end, (unsigned long)end - KERN_BASE_ADDR);
   cprintf("Kernel executable memory footprint: %luKB\n", (unsigned long)ROUNDUP(end - entry, 1024) / 1024);
   return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf) {
   // LAB 2: Your code here
   uint64_t *rbp = 0x0;
   uint64_t rip = 0x0;
   struct Ripdebuginfo info;

   rbp = (uint64_t *)read_rbp();
   if (rbp) rip = rbp[1];

   if (!rbp || !rip) {
       cprintf("JOS: Failed to backtrace stack");
       return -1;
   }

   cprintf("Stack backtrace:\n");
   while (rbp) {
       rip = rbp[1];
       cprintf("  rbp %016lx  rip %016lx\n", (unsigned long)rbp, (unsigned long)rip);

       debuginfo_rip(rip, &info);
       cprintf("         %s:%d: %s+%ld\n", info.rip_file, info.rip_line, info.rip_fn_name, rip - info.rip_fn_addr);

       rbp = (uint64_t *)rbp[0];
   }
   return 0;
}

int
mon_dumpcmos(int argc, char **argv, struct Trapframe *tf) {
   // Dump CMOS memory in the following format:
   // 00: 00 11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF
   // 10: 00 ..
   // Make sure you understand the values read.
   // Hint: Use cmos_read8()/cmos_write8() functions.
   // LAB 4: Your code here

   for (unsigned i = 0; i < 0x80; i += 0x10) {
       cprintf("%2.2X:", i);
       for (unsigned j = 0; j < 0x10; j++) {
           cprintf(" %2.2X", cmos_read8(i + j));
       }
       cprintf("\n");
   }
   return 0;
}

/* Implement timer_start (mon_start), timer_stop (mon_stop), timer_freq (mon_frequency) commands. */
// LAB 5: Your code here:

int
mon_start(int argc, char **argv, struct Trapframe *tf) {
   if (argc < 2) {
       return 1;
   }
   timer_start(argv[1]);
   return 0;
}

int
mon_stop(int argc, char **argv, struct Trapframe *tf) {
   timer_stop();
   return 0;
}

int
mon_frequency(int argc, char **argv, struct Trapframe *tf) {
   if (argc != 2) {
       return 1;
   }
   timer_cpu_frequency(argv[1]);
   return 0;
}

/* Implement memory (mon_memory) command.
* This command should call dump_memory_lists()
*/
// LAB 6: Your code here
int
mon_memory(int argc, char **argv, struct Trapframe *tf) {
   dump_memory_lists();
   return 0;
}

/* Implement mon_pagetable() and mon_virt()
 * (using dump_virtual_tree(), dump_page_table())*/
// LAB 7: Your code here

int
mon_pagetable(int argc, char **argv, struct Trapframe *tf) {
    dump_page_table(current_space->pml4);
    return 0;
}

int
mon_virt(int argc, char **argv, struct Trapframe *tf) {
    dump_virtual_tree(current_space->root, current_space->root->class);
    return 0;
}

int
mon_e1000_recv(int argc, char **argv, struct Trapframe *tf) {
    char buf[1000];
    int len = e1000_receive(buf);
    cprintf("received len: %d\n", len);
    cprintf("received packet: ");
    for (int i = 0; i < len; i++) {
        cprintf("%x ", buf[i] & 0xff);
    }
    cprintf("\n");
    return 0;
}

int
mon_eth_recv(int argc, char **argv, struct Trapframe *tf) {
    while (1) {
        char buf[1000];
        e1000_listen();
        int len = eth_recv(buf);
        if (trace_packets && len >= 0) {
            cprintf("received len: %d\n", len);
            if (len > 0) {
                cprintf("received packet: ");
                for (int i = 0; i < len; i++) {
                    cprintf("%x ", buf[i] & 0xff);
                }
                cprintf("\n");
            }
        } else {
            cprintf("received status: %s%s\n", (len >= 0) ? "OK" : "ERROR", (len == 0) ? " EMPTY" : " ");
        }
        cprintf("\n");
    }
    return 0;
}

int
mon_e1000_tran(int argc, char **argv, struct Trapframe *tf) {
    for (int i = 0; i < 70; i++) {
        char buf[] = "Hello\n";
        udp_send(buf, sizeof(buf));
        //e1000_transmit(buf, sizeof(buf));
    }
    return 0;
}

int
mon_http_test(int argc, char **argv, struct Trapframe *tf) {
    char reply[1024] = {};
    size_t reply_len = 0;
    char *buf1 = "Hello, HTTP!";
    cprintf("%s\n", http_parse(buf1, strlen(buf1), reply, &reply_len) ? "FAULT" : "SUCCESS");
    udp_send(reply, reply_len);
    char *buf2 = "POST /hello.world HTTP/1.1";
    cprintf("%s\n", http_parse(buf2, strlen(buf2), reply, &reply_len) ? "FAULT" : "SUCCESS");
    udp_send(reply, reply_len);
    char *buf3 = "GET /hello.world HTTP/2";
    cprintf("%s\n", http_parse(buf3, strlen(buf3), reply, &reply_len) ? "FAULT" : "SUCCESS");
    udp_send(reply, reply_len);
    char *buf4 = "GET /hello.world HTTP/1.1";
    cprintf("%s\n", http_parse(buf4, strlen(buf4), reply, &reply_len) ? "FAULT" : "SUCCESS");
    udp_send(reply, reply_len);
    return 0;
}
        /* Kernel monitor command interpreter */

static int
runcmd(char *buf, struct Trapframe *tf) {
   int argc = 0;
   char *argv[MAXARGS];

   argv[0] = NULL;

   /* Parse the command buffer into whitespace-separated arguments */
   for (;;) {
       /* gobble whitespace */
       while (*buf && strchr(WHITESPACE, *buf)) *buf++ = 0;
       if (!*buf) break;

       /* save and scan past next arg */
       if (argc == MAXARGS - 1) {
           cprintf("Too many arguments (max %d)\n", MAXARGS);
           return 0;
       }
       argv[argc++] = buf;
       while (*buf && !strchr(WHITESPACE, *buf)) buf++;
   }
   argv[argc] = NULL;

   /* Lookup and invoke the command */
   if (!argc) return 0;
   for (size_t i = 0; i < NCOMMANDS; i++) {
       if (strcmp(argv[0], commands[i].name) == 0)
           return commands[i].func(argc, argv, tf);
   }

   cprintf("Unknown command '%s'\n", argv[0]);
   return 0;
}

void
monitor(struct Trapframe *tf) {

   cprintf("Welcome to the JOS kernel monitor!\n");
   cprintf("Type 'help' for a list of commands.\n");

   if (tf) print_trapframe(tf);

   #ifdef NETWORK_ENABLE
	mon_eth_recv(0, NULL, NULL);
   #endif

   char *buf;
   do buf = readline("K> ");
   while (!buf || runcmd(buf, tf) >= 0);
}
