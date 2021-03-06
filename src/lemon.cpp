#define _CRT_SECURE_NO_WARNINGS
#include <set>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"
#include "lemon.h"

#define br std::string("\n")

struct syntax_error:std::logic_error
{
    syntax_error(const std::string &error = "syntax_error")
        :logic_error(error)
    {

    }
};

static inline std::vector<std::string>
split(const std::string &str, const std::string &delimiters)
{
    std::vector<std::string> tokens;
    std::string token;

    for (size_t i = 0l; i< str.size(); ++i)
    {
        char ch = str[i];
        if (delimiters.find(ch) == std::string::npos)
        {
            token.push_back(ch);
        }
        else
        {
            if (token.empty())
                continue;
            tokens.push_back(token);
            token.clear();
        }
    }
    if (!token.empty())
        tokens.push_back(token);
    return tokens;
}
static inline void skip(std::string &line,
                 const std::string &delimiters)
{
    size_t offset = 0;

    if (line.empty())
        return;

    for (size_t i = 0; i < line.size(); i++)
    {
        char ch = line[i];
        if (delimiters.find(ch) != delimiters.npos)
        {
            offset++;
            continue;
        }
        break;
    }

    if (offset == line.size())
    {
        line.clear();
    }
    else if (offset)
    {
        line = line.substr(offset);
    }
}

lemon::lemon()
{
    lexer_ = NULL;
    iterators_ = 0;
    init_filter();
}
lemon::~lemon()
{
    for (size_t i = 0; i < lexers_.size(); ++i)
    {
        lexer *l = lexers_[i];
        if(l->file_)
        {
            l->file_->close();
            delete l->file_;
        }
        delete l;
    }
}
void lemon::init_filter()
{
    filters_.insert("length");
    filters_.insert("default");
    filters_.insert("safe");
}
bool lemon::parse_cpp_header(const std::string &file_path)
{
    lexer_ = new_lexer(file_path);
    if(!lexer_)
        return false;
    try
    {
        parse_cpp_header();
    }
    catch (std::exception &e)
    {
        std::cout << e.what() << std::endl;
        print_lexer_status();
        return false;
    }
    return true;
}

lemon::lexer *lemon::new_lexer(const std::string &file_path)
{
    lexer *l = new lexer;

    l->line_ = 0;
    l->file_ = new std::ifstream;
    l->file_->clear();
    l->file_->open(file_path.c_str());
    l->file_path_ = file_path;

    if (!l->file_->good())
    {
        std::cout << "open file error. "<<file_path << std::endl;
        delete l->file_;
        delete l;
        return NULL;
    }
    return l;
}
void lemon::print_lexer_status()
{
    std::cout << "file:" << lexer_->file_path_<< std::endl;
    std::cout << "line:" << lexer_->line_ << std::endl;
    std::cout<< lexer_->current_line_ << std::endl;
    std::cout <<">>>> "<<lexer_->line_buffer_ << std::endl;
}
bool lemon::parse_template(const std::string &file_path)
{
    lexer_ = new_lexer(file_path);
    if(!lexer_)
    {
        return false;
    }
    try
    {
        lexers_.push_back(lexer_);
        parse_template();
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << std::endl;
        print_lexer_status();
        return false;
    }
    return true;
}
void lemon::read_line()
{
    if (std::getline(*(lexer_->file_), lexer_->line_buffer_).good())
    {
        lexer_->line_buffer_ += "\r\n";
        lexer_->current_line_ = lexer_->line_buffer_;
        lexer_->line_++;
        return;
    }
    lexer_->line_buffer_.clear();
}
void lemon::move_buffer(int offset)
{
    lexer_->line_buffer_ = lexer_->line_buffer_.substr(offset);
}
std::string lemon::get_string(const std::string &delimiters)
{
    std::string buffer;
    if (delimiters.empty())
        return lexer_->line_buffer_;

    for (size_t i = 0; i < lexer_->line_buffer_.size(); i++)
    {
        char ch = lexer_->line_buffer_[i];
        if (delimiters[0] != ch)
        {
            buffer.push_back(ch);
        }
        else 
        {
            int k = 0;
            bool ok = true;
            for (size_t j = i; j < lexer_->line_buffer_.size() &&
                 k < delimiters.size(); j++,k++)
            {
                if (lexer_->line_buffer_[j] != delimiters[k])
                {
                    ok = false;
                    break;
                }
            }
            if (ok)
                return buffer;
        }
    }
    return buffer;
}
std::string lemon::get_string(size_t len)
{
    if (len > lexer_->line_buffer_.size())
        len = lexer_->line_buffer_.size();

    return std::string (lexer_->line_buffer_.c_str(),len);
}
std::string lemon::next_token(const std::string &delimiters, const std::string &skips)
{
    std::string buffer;

    do
    {
        if (lexer_->line_buffer_.empty())
            read_line();
        if (lexer_->line_buffer_.empty())
            return buffer;
        if(skips.size())
            skip(lexer_->line_buffer_,skips);
        if (!lexer_->line_buffer_.empty())
            break;
    } while (true);
    

    for (size_t i = 0; i < lexer_->line_buffer_.size(); i++)
    {
        char ch = lexer_->line_buffer_[i];
        if (delimiters.find(ch) == std::string::npos)
        {
            buffer.push_back(ch);
        }
        else
        {
            if(buffer.empty())
                buffer.push_back(ch);
            break;
        }
    }
    lexer_->line_buffer_ = lexer_->line_buffer_.substr(buffer.size());
    return buffer;
}
void lemon::push_back(const lemon::token_t &value)
{
    tokens_.push_back(value);
}
int lemon::line()
{
    return lexer_->line_;
}
void lemon::assert_not_eof(const token_t &t)
{
    if(t.type_ == token_t::e_eof)
        throw syntax_error("end of file error");
}
#define eof_assert(T)  assert_not_eof(T)
lemon::token_t lemon::curr_token()
{
    return lexer_->token_;
}
void lemon::clear_line_buffer()
{
    lexer_->line_buffer_.clear();
}

