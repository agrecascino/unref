#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <string.h>
#include <algorithm>
#include <typeinfo>
#include <fstream>
#include <memory>
#include <map>
#include <llvm/ADT/STLExtras.h>
#include <boost/spirit/include/lex_lexertl.hpp>
using namespace std;
namespace lex = boost::spirit::lex;
class ReferenceManager {

};

class FunctionManager {

};
enum TokenNumber {
    tok_file_end = -1,
    tok_keyword = -2,
    tok_value = -3,
    tok_word = -4,
    tok_struct = -5,
    tok_string = -6

};

struct Token {
    int tokenid;
    std::string str;
};

class ExprAST {
public:
    virtual ~ExprAST() {}
};

class TypeDeclarationAST{
    std::string type;
    bool unref;
    bool constant;
    bool ptr;
public:
    TypeDeclarationAST(const std::string &type,
                       const bool &unref, const bool &constant, const bool &ptr)
        : type(type), unref(unref), constant(constant), ptr(ptr) {}

    const std::string &getType() const { return type; }

};



class NumberExprAST : public ExprAST {
    void* val;
    std::unique_ptr<TypeDeclarationAST> decl;
public:
    NumberExprAST(void* val, std::unique_ptr<TypeDeclarationAST> decl) : val(val), decl(std::move(decl)) {}
};

class VariableExprAST : public ExprAST {
    std::string name;
    std::unique_ptr<TypeDeclarationAST> decl;
    std::unique_ptr<ExprAST> arrayexpr;
    bool array;
public:
    VariableExprAST(const std::string &name,
                    std::unique_ptr<TypeDeclarationAST> decl, std::unique_ptr<ExprAST> arrayexpr, bool array)
        : name(name), decl(std::move(decl)), arrayexpr(std::move(arrayexpr)), array(array)  {}
};

class BinaryExprAST : public ExprAST {
    std::string op;
    std::unique_ptr<ExprAST> LHS, RHS;
public:
    BinaryExprAST(string op, std::unique_ptr<ExprAST> LHS,
                  std::unique_ptr<ExprAST> RHS)
        : op(op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
};

class CallExprAST : public ExprAST {
    std::string callee;
    std::vector<std::unique_ptr<ExprAST>> args;
public:
    CallExprAST(const std::string &callee,
                std::vector<std::unique_ptr<ExprAST>> args)
        : callee(callee), args(std::move(args)) {}
};

class StructPrototypeAST : public ExprAST {
    std::string name;
    std::vector<std::unique_ptr<ExprAST>> decl;
public:
    StructPrototypeAST(const std::string &name,
                       std::vector<std::unique_ptr<ExprAST>> decl)
        : name(name), decl(std::move(decl)) {}

    const std::string &getName() const { return name; }
};

class PrototypeAST {
    std::string name;
    std::vector<std::unique_ptr<VariableExprAST>> args;
    std::unique_ptr<TypeDeclarationAST> decl;
public:
    PrototypeAST(const std::string &name,
                 std::vector<std::unique_ptr<VariableExprAST>> args,
                 std::unique_ptr<TypeDeclarationAST> decl)
        : name(name), args(std::move(args)) , decl(std::move(decl)) {}

    const std::string &getName() const { return name; }

