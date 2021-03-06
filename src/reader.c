#include "reader.h"
#include <ctype.h>

int read_line_nr;
int read_line_pos;
int read_pos = 0;
#define CURRENT s[read_pos]

bool is_ok_in_symbol(char c, bool initial) {
  if(isdigit(c) && initial) {
    return false;
  }
  else if(c == '\'' && initial) {
    return true;
  }
  else if(c == '!' || c == '?' || c == '<' || c == '>' || c == '=' || c == '%' || 
	  c == '+' || c == '*' || c == '/' || c == '-' || c == '_') {
    return true;
  }
  else if(isalpha(c) || isdigit(c)) {
    return true;
  }
  else {
    return false;
  }
}

bool is_whitespace(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == ',';
}

void hit_new_line() {
  read_line_nr++;
  read_line_pos = 0;
}

void skip_whitespace(char *s) {
  while(is_whitespace(CURRENT)) {
    if(CURRENT == '\n') {
      hit_new_line();
    }
    read_pos++;
  }
  if(CURRENT == ';') {
    while(CURRENT != '\n' && CURRENT != '\0') {
      read_pos++;
    }
    read_pos++;
    skip_whitespace(s);
  }
}

void print_read_pos() {
  printf("Line: %d, pos: %d.\n", read_line_nr, read_line_pos);
}

Obj *read_internal(Obj *env, char *s) {
  skip_whitespace(s);

  if(CURRENT == ')' || CURRENT == ']') {
    read_pos++;
    printf("Too many parenthesis at the end.\n");
    print_read_pos();
    return nil;
  }
  else if(CURRENT == '(' || CURRENT == '[') {
    Obj *list = obj_new_cons(NULL, NULL);
    Obj *prev = list;
    read_pos++;
    while(1) {
      skip_whitespace(s);
      if(CURRENT == '\0') {
	printf("Missing parenthesis at the end.\n");
	print_read_pos();
	return nil;
      }
      if(CURRENT == ')' || CURRENT == ']') {
	read_pos++;
	break;
      }
      Obj *o = read_internal(env, s);
      Obj *new = obj_new_cons(NULL, NULL);
      prev->car = o;
      prev->cdr = new;
      prev = new;
    }
    return list;
  }
  else if(CURRENT == '{') {
    Obj *list = obj_new_cons(NULL, NULL);
    Obj *prev = list;
    read_pos++;
    while(1) {
      skip_whitespace(s);
      if(CURRENT == '\0') {
	printf("Missing } at the end.\n");
	print_read_pos();
	return nil;
      }
      if(CURRENT == '}') {
	read_pos++;
	break;
      }
      Obj *key = read_internal(env, s);

      if(CURRENT == '}') {
	printf("Uneven number of forms in dictionary.\n");
	print_read_pos();
	return nil;
      }
      
      Obj *value = read_internal(env, s);
      
      Obj *new = obj_new_cons(NULL, NULL);
      Obj *pair = obj_new_cons(key, value);
      prev->car = pair;
      prev->cdr = new;
      prev = new;
    }
    Obj *dict = obj_new_environment(NULL);
    dict->bindings = list;
    return dict;
  }
  else if(CURRENT == '&') {
    read_pos++;
    return ampersand;
  }
  else if(isdigit(CURRENT) || (CURRENT == '-' && isdigit(s[read_pos + 1]))) {
    int negator = 1;
    if(CURRENT == '-') {
      negator = -1;
      read_pos++;
    }
    bool is_floating = false;
    char scratch[32];
    int i = 0;
    while(isdigit(CURRENT)) {
      scratch[i++] = CURRENT;
      read_pos++;
      if(CURRENT == '.' && !is_floating) {
	scratch[i++] = CURRENT;
	is_floating = true;
	read_pos++;
      }
      if(CURRENT == 'f') {
	is_floating = true;
	read_pos++;
	break;
      }
    }
    scratch[i] = '\0';
    if(is_floating) {
      float x = atof(scratch) * negator;
      return obj_new_float(x);
    } else {
      int num = atoi(scratch) * negator;
      return obj_new_int(num);
    }
  }
  else if(CURRENT == '\'') {
    read_pos++;
    Obj *sym = read_internal(env, s);
    Obj *cons2 = obj_new_cons(sym, nil);
    Obj *cons1 = obj_new_cons(lisp_quote, cons2);
    return cons1;
  }
  else if(is_ok_in_symbol(CURRENT, true)) {
    char name[512];
    int i = 0;
    while(is_ok_in_symbol(CURRENT, false)) {
      name[i++] = CURRENT;
      read_pos++;
    }
    name[i] = '\0';
    return obj_new_symbol(name);
  }
  else if(CURRENT == ':') {
    read_pos++;
    char name[512];
    int i = 0;
    while(is_ok_in_symbol(CURRENT, true)) {
      name[i++] = CURRENT;
      read_pos++;
    }
    name[i] = '\0';
    return obj_new_keyword(name);
  }
  else if(CURRENT == '"') {
    read_pos++;
    char str[512];
    int i = 0;
    while(CURRENT != '"') {
      if(CURRENT == '\0') {
	printf("Missing quote in string\n");
	break;
      }
      else if(CURRENT == '\\') {
	read_pos++;
	if(CURRENT == 'n') {
	  str[i++] = '\n';
	}
	else if(CURRENT == '"') {
	  str[i++] = '"';
	}
	else if(CURRENT == '\\') {
	  str[i++] = '\\';
	}
	else {
	  printf("Can't read '%c' after backslash (%d)\n", CURRENT, CURRENT);
	  read_pos++;
	  return nil;
	}
	read_pos++;
      }
      else {
	str[i++] = CURRENT;
	read_pos++;
      }
    }
    str[i] = '\0';
    read_pos++;
    return obj_new_string(str);
  }
  else if(CURRENT == 0) {
    return nil;
  }
  else {
    printf("Can't read '%c' (%d)\n", CURRENT, CURRENT);
    read_pos++;
    return nil;
  }
}

Obj *read_string(Obj *env, char *s) {
  read_line_nr = 1;
  read_line_pos = 0;
  read_pos = 0;
  Obj *top_forms = NULL;
  Obj *prev = NULL;
  while(s[read_pos] != '\0') {
    Obj *o = read_internal(env, s);
    Obj *cons = obj_new_cons(NULL, NULL);
    cons->car = o;
    if(!top_forms) {
      top_forms = cons;
    }
    if(prev) {
      prev->cdr = cons;
    }
    prev = cons;
    skip_whitespace(s);
  }
  return top_forms;
}