lemon::token_t lemon::get_next_token(const std::string &skip_str)
{

    static std::string delimiters = " <>{}()[]%&!?:;|,\\/.\r\t\n\"'`=-";

    token_t t;
    std::string str;
    if (tokens_.size())
    {
        t = tokens_.front();
        tokens_.pop_front();
        return t;
    }
    else
    {
        if (skip_str.size())
        {
            skip(lexer_->line_buffer_, skip_str);
        }
        str = next_token(delimiters,skip_str);
        t.str_ = str;
    }

    if (str == " ")
    {
        t.type_ = token_t::e_space;
    }
    else if (str == "\t")
    {
        t.type_ = token_t::e_$t;
    }
    else if (str == "\r")
    {
        t.type_ = token_t::e_$r;
    }
    else if (str == "\n")
    {
        t.type_ = token_t::e_$n;
    }
   
    else if (str == "")
    {
        t.type_ = token_t::e_eof;
    }
    else if (str == "<")
    {
        t.type_ = token_t::e_less;
        if (get_string(1) == "=")
        {
            move_buffer(1);
            t.type_ = token_t::e_le;
            t.str_ = "<=";
        }
        else if (get_string(3) == "!--")
        {
            move_buffer(3);
            t.str_ = "<!--";
            t.type_ = token_t::e_html_comment_begin;
        }
    }
    else if (str == "-")
    {
        t.type_ = token_t::e_sub;
        if (get_string(2) == "->")
        {
            move_buffer(2);
            t.str_ = "-->";
            t.type_ = token_t::e_html_comment_end;
        }
    }
    else if (str == ">")
    {
        t.type_ = token_t::e_gt;
        if (get_string(1) == "=")
        {
            move_buffer(1);
            t.type_ = token_t::e_ge;
            t.str_ = ">=";
        }
    }
    else if (str == "{")
    {
        t.type_ = token_t::e_open_brace;
        if (get_string(1) == "{")
        {
            t.type_ = token_t::e_open_variable;
            t.str_ = "{{";
            move_buffer(1);
        }
        else if (get_string(1) == "%")
        {
            t.type_ = token_t::e_open_block;
            t.str_  = "{%";
            move_buffer(1);
        }
    }
    else if (str == "}")
    {
        t.type_ = token_t::e_close_brace;
        if (get_string(1) == "}")
        {
            t.type_ = token_t::e_close_variable;
            t.str_ = "}}";
            move_buffer(1);
        }
    }
    else if (str == "|")
    {
        t.type_ = token_t::e_pipeline;
    }
    else if (str == "/")
    {
        t.type_ = token_t::e_forward_slash;
        if(get_string(1) == "/")
        {
            move_buffer(1);
            t.type_ = token_t::e_cpp_comment;
        }
        else if(get_string(1) == "*")
        {
            move_buffer(1);
            t.type_ = token_t::e_cpp_comment_begin;
        }
    }
    else if(str == "*")
    {
        t.type_ = token_t::e_asterisk;
        if (get_string(1) == "/")
        {
            move_buffer(1);
            t.type_ = token_t::e_cpp_comment_end;
        }
    }
    else if(str == "class")
    {
        t.type_ = token_t::e_class;
    }
    else if(str == "struct")
    {
        t.type_ = token_t::e_struct;

    }
    else if(str == "public")
    {
        t.type_ = token_t::e_public;
    }
    else if(str == "private")
    {
        t.type_ = token_t::e_private;
    }
    else if(str == "protected")
    {
        t.type_ = token_t::e_protected;
    }
    else if(str == "inline")
    {
        t.type_ = token_t::e_inline;
    }
    else if(str == "virtual")
    {
        t.type_ = token_t::e_virtual;
    }
    else if (str == "namespace")
    {
        t.type_ = token_t::e_namespace;
    }
    else if(str == "bool")
    {
        t.type_ = token_t::e_bool;
    }
    else if (str == "%")
    {
        t.type_ = token_t::e_modulus;
        if (get_string(1) == "}")
        {
            move_buffer(1);
            t.type_ = token_t::e_close_block;
        }
    }
    else if (str == "&")
    {
        t.type_ = token_t::e_ampersand;
    }
    else if(str == "on")
    {
        t.type_ = token_t::e_on;
    }
    else if (str == "off")
    {
        t.type_ = token_t::e_off;
    }
    else if (str == "if")
    {
        t.type_ = token_t::e_if;
    }
    else if (str == "else")
    {
        t.type_ = token_t::e_else;
    }
    else if (str == "elif")
    {
        t.type_ = token_t::e_elif;
    }
    else if (str == "endif")
    {
        t.type_ = token_t::e_endif;
    }
    else if (str == "const")
    {
        t.type_ = token_t::e_const;
    }
    else if (str == "for")
    {
        t.type_ = token_t::e_for;
    }
    else if (str == "empty")
    {
        t.type_ = token_t::e_empty;
    }
    else if (str == "in")
    {
        t.type_ = token_t::e_in;
    }
    else if (str == "endfor")
    {
        t.type_ = token_t::e_endfor;
    }
    else if (str == "and")
    {
        t.type_ = token_t::e_and;
    }
    else if (str == "or")
    {
        t.type_ = token_t::e_or;
    }
    else if (str == "not")
    {
        t.type_ = token_t::e_not;
    }
    else if(str == "include")
    {
        t.type_ = token_t::e_include;
    }
    else if(str == "block")
    {
        t.type_ = token_t::e_block;
    }
    else if (str == "endblock")
    {
        t.type_ = token_t::e_end_block;
    }
    else if(str == "extends")
    {
        t.type_ = token_t::e_extends;
    }
    // filters
    else if (str == "length")
    {
        t.type_ = token_t::e_length;
    }
    else if(str == "default")
    {
        t.type_ = token_t::e_default;
    }
    else if(str == "safe")
    {
        t.type_ = token_t::e_safe;
    }// end filters
    else if(str == "autoescape")
    {
        t.type_ = token_t::e_autoescape;
    }
    else if(str == "endautoescape")
    {
        t.type_ = token_t::e_endautoescape;
    }
    //
    else if (str == ".")
    {
        t.type_ = token_t::e_dot;
    }
    else if (str == ":")
    {
        t.type_ = token_t::e_colon;
        if (get_string(1) == ":")
        {
            move_buffer(1);
            t.type_ = token_t::e_double_colon;
            t.str_ = "::";
        }
    }
    else if(str == "unsigned")
    {
        token_t t = get_next_token();
        eof_assert(t);
        if(t.type_ == token_t::e_int)
        {
            t.type_ = token_t::e_unsigned_int;
            t.str_ = "unsigned int";
        }
        else if(t.type_ == token_t::e_char)
        {
            t.type_ = token_t::e_unsigned_char;
            t.str_ = "unsigned short";
        }
        else if(t.type_ == token_t::e_short)
        {
            t.type_ = token_t::e_long;
            t.str_ = "unsigned long";
        }
        else if(t.type_ == token_t::e_long_long)
        {
            t.type_ = token_t::e_unsigned_long_long;
            t.str_ = "unsigned long long";
        }
        else
            push_back(t);
    }
    else if(str == "int")
    {
        t.type_ = token_t::e_int;
    }
    else if(str == "short")
    {
        t.type_ = token_t::e_short;
    }
    else if (str == "char")
    {
        t.type_ = token_t::e_char;
    }
    else if(str == "long")
    {
        token_t t = get_next_token();
        if(t.type_ == token_t::e_long)
        {
            t.type_ = token_t::e_long_long;
            t.str_ = "long long";
        }
        else
            push_back(t);
    }
    else if(str == "float")
    {
        t.type_ = token_t::e_float;
    }
    else if(str == "double")
    {
        t.type_ = token_t::e_double;
    }
    else if (str == "std")
    {

        token_t t2 = get_next_token();
        eof_assert(t2);
        if (t2.type_ == token_t::e_double_colon)
        {
            token_t t3 = get_next_token();
            eof_assert(t3);
            if (t3.str_ == "string")
            {
                t.type_ = token_t::e_std_string;
                t.str_ = "std::string";
            }
            else if (t3.str_ == "vector")
            {
                t.type_ = token_t::e_std_vector;
                t.str_ = "std::vector";
            }
            else if (t3.str_ == "list")
            {
                t.type_ = token_t::e_std_list;
                t.str_ = "std::list";
            }
            else if (t3.str_ == "map")
            {
                t.type_ = token_t::e_std_map;
                t.str_ = "std::map";
            }
            else if (t3.str_ == "set")
            {
                t.type_ = token_t::e_std_set;
                t.str_ = "std::set";
            }
            else
            {
                tokens_.push_back(t2);
                tokens_.push_back(t3);
            }
        }
        else
        {
            tokens_.push_back(t2);
        }
    }
    else if (str == "acl")
    {
        token_t t2 = get_next_token();
        eof_assert(t2);
        if (t2.type_ == token_t::e_double_colon)
        {
            token_t t3 = get_next_token();
            eof_assert(t3);
            if (t3.str_ == "string")
            {
                t.type_ = token_t::e_acl_string;
                t.str_ = "acl::string";
            }
        }
    }
    else if (str == ",")
    {
        t.type_ = token_t::e_comma;
    }
    else if (str == ";")
    {
        t.type_ = token_t::e_semicolon;
    }
    else if (str == ":")
    {
        t = get_next_token();
        eof_assert(t);
        t.type_ = token_t::e_colon;
        if (get_string(1) == ":")
        {
            move_buffer(1);
            t.type_ = token_t::e_double_colon;
        }
    }
    else if (str == "`")
    {
        t.type_ = token_t::e_backtick;
    }
    else if (str == "\"")
    {
        t.type_ = token_t::e_double_quote;
    }
    else if (str == "'")
    {
        t.type_ = token_t::e_quote;
    }
    else if (str == "(")
    {
        t.type_ = token_t::e_open_paren;
    }
    else if (str == ")")
    {
        t.type_ = token_t::e_close_paren;
    }
    else if (str == "=")
    {
        t.type_ = token_t::e_assign;
        if (get_string(1) == "=")
        {
            move_buffer(1);
            t.type_ = token_t::e_eq;
            t.str_ = "==";
        }
    }
    else if (str == "!")
    {
        t.type_ = token_t::e_not_op;
        if (get_string(1) == "=")
        {
            move_buffer(1);
            t.type_ = token_t::e_neq;
            t.str_ = "!=";
        }
    }
    else
    {
        t.type_ = token_t::e_identifier;
    }
    t.line_ = lexer_->line_;
    lexer_->token_ = t;
    return t;
}

