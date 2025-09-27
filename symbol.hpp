#ifndef __SYMBOL_HPP__
#define __SYMBOL_HPP__

#include <map>
#include <utility>
#include "lexer.hpp"
using namespace std;

enum Data_Type { type_integer, type_byte, type_bool };
enum Parameter_Type { value, reference };
enum Header_Type { definition, declaration };

// ====================================================
//             Variable semantic analysis 
// ====================================================

struct VarSTEntry {
    Data_Type basic_type;
    vector<int> dims;

    VarSTEntry() {}
    VarSTEntry(Data_Type type, vector<int> dimensions) :
        basic_type(type), dims(dimensions) {}

    bool operator==(const VarSTEntry& other) const {
        if (dims.size() != other.dims.size() || basic_type != other.basic_type) return false;
        for (int i = 0; i < (int)dims.size(); i++) {
            if (dims[i] != 0 && other.dims[i] != 0 && dims[i] != other.dims[i]) return false;
        }
        return true;
    }

    bool operator!=(const VarSTEntry& other) const {
        return !(*this == other);
    }
};

class Var_Scope {
public:
    Var_Scope() {}

    void insert(string name, Data_Type type, vector<int> dimensions) {
        if (local_vars.find(name) != local_vars.end()) {
            printf("Warning: duplicate declaration of variable %s\n", name.c_str());
        }
        local_vars[name] = VarSTEntry(type, dimensions);
    }

    VarSTEntry* lookup(string name) {
        if (local_vars.find(name) == local_vars.end()) return nullptr;
        return &(local_vars[name]);
    }

private:
    map<string, VarSTEntry> local_vars;
};

class Var_Symbol_Table {
public:
    void insertVar(string name, Data_Type type, vector<int> dimensions) {
        scope_stack.back().insert(name, type, dimensions);
    }

    VarSTEntry* lookup(string name, int line) {
        for (auto s = scope_stack.rbegin(); s != scope_stack.rend(); s++) {
            VarSTEntry* e = s->lookup(name);
            if (e != nullptr) return e;
        }

        char msg[1000];
        snprintf(msg, sizeof(msg), "Variable %s not found", name.c_str());
        yyerror(msg, line); 
        return nullptr;
    }

    // This function is used solely for detection of
    // the scope depth of variables defined in external scopes
    // It is part of IR generation and not semantic analysis
    int findscope(string name) {
        int ret = scope_stack.size()-1;
        for (auto s = scope_stack.rbegin(); s != scope_stack.rend(); s++) {
            VarSTEntry* e = s->lookup(name);
            if (e != nullptr && ret!=scope_stack.size()-1) return ret;
            ret--;
        }

        return -1;
    }

    void insertScope() {
        scope_stack.push_back(Var_Scope());
    }

    void popScope() {
        scope_stack.pop_back();
    }

private:
    vector<Var_Scope> scope_stack;
};

extern Var_Symbol_Table vst;

// ====================================================
//             Function semantic analysis 
// ====================================================

struct FuncSTEntry {
    Data_Type* return_type;
    Header_Type head_type;
    vector<pair<string, VarSTEntry>> parameters;

    FuncSTEntry() {}
    FuncSTEntry(Data_Type* ret_type, Header_Type headtype, vector<pair<string, VarSTEntry>> params) :
        return_type(ret_type), head_type(headtype), parameters(params) {}

    bool operator==(const FuncSTEntry& other) const {
        if (return_type != nullptr && other.return_type != nullptr) {
            return *return_type == *other.return_type && parameters == other.parameters;
        }
        else if (return_type == nullptr && other.return_type == nullptr) {
            return parameters == other.parameters;
        }
        else return false;
    }
};

class Func_Scope {
public:
    Func_Scope() {}

    void insert(string name, Data_Type* return_type, Header_Type headtype, vector<pair<string, VarSTEntry>> params) {
        if (local_funcs.find(name) != local_funcs.end()) {
            if (local_funcs[name].head_type == definition) {
                printf("Warning: duplicate declaration of function %s\n", name.c_str());
            }
        }
        local_funcs[name] = FuncSTEntry(return_type, headtype, params);
    }

    FuncSTEntry* lookup(string name) {
        if (local_funcs.find(name) == local_funcs.end()) return nullptr;
        return &(local_funcs[name]);
    }

private:
    map<string, FuncSTEntry> local_funcs;
};

class Func_Symbol_Table {
public:
    void insertFunc(string name, Data_Type* return_type, Header_Type headtype, vector<pair<string, VarSTEntry>> params) {
        if (!scope_stack.empty()) scope_stack.back().insert(name, return_type, headtype, params);
    }

