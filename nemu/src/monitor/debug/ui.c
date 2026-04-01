#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_si(char *args) {
    int step = 1;  
    if (args != NULL) {
        sscanf(args, "%d", &step);
    }
    cpu_exec(step);  
    return 0;
}

static int cmd_info(char *args){
	if(args==NULL){
		printf("Usage: info SUBCOMMAND\n");
		printf("r: Print CPU register status\n");
		printf("w:Print watchpoint information\n");
		return 0;
	}

	if(strcmp(args, "r") == 0){
		cpu_reg_dump();	
	}
	else if (strcmp(args, "w") == 0) {
    wp_display();
  }
	else{
		printf("Error: Unknown subcommand '%s'\n", args);
    printf("Available subcommands: r, w\n");
	}	
	return 0;
}

static int cmd_p(char *ards){
	if (args == NULL) {
    printf("Usage: p EXPR\n");
    return 0;
  }

	 bool success;
  // 调用表达式求值函数，解析输入的表达式
  uint32_t val = expr(args, &success);
  if (success) {
    printf("%u (0x%x)\n", val, val);
  } else {
    printf("Error: Invalid expression '%s'\n", args);
  }
  return 0;

}


static int cmd_x(char *args) {
  if (args == NULL) {
    printf("Usage: x N EXPR\n");
    return 0;
  }

  // 1. 拆分参数：第一个空格分隔「N」和「EXPR」
  char *space_pos = strchr(args, ' ');
  if (space_pos == NULL) {
    printf("Error: Invalid format, usage: x N EXPR\n");
    return 0;
  }
  *space_pos = '\0'; // 把空格改为字符串结束符，拆分两部分
  char *n_str = args;
  char *expr_str = space_pos + 1;

  // 2. 解析N（要打印的4字节单元个数）
  int n;
  if (sscanf(n_str, "%d", &n) != 1 || n <= 0) {
    printf("Error: Invalid number '%s', must be positive integer\n", n_str);
    return 0;
  }

  // 3. 求值EXPR，得到起始内存地址
  bool success;
  uint32_t addr = expr(expr_str, &success);
  if (!success) {
    printf("Error: Invalid expression '%s'\n", expr_str);
    return 0;
  }

  // 4. 循环读取并打印N个4字节（每行4个，带地址前缀）
  printf("Examining %d bytes at address 0x%08x:\n", n * 4, addr);
  for (int i = 0; i < n; i++) {
    // 读取虚拟地址 addr + i*4 处的4字节内容
    uint32_t val = vaddr_read(addr + i * 4, 4);
    
    if (i % 4 == 0) printf("0x%08x: ", addr + i * 4);
    printf("%08x ", val);
    if (i % 4 == 3) printf("\n");
  }
  // 最后一行不足4个时补换行
  if (n % 4 != 0) printf("\n");

  return 0;
}

static int cmd_w(char *args) {
  if (args == NULL) {
    printf("Usage: w EXPR\n");
    return 0;
  }

  // 调用监视点创建函数，添加新监视点
  WP *wp = wp_new(args);
  if (wp == NULL) {
    printf("Error: Failed to create watchpoint for '%s'\n", args);
    return 0;
  }

  // 打印监视点序号，方便后续删除
  printf("Watchpoint %d set: %s\n", wp->NO, args);
  return 0;
}
 

static int cmd_d(char *args) {
  if (args == NULL) {
    printf("Usage: d N\n");
    return 0;
  }

  // 解析监视点序号
  int no;
  if (sscanf(args, "%d", &no) != 1 || no <= 0) {
    printf("Error: Invalid watchpoint number '%s', must be positive integer\n", args);
    return 0;
  }

  // 调用删除函数
  bool success = wp_delete(no);
  if (success) {
    printf("Watchpoint %d deleted successfully\n", no);
  } else {
    printf("Error: Watchpoint %d does not exist\n", no);
  }
  return 0;
}


static int cmd_help(char *args);

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si",   "Single step execution, si N = execute N steps", cmd_si },
	{"info","print program status, info r for registers, info w for watchpoints", cmd_info},
	{"p","evaluate expression EXPR", cmd_p},
	{"x","examine N 4-byte memory units starting at EXPR", cmd_x},
	{"w","set watchpoint for expression EXPR", cmd_w},
	{"d","delete watchpoint number N", cmd_d},
  /* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  while (1) {
    char *str = rl_gets();
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
z