static inline std::string get_param_str(const std::string &str)
{
    int count = 0;
    for (size_t i = 0; i <str.size() ; ++i)
    {
        char ch = str[i];
        if (ch == ',')
        {
            if(count == 0)
                return str.substr(0, i);
        }
        else if(ch == ')')
        {
            return str.substr(0, i);
        }
        else if(ch == '<')
        {
            count ++;
        }
        else if(ch == '>')
        {
            count --;
        }
    }
    return str;
}

static inline std::string get_type_str(const std::string &str)
{
    std::string buffer(str);
    std::string const_str("const ");
    skip(buffer," \r\n\t");
    if (buffer.size() > const_str.size())
    {
        bool match = true;
        for (size_t i = 0; i < const_str.size(); i++)
        {
            if (buffer[i] != const_str[i])
            {
                match = false;
                break;
            }
        }
        if (match)
        {
            buffer = buffer.substr(const_str.size());
            skip(buffer, " \r\n\t");
        }
    }
    bool flag = false;
    for (int i = (int)buffer.size() -1; i >= 0; --i)
    {
        char ch = buffer[i];
        if(ch == ' '|| ch == '&')
        {
            flag = true;
            continue;
        }
        if(flag && ch != ' ' && ch != '&')
        {
            return buffer.substr(0, (size_t)(i+1));
        }
        if(ch == '>')
        {
            return buffer.substr(0, (size_t)(i+1));
        }
    }
    return buffer;
}
lemon::field lemon::parse_param()
{
    lemon::field f;

    f.str_ = get_param_str(lexer_->line_buffer_);
    f.type_str_ = get_type_str(f.str_);

    token_t t = get_next_token();
    eof_assert(t);
    if(t.type_ == token_t::e_const)
    {
        t = get_next_token();
        eof_assert(t);
    }
    if (t.type_ == token_t::e_std_string||
        t.type_ == token_t::e_bool||
        t.type_ == token_t::e_char||
        t.type_ == token_t::e_unsigned_char||
        t.type_ == token_t::e_short||
        t.type_ == token_t::e_unsigned_shot||
        t.type_ == token_t::e_int||
        t.type_ == token_t::e_unsigned_int||
        t.type_ == token_t::e_long||
        t.type_ == token_t::e_unsigned_long||
        t.type_ == token_t::e_long_long||
        t.type_ == token_t::e_unsigned_long_long||
        t.type_ == token_t::e_std_string||
        t.type_ == token_t::e_acl_string)
    {
        f.type_ = get_field_type(t);
    }
    else if(t.type_ == token_t::e_std_vector ||
            t.type_ == token_t::e_std_list)
    {
        f.type_ = field::e_std_vector;
        if(t.type_ == token_t::e_std_list)
            f.type_ = field::e_std_list;

        token_t t = get_next_token();
        eof_assert(t);
        if (t.type_ != token_t::e_less)
            throw syntax_error("not find <");
        int count = 1;
        do
        {
            t = get_next_token();
            eof_assert(t);
            if (t.type_ == token_t::e_less)
                count++;
            else if (t.type_ == token_t::e_gt)
            {
                count--;
                if (count == 0)
                    break;
            }
        } while (true);
    }
    else
    {
        push_back(t);
        namespaces_t namespaces = get_namespaces();
        t = get_next_token(true);

        if (!check_class_exist(t.str_, namespaces))
            throw syntax_error("not find class");

        f.type_ = field::e_class;
        f.type_str_ = t.str_;
        f.namespaces_ = namespaces;
    }
    //get name
    t = get_next_token();
    eof_assert(t);
    if (t.type_ == token_t::e_ampersand)
        t = get_next_token();
    eof_assert(t);
    f.name_ = t.str_;
    t = get_next_token();
    eof_assert(t);
    if (t.type_ != token_t::e_close_paren&&
        t.type_ != token_t::e_comma)
        throw syntax_error("no find `,` or `)` ");
    return f;
}

lemon::field lemon::parse_return()
{
    token_t t = get_next_token();
    eof_assert(t);
    if (t.type_ != token_t::e_std_string)
        throw syntax_error("return value not std::string .error");
    field f;
    f.type_ = field::e_std_string;
    return f;
}
void lemon::parse_interface()
{
    template_.interface_.return_ = parse_return();
    template_.interface_.name_ = get_next_token().str_;

    token_t t = get_next_token();
    eof_assert(t);
    if (t.type_ != token_t::e_open_paren)
        throw syntax_error("not find (");

    while(true)
    {
        template_.interface_.params_.push_back(parse_param());
        t = curr_token();
        if (t.type_ == token_t::e_close_paren)
        {
            t = get_next_token();
            eof_assert(t);
            if (t.type_ != token_t::e_html_comment_end)
                throw syntax_error("not find -->");
            return;
        }
    }
}

std::string lemon::get_iterator()
{
    char buffer[32];
    sprintf(buffer,"it%d",++iterators_);

    return std::string(buffer);
}
std::string lemon::get_type(const std::string &name)
{
    std::vector<std::string> tokens = split(name, ".");

    std::string token = tokens[0];
    std::string type;
    namespaces_t namespaces;

    std::vector<field>::reverse_iterator rit = stack_.rbegin();
    for (; rit != stack_.rend(); ++rit)
    {
        if (rit->name_ == token)
        {
            type = rit->type_str_;
            namespaces = rit->namespaces_;
            break;
        }
    }

    if (type.empty())
        throw syntax_error("not find variable "+name);
    
    if (tokens.size() == 1)
        return to_string(namespaces)+type;

    class_t *cls = get_class(type, namespaces);
    if(!cls)
    {
        throw syntax_error("not find class " + type);
    }
    //a.b.c
    for (size_t i = 1; i < tokens.size(); i++)
    {
        token = tokens[i];
        bool ok = false;
        for (int j = 0; j < cls->variables_.size(); ++j)
        {
            field &f = cls->variables_[j];
            if(f.name_ == token)
            {
                if(i+1 < tokens.size())
                {
                    if(f.type_ == field::e_class)
                    {
                        cls = get_class(f.type_str_, f.namespaces_);
                        if(!cls)
                            throw syntax_error("Not find "+f.type_str_);
                        ok = true;
                    }
                    else
                        throw syntax_error(f.name_ + " is not class");
                }
                else
                {
                    return to_string(f.namespaces_) + f.type_str_;
                }
            }
        }
        if(!ok)
            throw syntax_error("not find " + token);
    }
    return type;
}

inline std::string skip_all(const std::string &str,
                            const std::string &skip_str)
{
    std::string buffer;
    for (size_t i = 0; i < str.size(); ++i)
    {
        char ch = str[i];
        if (skip_str.find(ch) == std::string::npos)
        {
            buffer.push_back(ch);
        }
    }
    return buffer;
}

