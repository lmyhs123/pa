#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>

enum {
  TK_NOTYPE = 256, 
		 TK_EQ,
  TK_NUM,
  TK_HEX,
  TK_REG,
  TK_NEQ,
  TK_AND,
  TK_NEG,
  TK_DEREF,

  /* TODO: Add more token types */

};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
{"-", '-'},
  {"\\*", '*'},
  {"/", '/'},
  {"\\(", '('},
  {"\\)", ')'},
  {"!=", TK_NEQ},
  {"&&", TK_AND},
  {"\\$[a-zA-Z][a-zA-Z0-9]*", TK_REG},
  {"0[xX][0-9a-fA-F]+", TK_HEX},
  {"[0-9]+", TK_NUM},
  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"==", TK_EQ}         // equal
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
		  case TK_NOTYPE:
			 break;

		  case TK_NUM:
          case TK_HEX:
		  case TK_REG:
		  case '-':
          case '+':
		  case '*':
		  case '/':
		  case '(':
		  case ')':
          case TK_EQ:
		  case TK_NEQ:
		  case TK_AND:
             tokens[nr_token].type = rules[i].token_type;

             if (substr_len >= sizeof(tokens[nr_token].str)) {
               printf("token too long\n");
               return false;
             }

             strncpy(tokens[nr_token].str, substr_start, substr_len);
             tokens[nr_token].str[substr_len] = '\0';

             nr_token++;
             break;
          default: 
		    assert(0);
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }
 for (i = 0; i < nr_token; i++) {
  if (tokens[i].type == '-') {
    if (i == 0 ||
        !(tokens[i - 1].type == TK_NUM ||
          tokens[i - 1].type == TK_HEX ||
          tokens[i - 1].type == TK_REG ||
          tokens[i - 1].type == ')')) {
      tokens[i].type = TK_NEG;
    }
  }
  else if (tokens[i].type == '*') {
    if (i == 0 ||
        !(tokens[i - 1].type == TK_NUM ||
          tokens[i - 1].type == TK_HEX ||
          tokens[i - 1].type == TK_REG ||
          tokens[i - 1].type == ')')) {
      tokens[i].type = TK_DEREF;
    }
  }
}
  return true;
}


static bool check_parentheses(int p, int q) {
  if (tokens[p].type != '(' || tokens[q].type != ')') {
    return false;
  }

  int balance = 0;
  for (int i = p; i <= q; i++) {
    if (tokens[i].type == '(') balance++;
    else if (tokens[i].type == ')') balance--;

    if (balance == 0 && i < q) {
      return false;
    }
    if (balance < 0) {
      return false;
    }
  }

  return balance == 0;
}
static int precedence(int type) {
 switch (type) {
    case TK_AND: return 1;
    case TK_EQ:
    case TK_NEQ: return 2;
    case '+':
    case '-': return 3;
    case '*':
    case '/': return 4;
    case TK_NEG:
    case TK_DEREF: return 5;
    default: return 0;
  }
}
static int dominant_operator(int p, int q) {
  int op = -1;
  int min_pri = 100; // 设为一个足够大的初始值
  int balance = 0;

  for (int i = p; i <= q; i++) {
    // 1. 记录括号嵌套层数
    if (tokens[i].type == '(') {
      balance++;
      continue;
    }
    if (tokens[i].type == ')') {
      balance--;
      continue;
    }

    // 如果 balance 不为 0，说明当前符号在括号内部，绝对不可能是整个表达式的主运算符
    if (balance != 0) continue;

    int type = tokens[i].type;
    // 忽略数字和寄存器，它们不是运算符
    if (type == TK_NUM || type == TK_HEX || type == TK_REG) continue;

    int pri = precedence(type);
    if (pri > 0) {
      bool update = false;
      
      // 核心修改点：区分左结合与右结合
      if (pri < min_pri) {
        // 如果遇到了优先级【严格更低】的，无条件更新为主运算符
        update = true;
      } else if (pri == min_pri) {
        // 如果遇到了优先级【相同】的，需要看结合性
        // 在你的 precedence 函数中，单目运算符(TK_NEG, TK_DEREF)的优先级是 5
        // 单目运算符是右结合的（不能更新），而普通的加减乘除是左结合的（需要更新）
        if (pri != 5) { 
          update = true;
        }
      }

      if (update) {
        min_pri = pri;
        op = i;
      }
    }
  }

  return op;
}

static uint32_t eval(int p, int q, bool *success) {
  if (p > q) {
    *success = false;
    return 0;
  }

  if (p == q) {
    if (tokens[p].type == TK_NUM) {
      *success = true;
      return strtoul(tokens[p].str, NULL, 10);
    }
    if (tokens[p].type == TK_HEX) {
      *success = true;
      return strtoul(tokens[p].str, NULL, 16);
    }
    
		if (tokens[p].type == TK_REG) {
        char *reg_name = tokens[p].str + 1; // 指针 +1，跳过开头的 '$' 符号
        *success = true; // 默认假设匹配成功

        if (strcmp(reg_name, "eax") == 0) return cpu.eax;
        if (strcmp(reg_name, "ecx") == 0) return cpu.ecx;
        if (strcmp(reg_name, "edx") == 0) return cpu.edx;
        if (strcmp(reg_name, "ebx") == 0) return cpu.ebx;
        if (strcmp(reg_name, "esp") == 0) return cpu.esp;
        if (strcmp(reg_name, "ebp") == 0) return cpu.ebp;
        if (strcmp(reg_name, "esi") == 0) return cpu.esi;
        if (strcmp(reg_name, "edi") == 0) return cpu.edi;
        if (strcmp(reg_name, "eip") == 0 || strcmp(reg_name, "pc") == 0) return cpu.eip;

        // 如果上述都没匹配中，说明寄存器名字非法
        printf("Error: Unknown register '%s'\n", reg_name);
        *success = false; // 标记求值失败
        return 0;
    }

    *success = false;
    return 0;
  }

  if (check_parentheses(p, q)) {
    return eval(p + 1, q - 1, success);
  }

  int op = dominant_operator(p, q);
  if (op == -1) {
    *success = false;
    return 0;
  }

  if (tokens[op].type == TK_NEG) {
    uint32_t val = eval(op + 1, q, success);
    if (!*success) return 0;
    return -val;
  }

  if (tokens[op].type == TK_DEREF) {
    uint32_t addr = eval(op + 1, q, success);
    if (!*success) return 0;
    return vaddr_read(addr, 4);
  }

  uint32_t val1 = eval(p, op - 1, success);
  if (!*success) return 0;
  uint32_t val2 = eval(op + 1, q, success);
  if (!*success) return 0;

  switch (tokens[op].type) {
    case '+': return val1 + val2;
    case '-': return val1 - val2;
    case '*': return val1 * val2;
    case '/':
      if (val2 == 0) {
        *success = false;
        return 0;
      }
      return val1 / val2;
    case TK_EQ: return val1 == val2;
    case TK_NEQ: return val1 != val2;
    case TK_AND: return val1 && val2;
    default:
      *success = false;
      return 0;
  }
}


uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */


  if (nr_token == 0) {
    *success = false;
    return 0;
  }

return eval(0, nr_token - 1, success);
}
