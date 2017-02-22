#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <string.h>
#include <algorithm>
#include <typeinfo>
using namespace std;

class ReferenceManager {

};

class FunctionManager {

};

class BaseASTNode {
public:
    string identifier;
    string optype;
    vector<string> arguments;
    BaseASTNode *node = NULL;
};

class ASTFunctionNode : BaseASTNode{
    public:
    BaseASTNode function_nodes;

};

class ASTTree {
    public:
    ASTTree() {

    }

    ASTTree(BaseASTNode &noderef) {
        initial = &noderef;
    }
    void AddNode(BaseASTNode *node){
        BaseASTNode *currentnode = initial;
        while(true) {
            if(!currentnode->node){
                currentnode->node = node;
                break;
            }
        }
    }
    BaseASTNode* FindNode(string identifier, size_t start){
        BaseASTNode *currentnode = initial;
        size_t i = 0;
        while(true) {
            if(!currentnode->node){
                return NULL;
            }
            if((currentnode->identifier == identifier) && i >= start) {
                return currentnode;
            }
            i++;
        }

    }

    BaseASTNode* GetInitialNode(){
        return initial;
    }

    private:
    BaseASTNode *initial;
};
ASTTree AST;
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

int process_file(string filename) {
    cout << "Loading: " << filename << endl;
    std::vector<char> assembly;
    std::vector<std::string> lines;
    FILE *asmfile;
    asmfile = fopen(filename.c_str(),"r");
    if(!asmfile) {
        printf("The file needs to exist.\n");
        exit(-1);
    }
    size_t file_size = 0;
    fseek(asmfile,0L,SEEK_END);
    file_size = ftell(asmfile);
    rewind(asmfile);
    for(size_t i = 0;i < file_size;i++) {
        char *ptr = new char;
        fread(ptr,1,1,asmfile);
        assembly.push_back(*ptr);
        delete ptr;

    }
    std::string line;
    for(char s1 : assembly) {
        if(s1 == '\t') {
            line += " ";
            continue;
        }
        if(s1 == '\n') {
            lines.push_back(line);
            line = "";
            continue;
        }
        line += s1;
    }
    for(string &line : lines) {
        std::string::iterator new_end =
                std::unique(line.begin(), line.end(),
                [=](char lhs, char rhs){ return (lhs == rhs) && (lhs == ' '); }
                );
        line.erase(new_end, line.end());
        char k = 0;
        string oldline = line;
        bool clipline = false;
        line = "";
        for(size_t i = 0; i < oldline.size();i++) {
            string s =  "=*/+-^";
            if((s.find(oldline[i]) != string::npos) && (i != 0) && (oldline[i-1] != ' ') && (s.find(oldline[i-1]) == string::npos)) {
                oldline.insert(oldline.begin() + i,' ');
            } else if((s.find(oldline[i]) != string::npos) && (i != oldline.size() - 1) && (oldline[i+1] != ' ') && (s.find(oldline[i+1]) == string::npos)) {
                oldline.insert(oldline.begin() + (i+1), ' ');
            }
            if((oldline[i] != ' ') && !clipline) {
                k = oldline[i];
                line += oldline[i];
                clipline = true;

            } else if(clipline){
                line += oldline[i];
            }

        }
        cout << line << endl;
        if(k == '@') {
            cout << "This is a preprocessor line." << endl;
            line.erase(std::remove(line.begin(), line.end(), '@'), line.end());
            vector<string> ppline;
            string delim = " ";
            split_string(line,delim,ppline);
            if(ppline[0] == "load") {
                process_file(ppline[1] + ".unref");
            }
        } else if(k == '#') {
            cout << "This is a comment." << endl;
        } else {
            vector<string> ppline;
            string delim = " ";
            split_string(line,delim,ppline);
            string s = line;
            size_t start = s.find("+/*=(");
            if((start > 1) || (string("+-").find(s[start])) != string::npos) {
                if(s[start] == '(') {
                    if(s.find('{') != string::npos) {

                    } else {
                        string functionident = "";
                        for(size_t i = 0; i < start - 1; i++) {
                            functionident += s[i];

                        }
                        for(char i : functionident){
                            if(!isalnum(i)){
                                return -1;
                            }
                        }
                        BaseASTNode *node = new BaseASTNode;
                        node->identifier = functionident;
                        node->optype = "call";
                        node->arguments;
                    }
                } else if(s[start] = '=') {

                }
            } else {
                return -1;
            }

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
    BaseASTNode *initial = new BaseASTNode;
        initial->identifier = "start";
        AST = ASTTree(*initial);
    process_file("test.unref");
    BaseASTNode *node = AST.GetInitialNode();
    while(true){
        cout << node->identifier << endl;
        node = node->node;
    }
    return 0;
}