//std::map<std::string,std::map<int , int>>
static inline std::string get_first_type(const std::string &str)
{
    std::string buffer;

    for (size_t i = 0; i < str.size(); i++)
    {
        char ch = str[i];
        if(ch == '<')
        {
            if(skip_all(buffer," \r\t\n") != "std::map")
            {
                throw syntax_error("not find std::map ");
            }
            buffer.clear();
        }
        else if(ch == ',')
        {
            break;
        }
        else
        {
            if(ch == ' ' && buffer.empty())
            {
                continue;
            }
            else if(ch == ' ' && i+1 < str.size() && str[i+1] == ',')
            {
                continue;
            }
            buffer.push_back(ch);
        }
    }
    return buffer;
}
//std::map<std::string,std::map<int , int>>
static inline std::string get_second_type(const std::string &str)
{
    std::string buffer;
    int count = 0;
    int flag = false;

    for (size_t i = 0; i < str.size(); i++)
    {
        char ch = str[i];
        if(ch == '<')
        {
            count++;
            if(count == 1)
            {
                if(skip_all(buffer," \r\t\n") != "std::map")
                    throw syntax_error("not find std::map ");
                
                buffer.clear();
                continue;
            }
            buffer.push_back(ch);
        }
        else if(ch == ',')
        {
            if(!flag)
            {
                buffer.clear();
                flag = true;
            }
            else
                buffer.push_back(ch);
        }
        else if(ch == '>')
        {
            count --;
            if(count ==0)
                 break;

            buffer.push_back(ch);
        }
        else
        {
            if(ch == ' ' && buffer.empty())
                continue;

            buffer.push_back(ch);
        }
    }
    return buffer;
}
static inline std::string get_next_type(const std::string &str)
{
    std::string buffer;

    int count = 0;

    for (size_t i = 0; i < str.size(); i++)
    {
        char ch = str[i];
        if(ch == '<')
        {
            count++;
            if (count == 1)
            {
                buffer.clear();
            }else
            {
                buffer.push_back(ch);
            }
        }
        else if(ch == '>')
        {
            count--;
            if (count == 0)
                return buffer;
            else
                buffer.push_back(ch);
        }
        else
        {
            if(ch == ' ' && buffer.empty())
            {
                continue;
            }
            else if(ch == ' ' && i+1 < str.size() && str[i+1] == ',')
            {
                continue;
            }
            buffer.push_back(ch);
        }
    }
    return buffer;
}

static inline bool check_map(const std::string &str)
{
    std::string buffer;

    size_t pos = str.find_first_of('<');
    if(pos != std::string::npos)
    {
        buffer = str.substr(0,pos);
        return skip_all(buffer," \r\t\n") == "std::map";
    }
    return skip_all(str," \r\t\n") == "std::map";
}

static inline bool check_vector(const std::string &str)
{
    std::string buffer;

    size_t pos = str.find_first_of('<');
    if(pos != std::string::npos)
    {
        buffer = str.substr(0,pos);
        return skip_all(buffer," \r\t\n") == "std::vector";
    }
    return skip_all(str," \r\t\n") == "std::vector";
}
static inline bool check_list(const std::string &str)
{
    std::string buffer;

    size_t pos = str.find_first_of('<');
    if(pos != std::string::npos)
    {
        buffer = str.substr(0,pos);
        return skip_all(buffer," \r\t\n") == "std::list";
    }
    return skip_all(str," \r\t\n") == "std::list";
}

int g_tab;
std::string tab()
{
    std::string tab (g_tab ,'\t');
    return tab;
}
#define CODE_BEGIN  (tab() + "code += \"")
#define CODE_END ("\";" + br)
struct code_buffer
{
    code_buffer()
    {
    }
    code_buffer &operator +=(const std::string &str)
    {
        if (str == "\t")
            return *this;

        if(str == CODE_BEGIN)
        {
            code_.append(line_);
            line_= str;
        }
        else if(str == CODE_END)
        {
            if(line_ == CODE_BEGIN)
            {
                line_.clear();
                return *this;
            }
            line_.append(str);
            //std::cout << line_ << std::flush;
            code_.append(line_);
            line_.clear();
        }
        else if (str == " ")
        {
            if (line_ == CODE_BEGIN)
            {
                return *this;
            }
            line_.append(str);
        }
        else 
        {
            if (line_.empty())
                code_.append(str);
            else
                line_.append(str);
        }

        return *this;
        //std::cout << str << std::flush;
    }
    operator std::string ()
    {
        return code_+line_;
    }
    std::string line_;
    std::string code_;
};

lemon::field::type lemon::get_field_type(const token_t& token)
{
    switch (token.type_)
    {
        case token_t::e_void:
            return field::e_void;
        case token_t::e_char:
            return field::e_char;
        case token_t::e_unsigned_char:
            return field::e_unsigned_char;
        case token_t::e_short:
            return field::e_short;
        case token_t::e_unsigned_shot:
            return field::e_unsigned_shot;
        case token_t::e_int:
            return field::e_int;
        case token_t::e_unsigned_int:
            return field::e_unsigned_int;
        case token_t::e_long:
            return field::e_long;
        case token_t::e_unsigned_long:
            return field::e_unsigned_long;
        case token_t::e_long_long:
            return field::e_long_long;
        case token_t::e_unsigned_long_long:
            return field::e_unsigned_long_long;
        case token_t::e_std_set:
            return field::e_std_set;
        case token_t::e_std_map:
            return field::e_std_map;
        case token_t::e_std_list:
            return field::e_std_list;
        case token_t::e_std_vector:
            return field::e_std_vector;
        case token_t::e_std_string:
            return field::e_std_string;
        case token_t::e_acl_string:
            return field::e_acl_string;
        default:
            throw syntax_error("unknown type");
    }
    return field::e_void;
}
lemon::field::type lemon::get_field_type(const std::string &type)
{
    std::vector<std::string> tokens = split(type," \r\n\t<,>");
    if (tokens.empty())
        throw syntax_error("error type "+ type);

    if (tokens[0] == "std::string")
    {
        return field::e_std_string;
    }
    else if (tokens[0] == "std::vector")
    {
        return field::e_std_vector;
    }
    else if (tokens[0] == "std::list")
    {
        return field::e_std_list;
    }
    else if (tokens[0] == "short")
    {
        return field::e_short;
    }
    else if (tokens[0] == "long")
    {
        if (tokens.size() == 1)
        {
            return field::e_long;
        }
        else if (tokens.size() == 2)
        {
            if (tokens[1] == "long")
                return field::e_long_long;
        }
    }
    else if (tokens[0] == "unsigned")
    {
        if (tokens.size() == 2)
        {
            if (tokens[1] == "int")
            {
                return field::e_unsigned_int;
            }
            else if (tokens[1] == "short")
            {
                return field::e_unsigned_shot;
            }
            else if (tokens[1] == "long")
            {
                return field::e_unsigned_long;
            }
        }
        else if (tokens.size() == 3)
        {
            if (tokens[1] == "long")
            {
                if (tokens[2] == "long")
                {
                    return field::e_unsigned_long_long;
                }
            }
        }
    }
    else if (tokens[0] == "int")
    {
        return field::e_int;
    }
    else if (tokens[0] == "float")
    {
        return field::e_float;
    }
    else if (tokens[0] == "double")
    {
        return field::e_double;
    }
    for (size_t i = 0; i < classes_.size(); ++i)
    {
        if(to_string(classes_[i].namespaces_)+classes_[i].name_ == type)
            return field::e_class;
    }
    throw syntax_error("not support type: "+tokens[0]);
    return field::e_void;
}
std::string lemon::get_code()
{
    return "";
}
std::string lemon::gen_bool_code(const std::string &item)
{
    std::string item_type = get_type(item);
    field::type type = get_field_type(item_type);
    if (type == field::e_std_vector ||
        type == field::e_std_list ||
        type == field::e_std_map||
        type == field::e_std_string)
    {
        return "!"+item + ".empty()";
    }
    else if (type == field::e_int)
    {
        return item +" != 0";
    }
    throw syntax_error("not support type: "+item_type);
    return std::string();
}

bool lemon::check_filter(const std::string &name)
{
    return filters_.find(name) != filters_.end();
}

