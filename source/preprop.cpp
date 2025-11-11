#include <intrin.h>
#include <stdio.h>
#include <memory.h>

typedef signed char        i8;
typedef short              i16;
typedef int                i32;
typedef long long          i64;

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef float              f32;
typedef double             f64;

#define array_count(array) (sizeof(array) / sizeof((array)[0]))

#define assert(expr) if (!(expr)) {*(i32*)0 = 0;}
#define _(x)

struct Meta_struct {
  char *name;
  Meta_struct *next;
};
static Meta_struct *first_meta_struct;

static
char*
read_entire_file(char *file_name) {
  char *result = 0;
  
  FILE *file;
  i32 read = fopen_s(&file, file_name, "r");
  
  if (read) {
    return result;
  }
  
  fseek(file, 0, SEEK_END);
  size_t file_size = ftell(file);
  fseek(file, 0, SEEK_SET);
  
  result = (char*)malloc(file_size + 1);
  fread(result, file_size, _(count)1, file);
  result[file_size] = 0;
  
  fclose(file);
  return result;
}

enum Token_type {
  Token_unknown,
  
  Token_identifier,
  Token_string,
  
  Token_open_parenthesis,
  Token_close_parenthesis,
  Token_colon,
  Token_semi_colon,
  Token_asterisk,
  Token_open_bracket,
  Token_close_bracket,
  Token_open_brace,
  Token_close_brace,
  
  Token_end_of_stream
};

struct Token {
  Token_type type;
  
  size_t text_length;
  char *text;
}; 

struct Tokenizer {
  char *at;
};

inline
bool
token_equals(Token token, char *str) {
  char *at = str;
  
  for (u32 index = 0; index < token.text_length; index++, at++) {
    if (*at == 0 || token.text[index] != *at)
      return false;
  }
  
  bool result = *at == 0;
  
  return result;
}

inline
bool
is_end_of_line(char c) {
  bool result = (c == '\n' ||c == '\r');
  
  return result;
}

inline
bool
is_whitespace(char c) {
  bool result = (c == ' '  ||
                 c == '\t' ||
                 c == '\v' ||
                 c == '\f' ||
                 is_end_of_line(c));
  
  return result;
}

static
void
eat_all_whitespaces(Tokenizer *tokenizer) {
  
  while (true) {
    if (is_whitespace(tokenizer->at[0])) {
      tokenizer->at++;
    }
    
    // //
    else if (tokenizer->at[0] == '/' && tokenizer->at[1] == '/') {
      tokenizer->at += 2;
      
      while (tokenizer->at[0] && !is_end_of_line(tokenizer->at[0])) 
        tokenizer->at++;
    }
    
    // /*
    else if (tokenizer->at[0] == '/' && tokenizer->at[1] == '*') {
      tokenizer->at += 2;
      
      while (tokenizer->at[0] && !(tokenizer->at[0] == '*' && tokenizer->at[1] == '/'))
        tokenizer->at++;
      
      if (tokenizer->at[0] == '*') {
        tokenizer->at += 2;
      }
      
    }
    else {
      break;
    }
    
  }
  
}

inline
bool
is_alpha(char ch) {
  bool result = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_';
  
  return result;
}

inline
bool
is_number(char ch) {
  bool result = (ch >= '0' && ch <= '9');
  
  return result;
}

static
Token
get_token(Tokenizer *tokenizer) {
  
  eat_all_whitespaces(tokenizer);
  
  Token token = {};
  token.text_length = 1;
  token.text = tokenizer->at;
  char ch = tokenizer->at[0];
  tokenizer->at++;
  
  if (ch == 'i' && tokenizer->at[0] == 'n') {
    u32 kl =  0;
  }
  
  switch (ch) {
    
    default: {
      
      if (is_alpha(ch)) {
        
        token.type = Token_identifier;
        
        while (is_alpha(tokenizer->at[0]) || 
               is_number(tokenizer->at[0]) || 
               tokenizer->at[0] == '_') {
          tokenizer->at++;
        }
        
        token.text_length = tokenizer->at - token.text;
      }
      else {
        token.type = Token_unknown;
      }
      
    } break;
    
    case '\0': token.type = Token_end_of_stream; break;
    case '(': token.type = Token_open_parenthesis; break;
    case ')': token.type = Token_close_parenthesis; break;
    case ':': token.type = Token_colon; break;
    case ';': token.type = Token_semi_colon; break;
    case '*': token.type = Token_asterisk; break;
    case '[': token.type = Token_open_bracket; break;
    case ']': token.type = Token_close_bracket; break;
    case '{': token.type = Token_open_brace; break;
    case '}': token.type = Token_close_brace; break;
    
    case '"': {
      
      token.type = Token_string;
      token.text = tokenizer->at;
      
      while (tokenizer->at[0] && tokenizer->at[0] != '"') {
        if (tokenizer->at[0] == '\\' && tokenizer->at[1]) {
          tokenizer->at++;
        }
        
        tokenizer->at++;
      }
      
      token.text_length = tokenizer->at - token.text;
      
      if (tokenizer->at[0] == '"')
        tokenizer->at++;
      
    } break;
    
  }
  
  return token;
}