    const TypeDeclarationAST &getDecl() const { return *decl; }
};

class FunctionAST {
    std::unique_ptr<PrototypeAST> proto;
    vector<std::unique_ptr<ExprAST>> body;
public:
    FunctionAST(std::unique_ptr<PrototypeAST> proto,
                vector<std::unique_ptr<ExprAST>> body)
        : proto(std::move(proto)), body(std::move(body)) {}
};

void split_string(std::string const &k, std::string const &delim, std::vector<std::string> &output)
{
    // Due to the use of strpbrk here, this will stop copying at a null char. This is in fact desirable.
    char const *last_ptr = k.c_str(), *next_ptr;
    while ((next_ptr = strpbrk(last_ptr, delim.c_str())) != nullptr)
    {
        output.emplace_back(last_ptr, next_ptr - last_ptr);
        last_ptr = next_ptr + 1;
    }
    output.emplace_back(last_ptr);
}

class Tokenizer {
public:
    Tokenizer(string filename) {
        f = fstream(filename, std::fstream::in);
    }
    Token getToken() {
        Token token;
        while(isspace(lastchar)){
            lastchar = f.get();
        }
        if(isalpha(lastchar)) {
            token.str = lastchar;
            while(isalnum(lastchar = f.get()) || lastchar == '_') {
                token.str += lastchar;
            }
            if(token.str == "struct") {
                token.tokenid = tok_struct;
                return token;
            }
            if(iskeyword(token.str)) {
                token.tokenid = tok_keyword;
                return token;
            }
            token.tokenid = tok_word;
            return token;
        }
        if(lastchar == '\"') {
            int lookback = lastchar;
            token.tokenid = tok_string;
            while(true) {
                lastchar = f.get();
                if(lookback == '\\'){
                    if(lastchar == '\"') {
                        token.str += lastchar;
                        continue;
                    }
                }
                if(lastchar == '\"') {
                    lastchar = f.get();
                    break;
                }
                if(lastchar == EOF) {
                    token.tokenid = tok_file_end;
                    return token;
                }
                lookback = lastchar;
            }
            return token;
        }
        if(isdigit(lastchar) || lastchar == '.') {
            do {
                token.str += lastchar;
                lastchar = f.get();
            } while(isdigit(lastchar) || lastchar == '.');
            token.tokenid = tok_value;
            return token;
        }
        if(lastchar == '#') {
            do
                lastchar = f.get();
            while(f.get() != EOF && lastchar != '\n' && lastchar != '\r');
            if(lastchar != EOF)
                return getToken();

        }
        if(lastchar == EOF) {
            token.tokenid = tok_file_end;
            return token;
        }
        int thischar = lastchar;
        lastchar = f.get();
        token.tokenid = thischar;
        return token;
    }

private:
    int iskeyword(string s) {
        for(string i : keyword_list) {
            if(s == i) {
                return true;
            }
        }
        return false;
    }
    string keyword_list[18] = {"unref","deref","ptr","const","string","implicit","int64","int32","int16","int8","uint64","uint32","uint16","uint8","float32","float64","none"};
    int lastchar  = ' ';
    fstream f;
};

class Parser {
    friend class Tokenizer;
public:
    Parser(Tokenizer tokenizer) : tokenizer(std::move(tokenizer)) {
        BinopPrecedence['<'] = 10;
        BinopPrecedence['+'] = 20;
        BinopPrecedence['-'] = 20;
        BinopPrecedence['*'] = 40;
        getNextToken();
    }

    void MainLoop() {
        while(1) {
            switch(current_token.tokenid) {
                case tok_file_end:
                    return;
                case ';':
                    getNextToken();
                    break;
                case tok_keyword:
                    if(ParseDefinition() == nullptr) {
                        ParseIdentifierExpr();
                    }
                    break;
                default:
                    cout << current_token.tokenid << endl;
                    cout << current_token.str << endl;
                    cout << "Expressions not allowed at top level" << endl;
                    return;
            }
        }
    }

private:
    std::vector<StructPrototypeAST> types;
    std::map<char, int> BinopPrecedence;
    Tokenizer tokenizer;
    Token current_token; 
    int second_pass = 0;
    void* number_value;
    Token getNextToken() {
        current_token = tokenizer.getToken();
        cout << current_token.str << endl;
        cout << (char)current_token.tokenid << endl;
        cout << current_token.tokenid << endl;
        return current_token;
    }
    std::unique_ptr<ExprAST> ParseNumberExpr() {
        std::unique_ptr<TypeDeclarationAST> decl = InferValueTokenType();
        auto result = llvm::make_unique<NumberExprAST>(number_value, std::move(decl));
        getNextToken();
        return std::move(result);
    }

    std::unique_ptr<ExprAST> ParseParenExpr() {
        getNextToken();
        auto v = ParseExpression();
        if(!v)
            return nullptr;

        if(current_token.tokenid != ')')
            return LogError("expected ')'");
        getNextToken();
        return v;
    }