std::string lemon::parse_if()
{
    code_buffer code;

    std::string item;
    bool flag = false;
    bool pipeline = false;
    bool op = false;

    code += tab() + "if(";

    do
    {
        token_t t = get_next_token();
        eof_assert(t);
        if (t.type_ == token_t::e_close_block)
        {
            if (item.empty())
                throw syntax_error("not find variable ");
            if (!op && !pipeline)
                code += gen_bool_code(item);
            else
                code += item;
            if (flag)
                code += ")";
            break;
        }
        else if (t.type_ == token_t::e_and)
        {
            flag = true;
            code += "(";
            if (pipeline)
                code += item;
            else
                code += gen_bool_code(item);
            code += ")&&(";
            item.clear();
            pipeline = false;
            op = false;
        }
        else if (t.type_ == token_t::e_or)
        {
            flag = true;
            code += "(";
            if (pipeline)
                code += item;
            else
                code += gen_bool_code(item);
            code += ")||(";
            item.clear();
            pipeline = false;
            op = false;
        }
        else if (t.type_ == token_t::e_pipeline)
        {
            t = get_next_token();
            eof_assert(t);
            if (!check_filter(t.str_))
                throw syntax_error("unknown filter "+t.str_);
            pipeline = true;
            std::string filter = t.str_;

            item = "lm::$"+filter + "(" + item + ")";
        }
        else if (t.type_ == token_t::e_gt||
                 t.type_ == token_t::e_ge||
                 t.type_ == token_t::e_le||
                 t.type_ == token_t::e_less||
                 t.type_ == token_t::e_neq)
        {
            op = true;
            item += t.str_;
        }
        else
        {
            item += t.str_;
        }

    } while (true);

    code += ")" + br;
    code += tab() + "{" + br;
    g_tab++;
    code += parse_html();

    return code;
}
// {% for key, value in items%}
// {% for key in items%}
std::string lemon::get_for_items()
{
    std::string buffer;
    
    do{
        token_t t = get_next_token();
        eof_assert(t);
        if (t.type_ == token_t::e_close_block)
            break;
        buffer += t.str_;
    }while (true);
    return buffer;
}
std::vector<std::string> get_namespace(const std::string &str)
{
    std::vector<std::string> namespaces;
    std::string buffer;

    bool flag = false;
    for (size_t i = 0; i < str.size();)
    {
        char ch = str[i];
        if (ch == ':' && i +1 < str.size() && str[i+1] == ':')
        {
            i +=2;
            namespaces.push_back(buffer);
            buffer.clear();
            continue;
        }
        buffer.push_back(ch);
        i+=1;
    }
    return namespaces;
}
void lemon::push_stack(const std::string &name, const std::string &type)
{
    field f;
    f.name_ = name;
    f.type_ = get_field_type(type);
    f.namespaces_ = get_namespace(type);
    f.type_str_ = type;
    if(f.namespaces_.size())
    {
        f.type_str_ = type.substr(type.find_last_of(':')+1);
    }


    stack_.push_back(f);
}

void lemon::push_stack_size(int size)
{
    stack_size_.push_back(size);
}

void lemon::pop_stack()
{
    int size = stack_size_.back();
    stack_size_.pop_back();

    for (int i = 0; i < size; i++)
        stack_.pop_back();
}

void lemon::pop_for_item()
{
    for_items_.pop_back();
}