    FuncSTEntry* lookup(string name, int line) {
        for (auto s = scope_stack.rbegin(); s != scope_stack.rend(); s++) {
            FuncSTEntry* e = s->lookup(name);
            if (e != nullptr) return e;
        }

        char msg[1000];
        snprintf(msg, sizeof(msg), "Function %s not found", name.c_str());
        yyerror(msg, line); 
        return nullptr;
    }

    void insertScope() {
        scope_stack.push_back(Func_Scope());
    }

    void popScope() {
        scope_stack.pop_back();
    }

    int getLen() {
        return scope_stack.size();
    }

private:
    vector<Func_Scope> scope_stack;
};

inline Data_Type type_b = type_byte;
inline Data_Type type_i = type_integer;

inline void add_library_funcs(Func_Symbol_Table* FST) {
	vector<pair<string,VarSTEntry>> params;
	string name;

	//========================================
	//              IO (Write)
	//========================================

	// writeInteger
	name = "writeInteger";
	params.clear();
	params.push_back(make_pair("n",VarSTEntry(type_integer,vector<int>{})));
	FST->insertFunc(name,nullptr,definition,params);

	// writeByte
	name = "writeByte";
	params.clear();
	params.push_back(make_pair("b",VarSTEntry(type_byte,vector<int>{})));
	FST->insertFunc(name,nullptr,definition,params);

	// writeChar
	name = "writeChar";
	params.clear();
	params.push_back(make_pair("b",VarSTEntry(type_byte,vector<int>{})));
	FST->insertFunc(name,nullptr,definition,params);

	// writeString
	name = "writeString";
	params.clear();
	params.push_back(make_pair("s",VarSTEntry(type_byte,vector<int>{0})));
	FST->insertFunc(name,nullptr,definition,params);

	//========================================
	//              IO (Read)
	//========================================

	// readInteger
	name = "readInteger";
	params.clear();
	FST->insertFunc(name,&type_i,definition,params);

	// readByte
	name = "readByte";
	FST->insertFunc(name,&type_b,definition,params);

	// writeChar
	name = "readChar";
	FST->insertFunc(name,&type_b,definition,params);

	// readString
	name = "readString";
	params.push_back(make_pair("n",VarSTEntry(type_integer,vector<int>{})));	
	params.push_back(make_pair("s",VarSTEntry(type_byte,vector<int>{0})));
	FST->insertFunc(name,nullptr,definition,params);

	//========================================
	//            Type Conversion
	//========================================

	// extend
	name = "extend";
	params.clear();
	params.push_back(make_pair("b",VarSTEntry(type_byte,vector<int>{})));
	FST->insertFunc(name,&type_i,definition,params);

	// shrink
	name = "shrink";
	params.clear();
	params.push_back(make_pair("n",VarSTEntry(type_integer,vector<int>{})));
	FST->insertFunc(name,&type_b,definition,params);
		
	//========================================
	//           String Functions
	//========================================

	// strlen
	name = "strlen";
	params.clear();
	params.push_back(make_pair("s",VarSTEntry(type_byte,vector<int>{0})));
	FST->insertFunc(name,&type_i,definition,params);

	// strcmp
	name = "strcmp";
	params.clear();
	params.push_back(make_pair("s1",VarSTEntry(type_byte,vector<int>{0})));
	params.push_back(make_pair("s2",VarSTEntry(type_byte,vector<int>{0})));
	FST->insertFunc(name,&type_i,definition,params);

	// strcpy
	name = "strcpy";
	params.clear();
	params.push_back(make_pair("trg",VarSTEntry(type_byte,vector<int>{0})));
	params.push_back(make_pair("src",VarSTEntry(type_byte,vector<int>{0})));
	FST->insertFunc(name,nullptr,definition,params);

	// strcat
	name = "strcat";
	params.clear();
	params.push_back(make_pair("trg",VarSTEntry(type_byte,vector<int>{0})));
	params.push_back(make_pair("src",VarSTEntry(type_byte,vector<int>{0})));
	FST->insertFunc(name,nullptr,definition,params);

}

extern Func_Symbol_Table fst;

// Vector used for declared functions that haven't been defined in the code
extern vector<pair<string,FuncSTEntry>> declared_functions;

// Vector used for checking that a return command returns the 
// appropriate type of a function
extern vector<pair<string,Data_Type*>> return_type_vec;


// ====================================================
//            Loop naming semantic analysis 
// ====================================================

extern vector<string> loop_stack;

#endif
