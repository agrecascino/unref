#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <string.h>
#include <algorithm>
#include <typeinfo>
#include <fstream>
#include <memory>
#include <llvm/ADT/STLExtras.h>

using namespace std;

class ReferenceManager {

};

class FunctionManager {

};
enum TokenNumber {
    tok_file_end = -1,
    tok_keyword = -2,
    tok_value = -3,
    tok_word = -4

};

struct Token {
    int tokenid;
    std::string str;
};

class TypeDeclaration {
    std::string type;
    bool unref;
    bool constant;
    bool ptr;
public:
    TypeDeclaration(const std::string &type,
                    const bool &unref, const bool &constant, const bool &ptr)
        : type(type), unref(unref), constant(constant), ptr(ptr) {}

};

class ExprAST {
public:
    virtual ~ExprAST() {}
};

class NumberExprAST : public ExprAST {
    void* val;
    std::unique_ptr<TypeDeclaration> decl;
public:
    NumberExprAST(void* val, std::unique_ptr<TypeDeclaration> decl) : val(val), decl(decl) {}
};

class VariableExprAST : public ExprAST {
    std::string name;
    std::unique_ptr<TypeDeclaration> decl;
public:
    VariableExprAST(const std::string &name,
                    const std::unique_ptr<TypeDeclaration> &decl)
        : name(name), decl(decl)  {}
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

class StructPrototypeAST {
    std::string name;
    vector<std::unique_ptr<TypeDeclaration>> decl;
public:
    StructPrototypeAST(const std::string &name,
                       std::vector<std::unique_ptr<TypeDeclaration>> decl)
        : name(name), decl(decl) {}
};

class PrototypeAST {
    std::string name;
    std::vector<std::unique_ptr<VariableExprAST>> args;
    TypeDeclaration decl;
public:
    PrototypeAST(const std::string &name,
                 std::vector<std::unique_ptr<VariableExprAST>> args,
                 const TypeDeclaration &decl)
        : name(name), args(std::move(args)) , decl(std::move(decl)) {}

        const std::string &getName() const { return name; }

        const TypeDeclaration &getDecl() const { return decl; }
};

class FunctionAST {
    std::unique_ptr<PrototypeAST> proto;
    std::unique_ptr<ExprAST> body;
public:
    FunctionAST(std::unique_ptr<PrototypeAST> proto,
                std::unique_ptr<ExprAST> body)
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
            while(isalnum(lastchar = f.get())) {
                token.str += lastchar;
            }
            if(iskeyword(token.str)) {
                token.tokenid = tok_keyword;
                return token;
            }
            token.tokenid = tok_word;
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
    string keyword_list[17] = {"unref","deref","ptr","const","string","implicit","if","int64","int32","int16","int8","uint64","uint32","uint16","uint8","float32","float64"};
    int lastchar  = ' ';
    fstream f;
};

class Parser {
    Parser(Tokenizer tokenizer) : tokenizer(std::move(tokenizer)) {

    }

private:
    Tokenizer tokenizer;
    Token current_token;
    Token getNextToken() {
        return current_token = tokenizer.getToken();
    }
    std::unique_ptr<ExprAST> ParseNumberExpr(void* numval, TypeDeclaration decl) {
        auto result = llvm::make_unique<NumberExprAST>(numval,decl);
        getNextToken();
        return std::move(result);
    }

    std::unique_ptr<ExprAST> ParseParenExpr() {
        getNextToken();
        auto v = ParseExpression();
        if(!v)
            return nullptr;

        if(current_token != ')')
            return LogError("expected ')'");
        getNextToken();
        return v;
    }

    std::unique_ptr<ExprAST> ParseIdentifierExpr() {
        std::string identifier = current_token.str;

        getNextToken();

        if(current_token.tokenid != '(')
            return llvm::make_unique<VariableExprAST>(identifier);

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

        return llvm::make_unique<CallExprAST(identifier,std::move(args));
    }

    std::unique_ptr<ExprAST> ParsePrimary() {
        switch(current_token.tokenid) {
        default:
            return LogError("unknown token when expected an expression");
        case tok_word:
            break;
        }
    }

    std::unique_ptr<ExprAST> LogError(const char *str) {
        fprintf(stderr, "LogError: %s\n", str);
        return nullptr;
    }

    std::unique_ptr<PrototypeAST> LogErrorP(const char *str) {
        LogError(str);
        return nullptr;
    }

    int isstruct(string name) {

    }
};

int process_file(string filename) {
    cout << "Loading: " << filename << endl;
    Tokenizer t(filename);
    while(true) {
        Token token = t.getToken();
        cout << token.str << endl;
        cout << token.tokenid << endl;
        if(token.tokenid == tok_file_end) {
            break;
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    if(argc < 1) {
        printf("You need to specify a file.\n");
        exit(-1);
    }
    process_file("test.unref");
    return 0;
}