void lemon::add_for_item(const std::string &item)
{
    for_items_.push_back(item);
}
std::string lemon::get_for_item()
{
    if (for_items_.empty())
        throw syntax_error("for_items empty");
    return for_items_.back();
}
std::string lemon::to_string(const namespaces_t &namespaces)
{
    std::string str;
    for (size_t i = 0; i < namespaces.size(); ++i)
    {
        str.append(namespaces[i]);
        str.append("::");
    }
    return str;
}
std::string lemon::parse_for()
{
    code_buffer code;

    std::string it = get_iterator();
    token_t t1 = get_next_token();
    token_t t2 = get_next_token();
    eof_assert(t1);
    eof_assert(t2);

    //{% for k, v in items %}
    if (t2.type_ == token_t::e_comma)
    {
        token_t t3 = get_next_token();

        eof_assert(t3);
        if (get_next_token().type_ != token_t::e_in)
            throw syntax_error("not find in ");

        std::string items = get_for_items();
        std::string items_type = get_type(items);
        add_for_item(items);

        code += tab() + items_type + "::const_iterator " + it;
        code += " = " + items + ".begin();" + br;
        code += tab() + "for (; " + it + " != ";
        code += items + ".end(); ++" + it + ")" + br;
        code += tab() + "{" + br;
        g_tab++;


        std::string key = t1.str_;
        std::string value = t3.str_;

        std::string key_type = get_first_type(items_type);
        std::string value_type = get_second_type(items_type);

        code += tab() + "const " + key_type + " &" + key;
        code += " = " + it + "->first;" + br;
        code += tab() + "const " + value_type + " &" + value;
        code += " = " + it + "->second;" + br;

        push_stack(key, key_type);
        push_stack(value, value_type);
        push_stack_size(2);
    }
        //{% for v in items %}
    else
    {
        if (t2.type_ != token_t::e_in)
            throw syntax_error("not find in ");

        std::string items = get_for_items();
        std::string items_type = get_type(items);
        add_for_item(items);

        code += tab() + items_type + "::const_iterator " + it;
        code += " = " + items + ".begin();" + br;
        code += tab() + "for (; " + it + " != ";
        code += items + ".end(); ++" + it + ")" + br;
        code += tab() + "{" + br;
        g_tab++;

        if (check_list(items_type) || check_vector(items_type))
        {
            std::string item = t1.str_;
            std::string item_type = get_next_type(items_type);

            code += tab() + "const " + item_type + " &" + item;
            code += " = *" + it + ";" + br;

            push_stack(item, item_type);
            push_stack_size(1);
        }
        else if (check_map(items_type))
        {
            std::string value = t1.str_;
            std::string value_type = get_second_type(items_type);
            code += tab() + "const " + value_type + " &" + value;
            code +=" = " + it + "->second;" + br;

            push_stack(value, value_type);
            push_stack_size(1);
        }
    }

    code +=parse_html();

    return code;
}
std::string lemon::get_default_string()
{
    std::string buffer;
    token_t t = get_next_token();
    if(t.type_ != token_t::e_colon)
        throw syntax_error("not find : ");
    t = get_next_token();
    if(t.type_ != token_t::e_double_quote)
        throw syntax_error("not find \" ");
    do
    {
        t = get_next_token("");
        if(t.type_ != token_t::e_double_quote)
            buffer.append(t.str_);
        else
            break;
    }while (true);
    return buffer;
}
// a.b.c.d
std::string lemon::get_variable()
{
    std::string  code;

    do
    {
        code.append(get_next_token(true).str_);
        token_t t = get_next_token(true);
        eof_assert(t);
        if (t.type_ == token_t::e_dot)
        {
            code.append(t.str_);
            continue;
        }
        push_back(t);
        break;
    } while (true);

    return code;
}
std::string lemon::to_string(const std::string &name,const std::string &type)
{
    field::type_t t = get_field_type(type);
    if (t == field::e_std_string||
            t == field::e_acl_string)
        return name;
    return "$to_string("+name+")";
}
std::string lemon::parse_variable()
{
    std::string code = tab() + "code += ";
    bool safe = false;
    std::string item = get_variable();
    std::string type = get_type(item);
    if (type.empty())
        throw syntax_error("unknown " + item);
    std::string to_str = to_string(item, type);
    if(to_str != item)
        safe = true;
    item = to_str;
    token_t t = get_next_token();
    eof_assert(t);
    if (t.type_ == token_t::e_pipeline)
    {
        do
        {
            t = get_next_token();
            //{{ data|default:"string" }}
            if(t.type_ == token_t::e_default)
            {
                if(auto_escape())
                    item = "lm::$escape("+item+")";
                std::string str = get_default_string();
                item = "lm::$"+t.str_ + "(" + item + ", \""+str+"\")";
                safe = true;
            }
            else if(t.type_ == token_t::e_safe)
                safe = true;
            else
            {
                if(!check_filter(t.str_))
                    throw syntax_error("not found filter :" + t.str_);
                item = "lm::$"+t.str_ + "(" + item + ")";
            }
            t = get_next_token();
            eof_assert(t);
            if (t.type_ == token_t::e_pipeline)
                continue;
            else if (t.type_ == token_t::e_close_variable)
                break;

        } while (true);
    }
    else if (t.type_ != token_t::e_close_variable)
        throw syntax_error("unknown "+ t.str_);
    if(!safe && auto_escape())
        item = "lm::$escape("+item+")";
    code += item;
    code += ";" + br;

    return code;
}
//{% include "includes/nav.html" %}
std::string lemon::get_include_filepath()
{
    token_t t = get_next_token();
    eof_assert(t);
    std::string file_path;

    if(t.type_ == token_t::e_double_quote||
       t.type_ == token_t::e_quote)
    {
        do
        {
            token_t t1 = get_next_token();
            eof_assert(t1);
            if(t.type_ != t1.type_)
            {
                file_path.append(t1.str_);
            }
            else
            {
                if(get_next_token().type_ != token_t::e_close_block)
                    throw syntax_error("not find %}");
                break;
            }

        }while (true);
    }
    return file_path;
}
std::string lemon::parse_html_include()
{
    std::string file_path = get_include_filepath();

    lexer_ = new_lexer(file_path);
    if(!lexer_)
        throw std::runtime_error("new lexer error");
    lexers_.push_back(lexer_);

    push_status(token_t::e_include);

    std::string code = parse_html();

    if(pop_status() != token_t::e_include)
        throw syntax_error("status error");

    lexer_->file_->close();
    delete lexer_->file_;
    delete  lexer_;
    lexers_.pop_back();
    lexer_ = lexers_.back();
    return code;
}
void lemon::push_status(token_t::type_t type)
{
    status_.push_back(type);
}
lemon::token_t::type_t lemon::pop_status()
{
    if(status_.empty())
        return token_t::e_void;
    token_t::type_t type = status_.back();
    status_.pop_back();
    return type;
}
lemon::token_t::type_t lemon::get_status()
{
    if(status_.empty())
        throw syntax_error("[status empty]");
    token_t::type_t type = status_.back();
    return type;
}
std::string lemon::get_status_str()
{
    if(status_.empty())
        return "[status empty]";
    token_t::type_t type = status_.back();
    switch (type)
    {
        case token_t::e_for:
            return "for";
        case token_t::e_if:
            return "if";
        case token_t::e_include:
            return "include";
        case token_t::e_autoescape:
            return "autoescape";
        case token_t::e_block:
            return "block";
        default:
            return "unknown status";
    }
    return "error status";
}
lemon::block lemon::get_block(const std::string &name)
{
    for (size_t i = 0; i < blocks_.size(); ++i)
    {
        if(blocks_[i].name_ == name)
            return blocks_[i];
    }
    throw syntax_error("not block "+name);
    return block();
}
bool lemon::block_exist(const std::string &name)
{
    for (size_t i = 0; i < blocks_.size(); ++i)
    {
        if(blocks_[i].name_ == name)
            return true;
    }
    return false;
}
std::string lemon::parse_block()
{
    std::string code;
    std::string name = get_next_token().str_;
    if(get_next_token().type_ != token_t::e_close_block)
        throw syntax_error("not find %}");
    push_status(token_t::e_block);
    if(!block_exist(name))
        return code;
    block b = get_block(name);
    lexer *l = new lexer;
    l->file_ = b.file_;
    l->file_path_ = b.file_path_;
    l->line_buffer_ = b.line_buffer_;
    l->current_line_ = b.current_line_;
    l->line_ = b.line_;
    l->file_->clear();
    l->file_->seekg(b.offset_,std::ios::beg);
    if(!l->file_->good())
        throw std::runtime_error("file is not good: "+b.file_path_);
    lexer_ = l;
    lexers_.push_back(l);
    is_base_ = false;
    code = parse_html();
    is_base_ = true;
    delete l;
    lexers_.pop_back();
    lexer_ = lexers_.back();
    //skip parent block
    while(get_next_token().type_ != token_t::e_end_block);
    if(get_next_token().type_ != token_t::e_close_block)
        throw syntax_error("not find %}");

    return code;
}
std::string lemon::parse_extends()
{

    std::string file_name;
    do
    {
        token_t t =get_next_token();
        eof_assert(t);
        if(t.type_ != token_t::e_close_block)
            file_name.append(t.str_);
        else
            break;
    }while (true);

    do
    {
        token_t t =get_next_token();

        if(t.type_ == token_t::e_eof)
            break;
        if(t.type_ != token_t::e_block)
            continue;
        std::string block_name = get_next_token().str_;
        if(get_next_token().type_ != token_t::e_close_block)
            throw syntax_error("not find %}");
        block b;
        b.line_ = lexer_->line_;
        b.file_path_ = lexer_->file_path_;
        b.file_ = lexer_->file_;
        b.line_buffer_ = lexer_->line_buffer_;
        b.current_line_ = lexer_->current_line_;
        b.offset_ = lexer_->file_->tellg();
        b.name_ = block_name;
        blocks_.push_back(b);

    }while(true);

    lexer_ = new lexer;
    lexer_->file_ = new std::ifstream;
    lexer_->file_->clear();

    lexer_ ->line_ = 0;
    lexer_ ->file_->open(file_name.c_str());
    if(!lexer_ ->file_->good())
        throw syntax_error("open file error "+ file_name);
    lexers_.push_back(lexer_ );

    return parse_html();
}
std::string lemon::parse_open_block()
{
    code_buffer code;
    token_t t = get_next_token();

    eof_assert(t);
    if(t.type_ == token_t::e_include)
    {
        code += parse_html_include();
    }
    else if (t.type_ == token_t::e_for)
    {
        push_status(token_t::e_for);
        code += parse_for();
    }
    else if (t.type_ == token_t::e_empty)
    {
        if(get_status() != token_t::e_for)
            throw syntax_error("status error " + get_status_str());

        if (get_next_token().type_ != token_t::e_close_block)
            throw syntax_error("not find %}");

        g_tab--;
        code += tab() + "}" + br;
        code += tab() + "if(" + get_for_item()+".empty())"+br;
        code += tab() + "{"+br;
        g_tab++;
        code += tab() + "code += \"";
    }
    else if (t.type_ == token_t::e_endfor)
    {
        if(pop_status() != token_t::e_for)
            throw syntax_error("status error "+ get_status_str());
        g_tab--;
        code += tab() + "}"+br;
        pop_stack();
        pop_for_item();
        if (get_next_token().type_ != token_t::e_close_block)
            throw syntax_error("not find %}");
    }
    else if (t.type_ == token_t::e_if)
    {
        push_status(token_t::e_if);
        code += parse_if();
    }
    else if (t.type_ == token_t::e_elif)
    {
        if(get_status() != token_t::e_if)
            throw syntax_error("status error "+ get_status_str());
        code += tab() + "}" + br;
        code += tab() + "else ";
        code += parse_if();
    }
    else if (t.type_ == token_t::e_endif)
    {
        if(pop_status() != token_t::e_if)
            throw syntax_error("status error "+ get_status_str());

        g_tab--;
        code += tab() + "}" + br;
        if(get_next_token().type_ != token_t::e_close_block)
            throw syntax_error("not find %}");
    }
    else if(t.type_ == token_t::e_block)
    {
        code += parse_block();
    }
    else if(t.type_ == token_t::e_autoescape)
    {
        t = get_next_token();
        if(t.type_ == token_t::e_on)
            push_auto_escape(true);
        else if(t.type_ == token_t::e_off)
            push_auto_escape(false);
        else
            throw syntax_error("unknown token "+t.str_);
        if(get_next_token().type_ != token_t::e_close_block)
            throw syntax_error("not find %}");
        push_status(token_t::e_autoescape);
    }
    else if(t.type_ == token_t::e_endautoescape)
    {
        if(pop_status() != token_t::e_autoescape)
            throw syntax_error("status error. "+get_status_str());
        if(get_next_token().type_ != token_t::e_close_block)
            throw syntax_error("not find %}");
        pop_auto_escape();
    }
    return code;
}