    std::unique_ptr<ExprAST> ParseIdentifierExpr() {
        string identifier = current_token.str;
        if((current_token.tokenid == tok_keyword) || current_token.tokenid == tok_word) {
            std::unique_ptr<TypeDeclarationAST> raincheck;
            if((current_token.tokenid == tok_keyword) || isstruct(current_token.str)) {
                raincheck = ParseType();
            } else {
                raincheck = llvm::make_unique<TypeDeclarationAST>("unknown",false, false, false);
            }
            std::unique_ptr<VariableExprAST> LHS = llvm::make_unique<VariableExprAST>(current_token.str,std::move(raincheck),llvm::make_unique<NumberExprAST>(NumberExprAST((void*)new int(1),llvm::make_unique<TypeDeclarationAST>(TypeDeclarationAST("uint32",true,true,false)))),false);
            getNextToken();
            if(current_token.tokenid == '(') {
                goto call;
            }
            string op;
            while(current_token.tokenid >= 0) {
                if(current_token.tokenid == ';') {
                    return LogError("The line ended a long time ago, and we are still going.");
                }
                op += current_token.tokenid;
                getNextToken();
            }
            if(op.empty()) {
                return LogError("I think you forgot an operator somewhere.");
            }
            std::unique_ptr<ExprAST> RHS;
            RHS = ParsePrimary();
            return llvm::make_unique<BinaryExprAST>(op, std::move(LHS), std::move(RHS));
        }
        call:
        getNextToken();
        std::vector<std::unique_ptr<ExprAST>> args;
        if(current_token.tokenid != ')') {
            while (1) {
                if(auto arg = ParseExpression())
                    args.push_back(std::move(arg));
                else
                    return nullptr;

                if(current_token.tokenid == ')')
                    break;

                if(current_token.tokenid != ',')
                    return LogError("Expected ')' or ', in argument list");
                getNextToken();
            }
        }

        getNextToken();

        return llvm::make_unique<CallExprAST>(identifier,std::move(args));
    }

    std::unique_ptr<ExprAST> ParseString() {
        std::unique_ptr<TypeDeclarationAST> decl = llvm::make_unique<TypeDeclarationAST>(TypeDeclarationAST("string", false, true, false));
        auto result = llvm::make_unique<NumberExprAST>((void*)new string(current_token.str), std::move(decl));
        getNextToken();
        return result;
    }

    std::unique_ptr<ExprAST> ParsePrimary() {
        switch(current_token.tokenid) {
        default:
            return LogError("unknown token when expected an expression");
        case tok_string:
            return ParseString();
        case tok_keyword:
            return ParseIdentifierExpr();
        case tok_word:
            return ParseIdentifierExpr();
        case tok_value:
            return ParseNumberExpr();
        case '(':
            return ParseParenExpr();
        }
    }

    int GetTokPrecedence() {
        if(!isascii(current_token.tokenid))
            return -1;
        int tok_prec = BinopPrecedence[current_token.tokenid];
        if(tok_prec <= 0) return -1;
        return tok_prec;
    }

    std::unique_ptr<ExprAST> ParseExpression() {
        auto LHS = ParsePrimary();
        if(!LHS)
            return nullptr;

        return ParseBinOpRHS(0,std::move(LHS));
    }

    std::unique_ptr<ExprAST> ParseBinOpRHS(int expr_prec, std::unique_ptr<ExprAST> LHS) {
        while(1) {
            int tok_prec = GetTokPrecedence();

            if(tok_prec < expr_prec)
                return LHS;

            int bin_op = current_token.tokenid;
            getNextToken();

            auto RHS = ParsePrimary();
            if(!RHS)
                return nullptr;

            int next_prec = GetTokPrecedence();
            if(tok_prec < next_prec) {
                RHS = ParseBinOpRHS(tok_prec + 1,std::move(RHS));
                if(!RHS)
                    return nullptr;
            }

            LHS = llvm::make_unique<BinaryExprAST>(to_string(bin_op), std::move(LHS), std::move(RHS));
        }
    }

    std::unique_ptr<FunctionAST> ParseDefinition() {
         cout << current_token.str << endl;
        auto proto = ParsePrototype();
        if(!proto) return nullptr;

        if(current_token.tokenid != '{') {
            return nullptr;
        }
        getNextToken();
        std::vector<std::unique_ptr<ExprAST>> expressions;
        while(current_token.tokenid != '}') {
            cout << current_token.tokenid << endl;
            if(current_token.tokenid == tok_file_end) {
                return nullptr;
            }
            if(current_token.tokenid == ';') {
                getNextToken();
                continue;
            }
            if(auto E = ParseExpression())
                expressions.push_back(std::move(E));
            else
                return nullptr;
        }
        getNextToken();
        return llvm::make_unique<FunctionAST>(std::move(proto), std::move(expressions));
    }

    std::unique_ptr<TypeDeclarationAST> InferValueTokenType() {
        if(current_token.tokenid != tok_value) {
            return nullptr;
        }
        if(current_token.str.find('.') != string::npos) {
            number_value = new float;
            *(float*)number_value = strtof(current_token.str.c_str(),NULL);
            return llvm::make_unique<TypeDeclarationAST>("float32",true,false,false);
        } else {
            if(current_token.str.find('-') != string::npos) {
                number_value = new int;
                *(int*)number_value = stoi(current_token.str);
                return llvm::make_unique<TypeDeclarationAST>("int32",true,false,false);
            } else {
                number_value = new unsigned int;
                *(unsigned int*)number_value = stoi(current_token.str);
                return llvm::make_unique<TypeDeclarationAST>("uint32",true,false,false);
            }
        }
    }