static
bool
require_token(Tokenizer *tokenizer, Token_type type) {
  Token token = get_token(tokenizer);
  
  bool result = (token.type == type);
  
  return result;
}

static 
void
parse_introspection_param(Tokenizer *tokenizer) {
  for (;;) {
    Token token = get_token(tokenizer);
    if (token.type == Token_close_parenthesis || token.type == Token_end_of_stream)
      break;
  }
}

static
void
parse_member(Tokenizer *tokenizer, Token struct_type_token, Token member_type_token) {
  bool parsing = true;
  bool is_pointer = false;
  
  while (parsing) {
    Token token = get_token(tokenizer);
    
    switch (token.type) {
      
      case Token_asterisk: {
        is_pointer = true;
      } break;
      
      case Token_identifier: {
        printf(" {%s, Meta_type_%.*s, \"%.*s\", (u32)(size_t)&((%.*s*)0)->%.*s},\n",
               is_pointer ? "Meta_member_flag_is_pointer" : "0",
               (int)member_type_token.text_length, member_type_token.text,
               (int)token.text_length, token.text,
               (int)struct_type_token.text_length, struct_type_token.text,
               (int)token.text_length, token.text);                
      } break;
      
      case Token_semi_colon:
      case Token_end_of_stream:
      {
        parsing= false;
      } break;
      
    }
  }
}


static
void
parse_struct(Tokenizer *tokenizer) {
  Token token = get_token(tokenizer);
  
  if (require_token(tokenizer, Token_open_brace)) {
    
    printf("Member_definition members_of_%.*s[] = {\n", (int)token.text_length, token.text);
    
    for (;;) {
      Token member = get_token(tokenizer);
      
      if (member.type == Token_close_brace)
        break;
      else
        parse_member(tokenizer, token, member);
    }
    
    printf("};\n\n");
    
    Meta_struct *meta = (Meta_struct*)malloc(sizeof(Meta_struct));
    meta->name = (char*)malloc(token.text_length + 1);
    memcpy(meta->name, token.text, token.text_length);
    meta->name[token.text_length] = 0;
    meta->next = first_meta_struct;
    first_meta_struct = meta;
  }
  
}

static
void
parse_introspect(Tokenizer *tokenizer) {
  
  if (require_token(tokenizer, Token_open_parenthesis)) {
    
    parse_introspection_param(tokenizer);
    
    Token token = get_token(tokenizer);
    
    if (token_equals(token, "struct")) {
      parse_struct(tokenizer);
    }
    else {
      printf("not struct");
    }
    
  }
  else {
    printf("expected open parenthesis");
  }
}

i32
main(i32 arg_count, char **args) {
  
  char *file_names[] = {
    "../source/sim_region.h",
    "../source/math.h",
    "../source/world.h"
  };
  
  for (u32 file_index = 0; file_index < array_count(file_names); file_index++) {
    char *file_content = read_entire_file(file_names[file_index]);
    
    Tokenizer tokenizer = {};
    tokenizer.at = file_content;
    
    bool parsing = true;
    
    while (parsing) {
      Token token = get_token(&tokenizer);
      
      switch (token.type) {
        case Token_end_of_stream: parsing = false; break;
        case Token_unknown: break;
        
        case Token_identifier: {
          
          if (token_equals(token, "introspect")) {
            parse_introspect(&tokenizer);
          }
          
        } break;
        
        default: {
          
        } break;
      }
      
      //printf("%d: %.*s\n", token.type, (int)token.text_length, token.text);
    }
  }
  
  printf("#define Meta_type_dump(member_ptr, next_ident_level) \\\n");
  
  for (Meta_struct *meta = first_meta_struct; meta; meta = meta->next) {
    
    printf("case Meta_type_%s: { debug_text_line(member->name); debug_dump_struct(array_count(members_of_%s), members_of_%s, member_ptr, (next_ident_level)); } break; %s\n %s\n",
           meta->name, meta->name, meta->name, meta->next ? "\\" : "", meta->next ? "\\" : "");
    
  }
  
  return 0;
}