std::string lemon::parse_html()
{
    code_buffer code;

    code += tab() + "code += \"";

    do
    {
        token_t t = get_next_token("");
        if (t.type_ == token_t::e_eof)
        {
            code +="\";"+br;
            break;
        }
        else if (t.type_ == token_t::e_open_variable)
        {
            code += CODE_END;
            code += parse_variable();
            code += CODE_BEGIN;
        }
        else if (t.type_ == token_t::e_open_block)
        {
            code += CODE_END;
            t = get_next_token();
            if(t.type_ == token_t::e_end_block)
            {
                if(pop_status() != token_t::e_block)
                    throw syntax_error("status error."+ get_status_str());
                if(get_next_token().type_ != token_t::e_close_block)
                    throw syntax_error("not find %}");
                if(!is_base_)
                    return code;
            }
            else if(t.type_ == token_t::e_extends)
            {
                return parse_extends();
            }
            push_back(t);
            code += parse_open_block();
            code += CODE_BEGIN;
        }
        else if (t.type_ == token_t::e_$r)
        {
            t = get_next_token("");
            eof_assert(t);
            code += CODE_END;
            code += CODE_BEGIN;
            if (t.type_ == token_t::e_$n)
                continue;
            code += t.str_;
        }
        else if(t.type_ == token_t::e_$n)
        {
            code += CODE_END;
            code += CODE_BEGIN;
        }
        else if (t.type_ == token_t::e_$t)
        {
            code += "\t";
        }
        else if(t.type_ == token_t::e_double_quote)
        {
            code += "\\\"";
        }
        else
        {
            code += t.str_;
        }
        
    } while (true);

    return code;
}
#if 0
static inline std::string skip_multi_space(const std::string &str)
{
    std::string buffer;
    for (size_t i = 0; i < str.size(); ++i)
    {
        char ch = str[i];
        if (ch == ' ')
        {
            buffer.push_back(' ');
            for (; i < str.size(); ++i)
            {
                ch = str[i];
                if (ch != ' ')
                    break;
            }
        }

        buffer.push_back(ch);
    }
    return buffer;
}
#endif
void lemon::push_auto_escape(bool escape)
{
    auto_escape_.push_back(escape);
}
void lemon::pop_auto_escape()
{
    auto_escape_.pop_back();
}
bool lemon::auto_escape()
{
    if(auto_escape_.empty())
        throw syntax_error("auto_escape syntax error");
    return auto_escape_.back();
}
void lemon::parse_template()
{

    token_t t = get_next_token();
    eof_assert(t);
    if (t.type_ != token_t::e_html_comment_begin)
        throw syntax_error("error not find template interface.");
    template_.interface_.str_ = get_string("-->");
    skip(template_.interface_.str_," ");
    template_.name_ = lexer_->file_path_;
    parse_interface();

    stack_ = template_.interface_.params_;
    push_auto_escape(true);
    is_base_ = true;

    g_tab = 1;
    std::string code;
    code += "#include \"lemon.hpp\"" + br+br;
    code += template_.interface_.str_+br;
    code += "{"+br;
    code += tab() + "std::string code;" +br;
    code += parse_html();
    code += tab()+"return code;"+br;
    code +="}"+br;
    std::cout << code;

    std::fstream file;
    std::string file_path = template_.name_;
    file_path += ".cpp";
    file.open(file_path.c_str(), std::ios::out);
    file.write(code.c_str(), code.size());
}
/////////////////////////////////////////////////////////////////////////////
bool lemon::check_file_done(const std::string &file_name)
{
    for (size_t i = 0; i < analyzed_files_.size(); ++i)
    {
        if(analyzed_files_[i] == file_name)
            return true;
    }
    return false;
}
//{%include filename%}
void lemon::parse_cpp_include()
{
    std::string file_path = get_include_filepath();
    lexer_ = new_lexer(file_path);
    if(!lexer_)
        throw std::runtime_error("new lexer error");
    lexers_.push_back(lexer_);
    parse_cpp_header();
    delete lexer_->file_;
    delete lexer_;
    lexer_ = lexers_.back();
}
void lemon::parse_cpp_header()
{
    namespaces_.push_back(namespaces_t());
    do
    {
        token_t t = get_next_token();
        if (t.type_ == token_t::e_eof)
        {
            namespaces_.pop_back();
            return;
        }
        if(t.type_ == token_t::e_cpp_comment)
        {
            t = get_next_token();
            if(t.type_ != token_t::e_open_block)
            {
                clear_line_buffer();
                continue;
            }
            t = get_next_token();
            if(t.type_ == token_t::e_include)
                return parse_cpp_include();
        }
        else if(t.type_ == token_t::e_cpp_comment_begin)
        {
            skip_cpp_comment();
        }
        else if (t.type_ == token_t::e_namespace)
        {
            namespaces_.back().push_back(get_next_token(true).str_);
            if (get_next_token(true).type_ != token_t::e_open_brace)
                throw syntax_error("not find { ");
        }
        else if (t.type_ == token_t::e_close_brace)
        {
            if (namespaces_.back().empty())
                throw syntax_error("{ not match }");
            namespaces_.back().pop_back();
        }
        else if(t.type_ == token_t::e_struct ||
                t.type_ == token_t::e_class)
        {
            parse_class(t.type_ == token_t::e_struct);
        }

    }while(true);
}
lemon::token_t lemon::get_next_token(bool auto_skip_comment)
{
    if(!auto_skip_comment)
        return get_next_token();
    token_t t;
    do
    {
        t = get_next_token();
        if(t.type_ == token_t::e_space||
                t.type_ == token_t::e_$n||
                t.type_ == token_t::e_$r||
                t.type_ == token_t::e_$t)
            continue;
        if(t.type_ == token_t::e_cpp_comment)
        {
            clear_line_buffer();
            continue;
        }
        else if(t.type_ == token_t::e_cpp_comment_begin)
            while(get_next_token().type_ != token_t::e_cpp_comment_end);
        else
            break;
    }while(true);
    return t;
}