    std::unique_ptr<TypeDeclarationAST> ParseType() {
        bool constant = false;
        bool unref = false;
        bool ptr = false;
        std::string type;
        while(1) {
            cout << current_token.str << endl;
            if((current_token.tokenid == tok_keyword) || (isstruct(current_token.str))) {
                if(current_token.str == "const") {
                    constant = true;
                } else if(current_token.str == "unref") {
                    unref = true;
                } else if(current_token.str == "ptr") {
                    ptr = true;
                } else if(current_token.str == "deref") {
                    return LogErrorT("deref is not a typename");
                } else {
                    type = current_token.str;
                }
            } else {
                break;
            }
            getNextToken();
        }
        return llvm::make_unique<TypeDeclarationAST>(type,unref,constant,ptr);
    }

    std::unique_ptr<PrototypeAST> ParsePrototype() {
         cout << current_token.str << endl;
        std::unique_ptr<TypeDeclarationAST> decl = ParseType();
        if(!decl)
            return nullptr;
        if(decl->getType().empty()) {
            cout << decl->getType() << endl;
            return LogErrorP("no typename specified");
        }
        if(current_token.tokenid != tok_word)
            return LogErrorP("Expected function name in prototype");

        std::string fn_name = current_token.str;
        getNextToken();

        if(current_token.tokenid != '(')
            return LogErrorP("Expected '(' in prototype");

        std::vector<std::unique_ptr<VariableExprAST>> ArgsPrototypes;
        newarg:
        string name;
        std::unique_ptr<TypeDeclarationAST> arg_decl;
        while((getNextToken().tokenid == tok_keyword) || isstruct(current_token.str)){
            arg_decl =  ParseType();
            if(current_token.tokenid != tok_word)
                return LogErrorP("Unexpected token in argument list");
            name = current_token.str;
        }
        reitr:
        if(current_token.tokenid == '[') {
            getNextToken();
            std::unique_ptr<ExprAST> E = ParseExpression();
            if(E.get() == nullptr) {
                return LogErrorP("Failed to parse expression in array.");
            }
            ArgsPrototypes.push_back(llvm::make_unique<VariableExprAST>(name,std::move(arg_decl),std::move(E),true));
            getNextToken();
            goto reitr;
        } else if(current_token.tokenid == ',') {
            ArgsPrototypes.push_back(llvm::make_unique<VariableExprAST>(name,std::move(arg_decl),llvm::make_unique<NumberExprAST>(NumberExprAST((void*)new int(1),llvm::make_unique<TypeDeclarationAST>(TypeDeclarationAST("uint32",true,true,false)))),false));
            goto newarg;
        } else if(current_token.tokenid == ')') {
            ArgsPrototypes.push_back(llvm::make_unique<VariableExprAST>(name,std::move(arg_decl),llvm::make_unique<NumberExprAST>(NumberExprAST((void*)new int(1),llvm::make_unique<TypeDeclarationAST>(TypeDeclarationAST("uint32",true,true,false)))),false));
            goto finish;
        }
        else
            return LogErrorP("Expected ') in prototype");
        finish:
        getNextToken();
        return llvm::make_unique<PrototypeAST>(fn_name, std::move(ArgsPrototypes), std::move(decl));

    }

    std::unique_ptr<ExprAST> LogError(const char *str) {
        fprintf(stderr, "LogError: %s\n", str);
        return nullptr;
    }

    std::unique_ptr<PrototypeAST> LogErrorP(const char *str) {
        LogError(str);
        return nullptr;
    }

    std::unique_ptr<TypeDeclarationAST> LogErrorT(const char *str) {
        LogError(str);
        return nullptr;
    }


    int isstruct(string name) {
        for(StructPrototypeAST &st : types) {
            if(st.getName() == name) {
                return true;
            }
        }
        return false;
    }
};

int process_file(string filename) {
    cout << "Loading: " << filename << endl;
    Parser t = Parser(Tokenizer(filename));
    t.MainLoop();
    return 0;
}

int main(int argc, char *argv[])
{
    if(argc < 1) {
        printf("You need to specify a file.\n");
        exit(-1);
    }
    process_file("/root/build-unref-Desktop-Debug/test.unref");
    return 0;
}