lemon::fields_t lemon::get_variable(const std::string &name,
                                    const namespaces_t&nspaces)
{
    for (size_t i = 0; i < classes_.size(); ++i)
    {
        class_t &c = classes_[i];
        if(c.name_ == name)
        {
            if(c.namespaces_.empty() ||
               c.namespaces_ == nspaces)
                return c.variables_;
        }
    }
    throw syntax_error("not find class "+ name);
    return fields_t();
}
bool lemon::check_class_exist(const std::string &name, 
                                         const namespaces_t&nps)
{
    for (size_t i = 0; i < classes_.size(); ++i)
    {
        class_t &c = classes_[i];
        if (c.name_ == name)
        {
            if (c.namespaces_.size()&& c.namespaces_ == nps ||
                    c.namespaces_.empty() && nps.empty())
                return true;
        }
    }
    return false;
}
lemon::fields_t lemon::get_parent_variables(bool is_struct)
{
    std::vector<std::string> parents;
    std::vector<lemon::field> variables;

    do
    {
        token_t t = get_next_token(true);

        if(t.type_ == token_t::e_comma)
            t = get_next_token(true);
        if (t.type_ == token_t::e_open_brace)
        {
            break;
        }
        if(t.type_ == token_t::e_public || is_struct)
        {
            namespaces_t namespaces;

            token_t t1 = t;
            if(!is_struct)
                t1 = get_next_token(true);
            token_t t2 = get_next_token(true);
            do
            {
                //public namespace::
                if(t2.type_ == token_t::e_double_colon)
                {
                    namespaces.push_back(t1.str_);
                    t1= get_next_token(true);
                    t2 = get_next_token(true);
                    continue;
                }
                else
                {
                    push_back(t2);
                    if(namespaces.empty())
                        namespaces = namespaces_.back();
                    fields_t fields = get_variable(t1.str_, namespaces);
                    variables.insert(variables.end(),
                                     fields.begin(),fields.end());
                    break;
                }

            }while(true);
        }
        else if(t.type_ == token_t::e_private||
                t.type_ == token_t::e_protected)
        {
            while(true)
            {
                t = get_next_token();
                if(t.type_ == token_t::e_open_brace||
                        t.type_ == token_t::e_comma)
                    break;
            }
        }

    }while(true);

    return variables;
}
bool lemon::skip_to_public()
{
    int count = 1;
    do
    {
        token_t t = get_next_token(true);
        if(t.type_ == token_t::e_open_brace)
            count ++;
        else if (t.type_ == token_t::e_close_brace)
        {
            count--;
            if (count == 0)
                break;
        }
        if(t.type_ == token_t::e_public)
        {
            if(get_next_token(true).type_ != token_t::e_colon)
                throw syntax_error("not find :");
            break;
        }
    }while(true);

    if(count == 1)
        return true;
    else if(count == 0)
        return false;
    else
        throw syntax_error("not match { and }");

    return false;
}
lemon::namespaces_t lemon::get_namespaces()
{
    namespaces_t namespaces;

    do
    {
        token_t t1 = get_next_token(true);
        token_t t2 = get_next_token(true);
        if (t2.type_ == token_t::e_double_colon)
        {
            namespaces.push_back(t1.str_);
            continue;
        }
        push_back(t1);
        push_back(t2);
        break;
    } while (true);

    return namespaces;
}
lemon::field lemon::parse_field_type()
{
    field f;

    token_t t = get_next_token(true);
    eof_assert(t);

    if(t.type_ == token_t::e_std_vector||
            t.type_ == token_t::e_std_list||
            t.type_ == token_t::e_std_map ||
            t.type_ == token_t::e_std_set)
    {
        f.type_ = get_field_type(t);
        f.type_str_.append(t.str_);
        t = get_next_token(true);
        if(t.type_ != token_t::e_less)
            throw syntax_error("not find <");
        f.type_str_.append(t.str_);
        int count = 1;
        do
        {
            t = get_next_token(true);
            if (t.type_ == token_t::e_less)
            {
                count++;
                f.type_str_.append(t.str_);
            }
            else if (t.type_ == token_t::e_gt)
            {
                count--;
                f.type_str_.append(t.str_);
                if (count == 0)
                    break;
            }
            else if (t.type_ == token_t::e_identifier)
            {
                token_t t2 = get_next_token(true);
                if (t2.type_ != token_t::e_double_colon)
                {
                    push_back(t2);
                    if (!check_class_exist(t.str_, namespaces_.back()))
                        throw syntax_error("not find class " + t.str_);
                    t.str_ = to_string(namespaces_.back()) + t.str_;
                    f.type_str_.append(t.str_);
                }
                else
                {
                    push_back(t);
                    push_back(t2);
                    namespaces_t namespaces = get_namespaces();
                    t = get_next_token(true);
                    if (!check_class_exist(t.str_, namespaces))
                    {
                        namespaces_t ns = namespaces_.back();
                        ns.insert(ns.end(), namespaces.begin(), namespaces.end());
                        if (!check_class_exist(t.str_, ns))
                            throw syntax_error("not find class "+ 
                                               to_string(namespaces)+t.str_);

                        t.str_ = to_string(ns) + t.str_;
                        f.type_str_.append(t.str_);
                    }
                    else
                    {
                        t.str_ = to_string(namespaces) + t.str_;
                        f.type_str_.append(t.str_);
                    }
                }
            }
            else
            {
                f.type_str_.append(t.str_);
            }
        }while(true);
    }
    else if(t.type_ == token_t::e_bool||
            t.type_ == token_t::e_char||
            t.type_ == token_t::e_unsigned_char||
            t.type_ == token_t::e_short||
            t.type_ == token_t::e_unsigned_shot||
            t.type_ == token_t::e_int||
            t.type_ == token_t::e_unsigned_int||
            t.type_ == token_t::e_long||
            t.type_ == token_t::e_unsigned_long||
            t.type_ == token_t::e_long_long||
            t.type_ == token_t::e_unsigned_long_long||
            t.type_ == token_t::e_std_string||
            t.type_ == token_t::e_acl_string)
    {
        f.type_ = get_field_type(t);
        f.type_str_.append(t.str_);
    }
    else
    {
        push_back(t);
        namespaces_t namespaces = get_namespaces();
        t = get_next_token(true);
        if (!check_class_exist(t.str_, namespaces))
            throw syntax_error("not find class " + t.str_);
        f.type_ = field::e_class;
        f.type_str_ = t.str_;
        f.namespaces_ = namespaces;
    }

    return f;
}
void lemon::skip_function()
{
    int count = 0;

    do
    {
        token_t t = get_next_token(true);

        //virtual void hello() = 0;
        //virtual void hello();
        if (t.type_ == token_t::e_semicolon)
        {
            if (count == 0)
                return;
        }
        if (t.type_ == token_t::e_open_brace)
            count++;
        else if (t.type_ == token_t::e_close_brace)
        {
            count--;
            if (count == 0)
                return;
        }
            

    } while (true);
}
void lemon::parse_class(bool is_struct)
{
    class_t cls;
    cls.name_ = get_next_token(true).str_;
    token_t t2 = get_next_token(true);
    // class name ;
    if(t2.type_ == token_t::e_semicolon)
        return;
    else if(t2.type_ == token_t::e_colon)
    {
        cls.variables_ = get_parent_variables(is_struct);
    }
    if(!is_struct)
    {
        if(!skip_to_public())
            return;
    }
    do
    {
        token_t t = get_next_token(true);
        if (t.type_ == token_t::e_eof)
            break;
        if(t.type_ == token_t::e_protected||
                t.type_ == token_t::e_private)
        {
            if(!skip_to_public())
                break;
            else
                continue;
        }
        else if(t.type_ == token_t::e_inline||
                 t.type_ == token_t::e_virtual||
                 t.type_ == token_t::e_void)
        {
            //member function
            skip_function();
            continue;
        }
        else if(t.type_ == token_t::e_identifier)
        {
            //construct function
            if (t.str_ == cls.name_)
            {
                skip_function();
                continue;
            }
        }
        else if(t.type_ == token_t::e_close_brace)
        {
            if (get_next_token(true).type_ != token_t::e_semicolon)
                throw syntax_error("not find ;");
            break;//end of class 
        }
        push_back(t);

        field f = parse_field_type();
        f.name_ = get_next_token(true).str_;
        t = get_next_token();
        if (t.type_ == token_t::e_open_paren)
        {
            skip_function();
            continue;
        }
        else if (t.type_ == token_t::e_semicolon)
        {
            cls.variables_.push_back(f);
        }
        else
        {
            throw syntax_error("not find ;");
        }
    }while(true);

    cls.namespaces_ = namespaces_.back();
    classes_.push_back(cls);
}

void lemon::skip_cpp_comment()
{
    while(get_next_token().type_ != token_t::e_cpp_comment_end);
}

lemon::class_t * 
    lemon::get_class(const std::string &name,
                     const namespaces_t &nsp)
{
    for (size_t i = 0; i < classes_.size(); ++i)
    {
        if (classes_[i].name_ == name)
        {
            if (!!classes_[i].namespaces_.size() &&
                classes_[i].namespaces_ == nsp)
                return &classes_[i];
            else if (classes_[i].namespaces_.empty() &&
                     nsp.empty())
                return &classes_[i];
        }
    }
    return NULL;
}