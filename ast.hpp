#ifndef AST_HPP
#define AST_HPP

#include <llvm/Pass.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Utils.h>

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>

#include "symbol.hpp"

using namespace std;
using namespace llvm;

inline const char* data_name[] = {"int","byte","bool"};
inline const char* param_name[] = {"value","reference"};

// Data structures used to find allocated variables easily 
extern vector<vector<pair<string,Type*>>> func_decl_stack;
extern vector<vector<pair<AllocaInst*,Type*>>> func_var_stack;

// Data structured used to hold metadata for the parameters
// of functions. Value of 1 means that the function parameter 
// is a dynamic array. Value of 2 mean it is a base type
// (int or byte) passed by value
extern map<Argument*,int> param_metadata;
extern map<string,int> aux_param_metadata;

// Data structure used for IR generation of loops
extern vector<pair<string,pair<BasicBlock*,BasicBlock*>>> loop_block_stack;
extern int loop_block_id;

// TODO

// variables with the same and different locality don't work
// ex. var n is int (in main) and var n is int (inside def swap)
// Revisit semantic analysis on functions that are declared but not defined
// Add semantic analysis for code that doesn't have break inside loops or return inside functions
// Semantic analysis for parameters passed by reference
// See code generation for functions that alter variables of outer functions
// Definition of functions with same name
// == operator for varstentry might be errogenous
// lineno in semantic errors is sometimes misplaced

class AST {
public:
    virtual void printAST(std::ostream &out) const = 0;
    virtual void sem_analysis() {}

    int get_line_num() const { return line_num; }
	void set_line(int line) { line_num = line; }

	virtual Value* igen() const { return nullptr; }

protected:
    int line_num;

	static LLVMContext TheContext;
	static IRBuilder<> Builder;
	static unique_ptr<Module> TheModule;
	static unique_ptr<legacy::FunctionPassManager> TheFPM;

	static Function *TheWriteInteger;
	static Function *TheWriteString;
	static Function *TheWriteChar;
	static Function *TheWriteByte;
	static Function *TheReadInteger;
	static Function *TheReadString;
	static Function *TheReadChar;
	static Function *TheReadByte;
	static Function *TheExtend;
	static Function *TheShrink;
	static Function *TheStrlen;
	static Function *TheStrcmp;
	static Function *TheStrcpy;
	static Function *TheStrcat;

	static Type *i1;
	static Type *i8;
	static Type *i32;

	static ConstantInt* c1(int n) {
		return ConstantInt::get(TheContext, APInt(1, n, true));
	}

	static ConstantInt* c8(char c) {
		return ConstantInt::get(TheContext, APInt(8, c, true));
	}

	static ConstantInt* c32(int n) {
		return ConstantInt::get(TheContext, APInt(32, n, true));
	}

	// Find the next Basic Block of a given BB
	static BasicBlock* set_bb_pos(BasicBlock* PrevBB) {
		auto InsertPos = next(Function::iterator(PrevBB));
		Function* TheFunction = PrevBB->getParent();
		if (InsertPos!=TheFunction->end()) return &*InsertPos;
		else return nullptr;

	}

	// Search identifier in function parameters
	static Argument *findParam(Function *F, StringRef name) {
		for (Argument &Arg : F->args()) {
			if (Arg.getName() == name)
				return &Arg;
		}
		return nullptr; // not found
	}

	// Return a vector of the function parameters
	vector<Argument*> getFunctionArgs(Function* F) {
    	vector<Argument*> args;
    	for (auto &arg : F->args()) {
        	args.push_back(&arg);
    	}
    	return args;
	}
};

inline ostream &operator << (ostream &out, const AST &ast) {
  ast.printAST(out);
  return out;
}

class Declaration : public AST {
public:
};

class Statement : public AST {
public:
	virtual bool is_break_or_cond() { return false; }
};

class Expression : public AST {
public:
	VarSTEntry get_var_type() { return var_type; }
	virtual bool is_lvalue() { return false; }
	virtual int get_arg_data() { return -1; } // used by the lvalue class 
protected:
	VarSTEntry var_type;
};


// Class for Identifiers
class Identifier : public Expression {
public:
	Identifier (char* Name) : name(Name) {}

	string get_name() { 
		if (name!=nullptr) return string(name); 
		else {
			printf("Identifier name is null");
			return "";
		}
	}

	void printAST (ostream &out) const override {
		out << "Id(" <<  name << ")";
	}
private:
	char* name;
};


// Class for integer constants
class Int_const : public Expression {
public:
	Int_const (int val) : value(val) {}
	
	int get_value() { return value; }

	void sem_analysis() override { var_type = VarSTEntry(type_integer,vector<int>{}); }

	Value* igen() const override { return c32(value); }

	void printAST (ostream &out) const override {
		out << "Int_Const(" << value << ")";
	}

private:
	int value;
};


// Class for boolean constants
class Bool_const : public Expression {
public:
	Bool_const (int val) : value(val) {}
	
	int get_value() { return value; }

	void sem_analysis() override { var_type = VarSTEntry(type_byte,vector<int>{}); }

	Value* igen() const override { return c8(value); }

	void printAST (ostream &out) const override {
		out << "Bool_Const(" << (value ? "true" : "false") << ")";
	}

private:
	int value;
};


// Class for character constants
class Char_const : public Expression {
public:
	Char_const (char val) : value(val) {}
	
	char get_value() { return value; }

	void sem_analysis() override { var_type = VarSTEntry(type_byte,vector<int>{}); }

	Value* igen() const override { return c8(value); }

	void printAST (ostream &out) const override {
		string printable_char;

		out << "Char_Const(";
		if (value=='\n') { printable_char = "\\n"; out << printable_char << ")"; }
		else if (value=='\t') { printable_char = "\\t"; out << printable_char << ")"; }
		else if (value=='\r') { printable_char = "\\r"; out << printable_char << ")"; }
		else if (value=='\0') { printable_char = "\\0"; out << printable_char << ")"; }
		else out << value << ")";
	}

private:
	char value;
};


// Class for string constants
class String_const : public Expression {
public:
	String_const (char* val) : value(val) {}
	
	void sem_analysis() override { var_type = VarSTEntry(type_byte,vector<int>{(int)string(value).size()}); }

	Value* igen() const override {
		string initial_str = string(value);
		string processed_str;

		for (int i=1; i<initial_str.size()-1; i++) {
			if (initial_str[i]=='\\') {
				switch (initial_str[i+1]) {
					case 'n':
						processed_str.push_back('\n');
						break;	
					case 't':
						processed_str.push_back('\t');
						break;
					case 'r':
						processed_str.push_back('\r');
						break;
					case '\\':
						processed_str.push_back('\\');
						break;
				}
				i++;
			}
			else processed_str.push_back(initial_str[i]);
		}

		Constant *ConstStr = ConstantDataArray::getString(TheContext, processed_str, true);
		Type* strtype = ArrayType::get(Type::getInt8Ty(TheContext), processed_str.size()+1);

		GlobalVariable *GV = new GlobalVariable(*TheModule,strtype,false,GlobalValue::PrivateLinkage,ConstStr,"const_string");
		return GV;
	} 

	string get_value() {
		if (value!=nullptr) return string(value); 
		else {
			printf("String literal is null");
			return "";
		}
	}
	
	void printAST (ostream &out) const override {
		out << "String_Const(" << value << ")";
	}

private:
	char* value;
};


// Class for l-value
class L_value : public Expression {
public:	
	L_value (Identifier* Id, String_const* str) :
		id(Id), string_literal(str), expr_list() {}

	void append (Expression* expr) { expr_list.push_back(expr); }

    bool isArrayElement() const {
        return !expr_list.empty();
    }

	void sem_analysis() override {
		if (string_literal!=nullptr) { 
			string_literal->sem_analysis();
			var_type = string_literal->get_var_type();
			return; 
		}

		for (auto &expr: expr_list) {
			expr->sem_analysis();
			if (expr->get_var_type() != VarSTEntry(type_integer,vector<int> {})) {
				yyerror("Variable dimension cannot have a non-integer value", this->get_line_num());
			}
		}
		VarSTEntry var = *vst.lookup(id->get_name(),this->get_line_num());
		char msg[1000];

		if (expr_list.size() > var.dims.size()) {
			snprintf(msg,sizeof(msg),"Variable %s has dimensions %d but was called with dimensions %d",id->get_name().c_str(),var.dims.size(),expr_list.size());
			yyerror(msg, this->get_line_num());
		}
		for (int i=0; i<expr_list.size(); i++) {
			var.dims.erase(var.dims.begin());
		}

		var_type = var;

	}

	Value* igen() const override {
		// Case that lvalue is a string const
		if (string_literal!=nullptr) {
			return string_literal->igen(); //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		}

		vector<Value*> indices;
		Value* expr_res;
		int i;

		// Search identifer in allocated variables
		Type *Ty;
		for (i=0; i<func_decl_stack.back().size(); i++) {
			if (func_decl_stack.back()[i].first == id->get_name()) break;
		}

		// Then in function parameters
		if (i==func_decl_stack.back().size()) {
			Function *CurFunc = Builder.GetInsertBlock()->getParent();
			Argument* Arg = findParam(CurFunc,id->get_name());
			if (Arg==nullptr) yyerror("Not yet fixed",this->get_line_num());

			Ty = Arg->getType()->getPointerElementType();
		
			if (param_metadata[Arg]==3) return Arg;
			// Check if parameter is static or dynamic array	
			// Then, calculate expressions for indices
			if (param_metadata[Arg]!=1) indices.push_back(c32(0));
			for (auto &expr : expr_list) {
				expr_res = expr->igen();
				if (expr->is_lvalue()) expr_res = Builder.CreateLoad(expr_res->getType()->getPointerElementType(),expr_res);
				indices.push_back(expr_res);
			} 
			return Builder.CreateGEP(Ty,Arg,indices,id->get_name());
		}
		else {

			// Calculate expressions of indices
			indices.push_back(c32(0));
			for (auto &expr : expr_list) {
				expr_res = expr->igen();
				if (expr->is_lvalue()) expr_res = Builder.CreateLoad(expr_res->getType()->getPointerElementType(),expr_res);
				indices.push_back(expr_res);
			}

			AllocaInst* Alloc = func_var_stack.back()[i].first;
			Ty = func_var_stack.back()[i].second;
			if (Ty->isIntegerTy(32) || Ty->isIntegerTy(8)) return Alloc;
			else return Builder.CreateGEP(Ty,Alloc,indices,id->get_name());
		}

	}

	bool is_lvalue() override { return true; }

	int get_arg_data() override {
		if (string_literal!=nullptr) return -1;
		Function *CurFunc = Builder.GetInsertBlock()->getParent();
		Argument* Arg = findParam(CurFunc,id->get_name());
		if (Arg!=nullptr) return param_metadata[Arg];
		else return -1;	
	}

	void printAST (ostream &out) const override {
		out << "L-value(";
		if (id!=nullptr) out << *id;
		if (string_literal!=nullptr) out << *string_literal;
		if (!expr_list.empty()) {
			for (const auto &expr : expr_list) out << "," << *expr;
		}
		out << ")";
	}

private:
	Identifier* id;
	String_const* string_literal;
	vector<Expression*> expr_list;	
};


// Class for parameter type
class Param_type : public Declaration {
public:
	Param_type( vector<Int_const*>* Int_list, Parameter_Type par_type, Data_Type* dtype) :
		int_list(Int_list), parameter_type(par_type), data_type(dtype) {}

	VarSTEntry get_param_type() {
		vector<int> dims ;
		if (int_list != nullptr) {
			for (auto &intval : *int_list) {	
				if (intval!=nullptr) {
					if (intval->get_value() > 0) dims.push_back(intval->get_value());
					else yyerror("Parameter dimensions cannot be zero", this->get_line_num());
				}
				else dims.push_back(0);
			}
		}
		return VarSTEntry(*data_type,dims);
	}

	Type* get_Type() {
		vector<int> dims;
		Type* base_type;
		Type *Ty;

		if (*data_type == type_integer) { base_type = Type::getInt32Ty(TheContext); }
		else { base_type = Type::getInt8Ty(TheContext); }

		if (int_list!=nullptr) {
			if (!int_list->empty()) {
				for (auto &intval : *int_list) {	
					if (intval!=nullptr) dims.push_back(intval->get_value());
					else dims.push_back(0);
				}

				if (dims[0]==0 && dims.size()==1) return PointerType::get(base_type, 0);
				else if (dims.size()==1) return PointerType::get(ArrayType::get(base_type, dims[0]),0);

				Ty = ArrayType::get(base_type, dims.back());
				for (int i=dims.size()-2; i>=1; --i) {
					Ty = ArrayType::get(Ty, dims[i]);
				}

				if (dims[0]!=0) {
					Ty = ArrayType::get(Ty, dims[0]);
				}

				return PointerType::get(Ty, 0);
			}
			else {
				return base_type;
			}	
		}
		else {
			return PointerType::get(base_type, 0);
		}

	}

	int get_param_metadata() {
		if (int_list!=nullptr) { 
			if (!int_list->empty()) if ((*int_list)[0]==nullptr) return 1; 
			if (parameter_type==value && int_list->empty()) return 2;
			return 0;
		}
		return 3;
	}

	void printAST (ostream &out) const override {
		out << "Data_Type(" << data_name[*data_type] << "),Param_Type(" << param_name[parameter_type] << ")";
		if (int_list!=nullptr) { 
			for (auto &intval : *int_list) if (intval!=nullptr) out << "," << *intval; else out << ",null";		
		}
		out << ")";
	}

private:
	vector<Int_const*>* int_list;
	Data_Type* data_type;
	Parameter_Type parameter_type;
};


// Class for parameter
class Parameter : public Declaration {
public:
	Parameter (vector<Identifier*>* Id_list, Param_type* par_type) :
		id_list(Id_list), param_type(par_type) {}
 
	vector<pair<string,VarSTEntry>> get_param_vector() { return params; }

	void sem_analysis() override {
		VarSTEntry param_chars = param_type->get_param_type();
		params.clear();

		for (auto &id : *id_list) {
			params.push_back(make_pair(id->get_name(),param_chars));
		}
	}

	vector<pair<string,Type*>> get_Params() {
		vector<pair<string,Type*>> parameters;
		Type* par_Type = param_type->get_Type();
		
		for (auto &id : *id_list) {
			parameters.push_back(make_pair(id->get_name(),par_Type));
		}

		// Parameters passed by value need to be also allocated
		// This way they are mutable (can be altered)
		if (par_Type->isIntegerTy(32) || par_Type->isIntegerTy(8)) {
			for (auto &id : *id_list) func_decl_stack.back().push_back(make_pair(id->get_name(),par_Type));
		}

		// Store info that array is dynamic
		int metadata = param_type->get_param_metadata();
		for (auto &id : *id_list) aux_param_metadata[id->get_name()] = metadata;

		return parameters;
	}

	void printAST (ostream &out) const override {
		out << "Param(";
		for (auto& id : *id_list) out << *id << ","; 
		out << *param_type << ")";
	}

protected:
	vector<pair<string,VarSTEntry>> params;
private:	
	vector<Identifier*>* id_list;
	Param_type* param_type;
};


// Class for function headers
class Func_Header : public Declaration {
public:
	Func_Header (Identifier* Id, Header_Type head_type, Data_Type* ret_type, vector<Parameter*>* par_list) :
		id(Id), header_type(head_type), return_type(ret_type), param_list(par_list) {}

	void set_to_decl() { header_type = declaration; }

	void sem_analysis() override {
		vector<pair<string,VarSTEntry>> total_params;

		if (param_list != nullptr) {
			for (auto &param : *param_list) {
				param->sem_analysis();
				vector<pair<string,VarSTEntry>> param_vec = param->get_param_vector();
 				for (auto &element : param_vec) total_params.push_back(element);
			}

			if (header_type == definition) {
				for (auto &element : total_params) {
					vst.insertVar(element.first,element.second.basic_type,element.second.dims);
				}
			}
		}

		fst.insertFunc(id->get_name(),return_type,header_type,total_params);

		// Declared functions analysis
		pair<string,FuncSTEntry> func_pair = make_pair(id->get_name(),FuncSTEntry(return_type,header_type,total_params));
		
		if (header_type == declaration) declared_functions.push_back(func_pair);
		else {
			auto pos = find(declared_functions.begin(),declared_functions.end(),func_pair);
			if ( pos != declared_functions.end()) declared_functions.erase(pos);
		}

		// Return type analysis
		if (header_type == definition) return_type_vec.push_back(make_pair(id->get_name(),return_type));

	}

	Value* igen() const override {
		vector<string> Parameter_Names;
		vector<Type*> Parameter_Types;
		vector<pair<string,Type*>> temp_params;
		FunctionType* FuncType;
		unsigned Idx = 0;

		if (header_type==definition) {
			if (param_list != nullptr) {
				for (auto &param : *param_list) {
					temp_params = param->get_Params();
 					for (auto &element : temp_params) { 
						Parameter_Names.push_back(element.first);
						Parameter_Types.push_back(element.second);
					}
				}
				if (return_type == nullptr) FuncType = FunctionType::get(Type::getVoidTy(TheContext),Parameter_Types,false);
				else if (*return_type == type_integer) FuncType = FunctionType::get(Type::getInt32Ty(TheContext),Parameter_Types,false);
				else FuncType = FunctionType::get(Type::getInt8Ty(TheContext),Parameter_Types,false);
			}
			else {
				if (return_type == nullptr) FuncType = FunctionType::get(Type::getVoidTy(TheContext),{},false);
				else if (*return_type == type_integer) FuncType = FunctionType::get(Type::getInt32Ty(TheContext),{},false);
				else FuncType = FunctionType::get(Type::getInt8Ty(TheContext),{},false);
			}

			Function* Func = Function::Create(FuncType,Function::InternalLinkage,id->get_name(),TheModule.get());
			for (auto &Arg : Func->args()) { 
				Arg.setName(Parameter_Names[Idx]); 
				param_metadata[&Arg] = aux_param_metadata[Parameter_Names[Idx]];
				Idx++;
			}

			return Func;
		}
		return nullptr;
	}

	string get_func_name() { return id->get_name(); }

	void printAST (ostream &out) const override{
		out << "Func_header(" << *id << ",Type(" << header_type << ")"; 
		if (return_type!=nullptr) out << ",Return_type(" << data_name[*return_type] << ")";
		if (param_list!=nullptr) {
			out << ",Param_list(";
			bool first = true;
			for (auto &param : *param_list) {
				if (!first) out << ",";
				first = false;
				out << *param;
			}
			out << ")";
		}
		out << ")";
	}

private:
	Identifier* id;
	Header_Type header_type;
	Data_Type* return_type;
	vector<Parameter*>* param_list;
};


// Class for list of declarations
class Decl_list : public Declaration {
public:
	Decl_list () : decl_list() {}

	void append (Declaration* decl) { decl_list.push_back(decl); }

	void sem_analysis() override {
		for (auto &decl : decl_list) decl->sem_analysis();
	}

	Value* igen() const override {
		for (auto &decl : decl_list) decl->igen();
		return nullptr;
	}

	void printAST (ostream &out) const override {
		out << "Decl_list(";
		bool first = true;
		for (const auto &decl : decl_list) {
			if (!first) out << ",";
			first = false;
			out << *decl;
		}
		out << ")";
	}

private:
	vector<Declaration *> decl_list;
};


// Class for variable declaration
class Variable_decl : public Declaration {
public:
	Variable_decl (vector<Identifier*>* Var_list, vector<Int_const*>* Int_list, Data_Type* dtype) :
		var_list(Var_list), int_list(Int_list), data_type(dtype) {}

	void sem_analysis() override {
		vector<int> dims;
		
		for (auto &intval : *int_list) {
			if (intval!=nullptr) {
				if (intval->get_value() > 0) dims.push_back(intval->get_value());
				else yyerror("Parameter dimensions cannot be zero", this->get_line_num());
			}
			else dims.push_back(0);
		}
		for (auto &id : *var_list) vst.insertVar(id->get_name(),*data_type,dims);
	}

	Value* igen() const override {
		vector<int> dims;
		Type* base_type;
		Type *Ty;

		if (*data_type == type_integer) { base_type = Type::getInt32Ty(TheContext); }
		else { base_type = Type::getInt8Ty(TheContext); }

		if (!int_list->empty()) {
			for (auto &intval : *int_list) { dims.push_back(intval->get_value()); }
			Ty = ArrayType::get(base_type, dims.back());
			for (int i=dims.size()-2; i>=0; --i) {
				Ty = ArrayType::get(Ty, dims[i]);
			}
		}
		else {
			Ty = base_type;
			//Ty = ArrayType::get(base_type, 1);
		}

		for (auto &var : *var_list) {
			func_decl_stack.back().push_back(make_pair(var->get_name(),Ty));
		}
		return nullptr;
	}

	void printAST (ostream &out) const override {
		out << "Var_decl(Data_type(" << data_name[*data_type] << ")";
		for (auto &id : *var_list) out << "," << *id;
		for (auto &intval : *int_list) if (intval!=nullptr) out << "," << *intval; else out << ",null";
		out << ")";
	}	

private:
	vector<Identifier*>* var_list;
	vector<Int_const*>* int_list;
	Data_Type* data_type;
};


// Class for blocks
class Block : public Statement {
public:
	Block () : stmt_list() {}

	void append (Statement* stmt) { stmt_list.push_back(stmt); }

	void sem_analysis() override {
		for (auto &stmt : stmt_list) stmt->sem_analysis();
	}

	Value* igen() const override {
		for (auto &stmt : stmt_list) {
			stmt->igen();
			if (stmt->is_break_or_cond()) break;
		}
		return nullptr;
	}

	void printAST (ostream &out) const override {	
		out << "Block(";
		bool first = true;
		for (const auto &stmt : stmt_list) {
			if (!first) out << ",";
			first = false;
			out << *stmt;
		}
		out << ")";
	}

private:
	vector<Statement*> stmt_list;
};


// Class for function definitions
class Func_Def : public Declaration {
public:
	Func_Def (Func_Header *head, Decl_list *decl_list, Block *blk) :
		header(head), declaration_list (decl_list), function_block(blk) {}
	
	void sem_analysis() override {
		int prev_len = fst.getLen();

		vst.insertScope();
		header->sem_analysis();
		fst.insertScope();
		if (fst.getLen() == 1 && prev_len == 0) add_library_funcs(&fst);

		declaration_list->sem_analysis();
		function_block->sem_analysis();

		fst.popScope();
		vst.popScope();
		
		return_type_vec.pop_back();

		// Analysis for declared but undefined functions
		if (fst.getLen() == 0 && declared_functions.size() != 0) {
			ostringstream stringstream;
			bool first = true;
			char msg[1000];

			for (int i=0; i<declared_functions.size(); i++) {
				if (!first) stringstream << ",";
				first = false;
				stringstream << declared_functions[i].first;
			}
			snprintf(msg,sizeof(msg),"The following functions have been declared but never defined: %s",stringstream.str().c_str());
			yyerror(msg, this->get_line_num());
		}

	}

	Value* igen() const override {
		AllocaInst *Alloca;
		Argument* Arg;

		func_decl_stack.push_back(vector<pair<string,Type*>>{});
		func_var_stack.push_back(vector<pair<AllocaInst*,Type*>>{});

		// First define all inner functions, then this one
		declaration_list->igen();
		header->igen();

		// Create basic block
		Function *TheFunction = TheModule->getFunction(header->get_func_name());
		BasicBlock *BB = BasicBlock::Create(TheContext, "entry", TheFunction);
		Builder.SetInsertPoint(BB);

		// Allocate local variables
		for (int i=0; i<func_decl_stack.back().size(); i++) {
			Alloca = Builder.CreateAlloca(func_decl_stack.back()[i].second,nullptr,func_decl_stack.back()[i].first);
			Arg = findParam(TheFunction,func_decl_stack.back()[i].first);
			if (Arg!=nullptr) {
				Builder.CreateStore(Arg,Alloca);
			}
			func_var_stack.back().push_back(make_pair(Alloca,func_decl_stack.back()[i].second));
		}

		// Generate code for block
		function_block->igen();
		BasicBlock *PrevBB = Builder.GetInsertBlock();
		if (!PrevBB->getTerminator()) {
			Type *retType = TheFunction->getFunctionType()->getReturnType();
			if (retType->isIntegerTy(32)) Builder.CreateRet(c32(0));
			else if (retType->isIntegerTy(8)) Builder.CreateRet(c8(0)); 
			else Builder.CreateRetVoid();
		}

		func_decl_stack.pop_back();
		func_var_stack.pop_back();

		TheFPM->run(*TheFunction);
		return TheFunction;

	}

	void LLVM_IR_gen(bool optimize = false) {
		// Initialize
		TheModule = make_unique<Module>("Dana Program", TheContext);
		TheFPM = make_unique<legacy::FunctionPassManager>(TheModule.get());

		if (optimize) {
			// Reassociate expressions (e.g. 4+(x+5) to x+(4+5))
			TheFPM->add(createReassociatePass());
			// Simplify algebraic expressions and combine them
			TheFPM->add(createInstructionCombiningPass());
			// Sparse Conditional Constant Propagation
			TheFPM->add(createSCCPPass());
			// Promote memory references to register references
			TheFPM->add(createPromoteMemoryToRegisterPass());
			// Remove rendundant instructions
			TheFPM->add(createGVNPass());

			// Move loop invariants outside of loops
			TheFPM->add(createLICMPass());
			// Transform loops so that their counter starts at zero and increments by one 
			// Useful for loop unrolling
			TheFPM->add(createIndVarSimplifyPass()); 
			// Loop unrolling
			TheFPM->add(createLoopUnrollPass());   

			// Redo some optimizations
			TheFPM->add(createReassociatePass());
			TheFPM->add(createInstructionCombiningPass());
			TheFPM->add(createSCCPPass());
			TheFPM->add(createGVNPass());

			// Eliminate dead code (instruction level)
			TheFPM->add(createDeadCodeEliminationPass());
			// Simplify the CFG
			TheFPM->add(createCFGSimplificationPass());

		}
		TheFPM->doInitialization();

		// Initialize types
		i1 = IntegerType::get(TheContext,1);
		i8  = IntegerType::get(TheContext, 8);
		i32 = IntegerType::get(TheContext, 32);

		// Initialize library functions
		// writeInteger
		vector<Argument*> Args;
		FunctionType *writeInteger_type = FunctionType::get(Type::getVoidTy(TheContext), {i32}, false);
		TheWriteInteger = Function::Create(writeInteger_type, Function::ExternalLinkage,"writeInteger", TheModule.get());
		Args = getFunctionArgs(TheWriteInteger);
		param_metadata[Args[0]] = 2;

		// writeString
		FunctionType *writeString_type = FunctionType::get(Type::getVoidTy(TheContext),{PointerType::get(i8, 0)}, false);
		TheWriteString = Function::Create(writeString_type, Function::ExternalLinkage,"writeString", TheModule.get());
		Args = getFunctionArgs(TheWriteString);
		param_metadata[Args[0]] = 1;

		// writeChar
		FunctionType *writeChar_type = FunctionType::get(Type::getVoidTy(TheContext), {i8}, false);
		TheWriteChar= Function::Create(writeChar_type, Function::ExternalLinkage,"writeChar", TheModule.get());
		Args = getFunctionArgs(TheWriteChar);
		param_metadata[Args[0]] = 2;

		// writeByte
		FunctionType *writeByte_type = FunctionType::get(Type::getVoidTy(TheContext), {i8}, false);
		TheWriteByte = Function::Create(writeByte_type, Function::ExternalLinkage,"writeByte", TheModule.get());
		Args = getFunctionArgs(TheWriteByte);
		param_metadata[Args[0]] = 2;

		// readInteger
		FunctionType *readInteger_type = FunctionType::get(i32, {}, false);
		TheReadInteger = Function::Create(readInteger_type, Function::ExternalLinkage,"readInteger", TheModule.get());

		// readString
		FunctionType *readString_type = FunctionType::get(Type::getVoidTy(TheContext),{i32,PointerType::get(i8, 0)}, false);
		TheReadString = Function::Create(readString_type, Function::ExternalLinkage,"readString", TheModule.get());
		Args = getFunctionArgs(TheReadString);
		param_metadata[Args[0]] = 2;
		param_metadata[Args[1]] = 1;

		// readChar
		FunctionType *readChar_type = FunctionType::get(i8, {}, false);
		TheReadChar= Function::Create(readChar_type, Function::ExternalLinkage,"readChar", TheModule.get());

		// readByte
		FunctionType *readByte_type = FunctionType::get(i8, {}, false);
		TheReadByte = Function::Create(readByte_type, Function::ExternalLinkage,"readByte", TheModule.get());

		// extend
		FunctionType *extend_type = FunctionType::get(i32, {i8}, false);
		TheExtend = Function::Create(extend_type, Function::ExternalLinkage,"extend", TheModule.get());
		Args = getFunctionArgs(TheExtend);
		param_metadata[Args[0]] = 2;

		// shrink
		FunctionType *shrink_type = FunctionType::get(i8, {i32}, false);
		TheShrink = Function::Create(shrink_type, Function::ExternalLinkage,"shrink", TheModule.get());
		Args = getFunctionArgs(TheShrink);
		param_metadata[Args[0]] = 2;

		// strlen
		FunctionType *strlen_type = FunctionType::get(i32, {PointerType::get(i8, 0)}, false);
		TheStrlen = Function::Create(strlen_type, Function::ExternalLinkage,"strlen", TheModule.get());
		Args = getFunctionArgs(TheStrlen);
		param_metadata[Args[0]] = 1;
		
		// strcmp
		FunctionType *strcmp_type = FunctionType::get(i32, {PointerType::get(i8, 0), PointerType::get(i8, 0)}, false);
		TheStrcmp = Function::Create(strcmp_type, Function::ExternalLinkage,"strcmp", TheModule.get());
		Args = getFunctionArgs(TheStrcmp);
		param_metadata[Args[0]] = 1;
		param_metadata[Args[1]] = 1;

		// strcpy
		FunctionType *strcpy_type = FunctionType::get(Type::getVoidTy(TheContext), {PointerType::get(i8, 0), PointerType::get(i8, 0)}, false);
		TheStrcpy = Function::Create(strcpy_type, Function::ExternalLinkage,"strcpy", TheModule.get());
		Args = getFunctionArgs(TheStrcpy);
		param_metadata[Args[0]] = 1;
		param_metadata[Args[1]] = 1;

		// strcat
		FunctionType *strcat_type = FunctionType::get(Type::getVoidTy(TheContext), {PointerType::get(i8, 0), PointerType::get(i8, 0)}, false);
		TheStrcat = Function::Create(strcat_type, Function::ExternalLinkage,"strcat", TheModule.get());
		Args = getFunctionArgs(TheStrcat);
		param_metadata[Args[0]] = 1;
		param_metadata[Args[1]] = 1;

		// Emit the program code.
		AllocaInst *Alloca;

		func_decl_stack.push_back(vector<pair<string,Type*>>{});
		func_var_stack.push_back(vector<pair<AllocaInst*,Type*>>{});

		// First define all inner functions, then this one
		declaration_list->igen();

		// Create basic block
		FunctionType *main_type = FunctionType::get(Type::getVoidTy(TheContext), {}, false);
    	Function *main = Function::Create(main_type, Function::ExternalLinkage,"main", TheModule.get());
		BasicBlock *BB = BasicBlock::Create(TheContext, "entry", main);
		Builder.SetInsertPoint(BB);

		// Allocate local variables
		for (int i=0; i<func_decl_stack.back().size(); i++) {
			Alloca = Builder.CreateAlloca(func_decl_stack.back()[i].second,nullptr,func_decl_stack.back()[i].first);
			func_var_stack.back().push_back(make_pair(Alloca,func_decl_stack.back()[i].second));
		}

		// Generate code for block
		function_block->igen();
		Builder.CreateRetVoid();

		func_decl_stack.pop_back();
		func_var_stack.pop_back();

		// Verify the IR.
		bool bad = verifyModule(*TheModule, &errs());
		if (bad) {
			cerr << "The IR is bad!" << endl;
			TheModule->print(errs(), nullptr);
			exit(1);
		}

		// Optimize!
		TheFPM->run(*main);

		// Print out the IR.
		TheModule->print(outs(), nullptr);
	}

	void printAST (ostream &out) const override {
		out << "Function(" << *header << "," << *declaration_list << "," << *function_block << ")";
	}	

private:
	Func_Header* header;
	Decl_list* declaration_list;
	Block* function_block;
};


// Class for assignment
class Assignment : public Statement {
public:
    Assignment(L_value* Lval, Expression* Expr) : lval(Lval), expr(Expr) {}

    void sem_analysis() override {
        lval->sem_analysis();
        expr->sem_analysis();
        if (!lval->get_var_type().dims.empty() && !lval->isArrayElement()) {
            yyerror("Cannot assign to entire array", this->get_line_num());
        }
        if (lval->get_var_type() != expr->get_var_type()) {
            yyerror("Type mismatch in assignment", this->get_line_num());
        }
    }

    Value* igen() const override {
        Value* rhs = expr->igen();
        Value* lhs = lval->igen();
        if (expr->is_lvalue()) rhs = Builder.CreateLoad(rhs->getType()->getPointerElementType(), rhs);
        return Builder.CreateStore(rhs, lhs);
    }

    void printAST(std::ostream &out) const override {  // <--- must match exactly
        out << "Assign(" << *lval << "," << *expr << ")";
    }

private:
    L_value* lval;
    Expression* expr;
};



// Class for exit command
class Exit : public Statement {
public:
	Exit () {}

	void sem_analysis() override {
		Data_Type* func_return_type = return_type_vec[return_type_vec.size()-1].second;
		string func_name = return_type_vec[return_type_vec.size()-1].first;
		char msg[1000];

		if (func_return_type != nullptr) {
			snprintf(msg,sizeof(msg),"In function %s: cannot use exit command on function that returns %s",func_name.c_str(),
				data_name[*func_return_type]);
			yyerror(msg, this->get_line_num());
		}
	}

	Value* igen() const override {
		return Builder.CreateRetVoid();
	}

	void printAST (ostream &out) const override {
		out << "Exit";
	}

private:

};


// Class for return command
class Return : public Statement {
public:
	Return (Expression* Expr) :  expr(Expr) {}

	void sem_analysis() override {
		expr->sem_analysis();

		Data_Type* func_return_type = return_type_vec[return_type_vec.size()-1].second;
		string func_name = return_type_vec[return_type_vec.size()-1].first;
		char msg[1000];

		if (func_return_type == nullptr) {
			snprintf(msg,sizeof(msg),"In function %s: cannot use return command on a process",func_name.c_str());
			yyerror(msg, this->get_line_num());
		}
		if (expr->get_var_type() != VarSTEntry(*func_return_type,vector<int>{})) {
			snprintf(msg,sizeof(msg),"Function %s must return type %s",func_name.c_str(),data_name[*func_return_type]);
			yyerror(msg, this->get_line_num());
		}
	}

	Value* igen() const override {
		Value* result = expr->igen();
		if (expr->is_lvalue()) result = Builder.CreateLoad(result->getType()->getPointerElementType(),result);
		return Builder.CreateRet(result);
	}

	void printAST (ostream &out) const override {
		out << "Return(" << *expr << ")";
	}

private:
	Expression* expr;
};


// Class for break command
class Break : public Statement {
public:
	Break (Identifier* Id) : id(Id) {}

	void sem_analysis() override {
		char msg[1000];

		if (loop_stack.empty()) yyerror("Cannot use break command outside of a loop scope", this->get_line_num());
		
		if (id != nullptr) {
			if (find(loop_stack.begin(),loop_stack.end(),id->get_name()) == loop_stack.end()) {
				snprintf(msg,sizeof(msg),"Loop identifier %s does not exist",id->get_name().c_str());
				yyerror(msg, this->get_line_num());
			}
		}
	}

	Value* igen() const override {
		if (id!=nullptr) {
			for (int i=0; i<loop_block_stack.size(); i++) {
				if (loop_block_stack[i].first == id->get_name()) {
					return Builder.CreateBr(loop_block_stack[i].second.second);
				}
			}
			return nullptr;
		}
		else {
			return Builder.CreateBr(loop_block_stack.back().second.second);
		}
	}

	bool is_break_or_cond() override { return true; }

	void printAST (ostream &out) const override {
		if (id!=nullptr) out << "Break(" << *id << ")";
		else out << "Break";
	}

private:
	Identifier* id;
};


// Class for continue command
class Continue : public Statement {
public:
	Continue (Identifier* Id) : id(Id) {}

	void sem_analysis() override {
		char msg[1000];

		if (loop_stack.empty()) yyerror("Cannot use continue command outside of a loop scope", this->get_line_num());
		
		if (id != nullptr) {
			if (find(loop_stack.begin(),loop_stack.end(),id->get_name()) == loop_stack.end()) {
				snprintf(msg,sizeof(msg),"Loop identifier %s does not exist",id->get_name().c_str());
				yyerror(msg, this->get_line_num());
			}
		}
	}
	
	Value* igen() const override {
		if (id!=nullptr) {
			for (int i=0; i<loop_block_stack.size(); i++) {
				if (loop_block_stack[i].first == id->get_name()) {
					return Builder.CreateBr(loop_block_stack[i].second.first);
				}
			}
			return nullptr;
		}
		else {
			return Builder.CreateBr(loop_block_stack.back().second.first);
		}
	}

	bool is_break_or_cond() override { return true; }
	
	void printAST (ostream &out) const override {
		if (id!=nullptr) out << "Continue(" << *id << ")";
		else out << "Continue";
	}

private:
	Identifier* id;
};


// Class for loop command
class Loop : public Statement {
public:
	Loop (Identifier* Id, Block* blk) : id(Id), block(blk) {}

	void sem_analysis() override {
		char msg[1000];
		
		if (id != nullptr) {
			if (find(loop_stack.begin(),loop_stack.end(),id->get_name()) != loop_stack.end()) {
				snprintf(msg,sizeof(msg),"Identifier %s already used for a loop name",id->get_name().c_str());
				yyerror(msg, this->get_line_num());
			}
			loop_stack.push_back(id->get_name());
		}
		else loop_stack.push_back("");

		block->sem_analysis();

		loop_stack.pop_back();
	}

	Value* igen() const override {
		BasicBlock *PrevBB = Builder.GetInsertBlock();
		Function *TheFunction = PrevBB->getParent();
		BasicBlock *Loop = BasicBlock::Create(TheContext, "loop", TheFunction,set_bb_pos(PrevBB));
		BasicBlock *EndLoop = BasicBlock::Create(TheContext,"endloop",TheFunction,set_bb_pos(Loop));

		if (id!=nullptr) loop_block_stack.push_back(make_pair(id->get_name(),make_pair(Loop,EndLoop)));
		else loop_block_stack.push_back(make_pair(to_string(loop_block_id),make_pair(Loop,EndLoop)));

		Builder.CreateBr(Loop);
		Builder.SetInsertPoint(Loop);
		block->igen();
		PrevBB = Builder.GetInsertBlock();
		if (!PrevBB->getTerminator()) Builder.CreateBr(Loop);
		Builder.SetInsertPoint(EndLoop);

		loop_block_stack.pop_back();
		loop_block_id++;
		return nullptr;

	}

	void printAST (ostream &out) const override {
		out << "Loop(";
		if (id!=nullptr) out << *id << ",";
		out << *block << ")";
	}

private:
	Identifier* id;
	Block* block;
};


// Class for list of if conditions
class If : public Statement {
public:
	If () : cond_list(), if_list() {}

	void append (Expression* cond, Block* blk) { 
		cond_list.push_back(cond);
		if_list.push_back(blk);
	}

	void append_front (Expression* cond, Block* blk) {
		cond_list.insert(cond_list.begin(),cond);
		if_list.insert(if_list.begin(),blk);
	}

	void sem_analysis() override {
		for (int i=0; i<cond_list.size()-1; i++) {
			cond_list[i]->sem_analysis();
			if (cond_list[i]->get_var_type() != VarSTEntry(type_bool,vector<int>{})) yyerror("Type mismatch in condition", cond_list[i]->get_line_num());
			if_list[i]->sem_analysis();
		}

		if (cond_list.back()!=nullptr) {
			cond_list.back()->sem_analysis();
			if (cond_list.back()->get_var_type() != VarSTEntry(type_bool,vector<int>{})) yyerror("Type mismatch in condition", cond_list.back()->get_line_num());
		}
		if_list.back()->sem_analysis();
	}

	Value* igen() const override {
		vector<BasicBlock*> condBB_list;
		vector<BasicBlock*> bodyBB_list;
		BasicBlock* current_BB = Builder.GetInsertBlock();
		Function *TheFunction = current_BB->getParent();
		BasicBlock *PrevBB;

		// Make all basic blocks
		for (int i=0; i<cond_list.size(); i++) {
			current_BB = BasicBlock::Create(TheContext, "if_cond" + to_string(i), TheFunction, set_bb_pos(current_BB));
			condBB_list.push_back(current_BB);
		}

		for (int i=0; i<if_list.size(); i++) {
			current_BB = BasicBlock::Create(TheContext, "if_body" + to_string(i), TheFunction, set_bb_pos(current_BB));
			bodyBB_list.push_back(current_BB);
		}
		current_BB = BasicBlock::Create(TheContext, "end_if", TheFunction, set_bb_pos(current_BB));

		// Generate code for condition check
		Builder.CreateBr(condBB_list[0]);
		Builder.SetInsertPoint(condBB_list[0]);
		Value* cond;
		for (int i=0; i<cond_list.size()-1; i++) {
			cond = cond_list[i]->igen();
			Builder.CreateCondBr(cond,bodyBB_list[i],condBB_list[i+1]);
			Builder.SetInsertPoint(condBB_list[i+1]);
		}

		if (cond_list.back()!=nullptr) {
			cond = cond_list.back()->igen();
			Builder.CreateCondBr(cond,bodyBB_list.back(),current_BB);
		}
		else {
			Builder.CreateBr(bodyBB_list.back());
		}

		// Generate code for bodies
		Builder.SetInsertPoint(bodyBB_list[0]);
		for (int i=0; i<if_list.size()-1; i++) {
			if_list[i]->igen();
			PrevBB = Builder.GetInsertBlock();
			if(!PrevBB->getTerminator()) Builder.CreateBr(current_BB);
			Builder.SetInsertPoint(bodyBB_list[i+1]);
		}
		if_list.back()->igen();
		PrevBB = Builder.GetInsertBlock();
		if(!PrevBB->getTerminator()) Builder.CreateBr(current_BB);
		Builder.SetInsertPoint(current_BB);

		return nullptr;
	}

	void printAST (ostream &out) const override {
		out << "If(Cond(" << *(cond_list[0]) << ")," << *(if_list[0]);
		for (int i=1; i<cond_list.size(); i++) {
			if (cond_list[i]!=nullptr) out << ",Cond(" << *(cond_list[i]) << ")," << *(if_list[i]);
			else out << "," << *(if_list[i]);
		}
		out << ")"; 
	}

private:
	vector<Expression*> cond_list;
	vector<Block*> if_list;
};


// Class for process calls
class Proc_call : public Statement {
public:
	Proc_call (Identifier* Id, vector<Expression*>* Expr_list) :
		id(Id), expr_list(Expr_list) {}

	void sem_analysis() override {
		FuncSTEntry* func_info = fst.lookup(id->get_name(),this->get_line_num());
		char msg[1000];
		VarSTEntry expected_type;
		VarSTEntry given_type;

		if (func_info->return_type != nullptr) {
			snprintf(msg,sizeof(msg),"Function %s is not a process (return type is %s)",id->get_name().c_str(),data_name[*(func_info->return_type)]);
			yyerror(msg, this->get_line_num());
		}

		if (expr_list != nullptr) {
			if (func_info->parameters.size() != expr_list->size()) {
				snprintf(msg,sizeof(msg),"Process %s called with incorrect number of parameters: called with %d but has %d",
					id->get_name().c_str(),expr_list->size(),func_info->parameters.size());
				yyerror(msg, this->get_line_num());
			}

			int i=0;
			for (auto &expr : *expr_list) {
				expr->sem_analysis();
				expected_type = func_info->parameters[i].second;
				given_type = expr->get_var_type();

				if (expected_type != given_type) {
					snprintf(msg,sizeof(msg),"Parameter type mismatch in process %s, parameter number %d: parameter is of type %s (dims %d) but got called as type %s (dims %d)",
						id->get_name().c_str(),i+1,data_name[expected_type.basic_type],expected_type.dims.size(),data_name[given_type.basic_type],given_type.dims.size());
					yyerror(msg, this->get_line_num());
				}
				i++;
			}
		}
		else {
			if (func_info->parameters.size()!=0) {
				snprintf(msg,sizeof(msg),"Process %s called with incorrect number of parameters: called with 0 but has %d",id->get_name().c_str(),func_info->parameters.size());
				yyerror(msg, this->get_line_num());
			}
		}
		
	}

	Value* igen() const override {
		Function* calleeFunc = TheModule->getFunction(id->get_name());
		vector<Value*> parameters;
		Value* currentVal;
		Argument* arg;
		int Idx=0;

		if (expr_list!=nullptr) {
			for (auto & expr : *expr_list) {
				currentVal = expr->igen();
				
				if (expr->is_lvalue()) {
					arg = next(calleeFunc->arg_begin(), Idx);
					if (param_metadata[arg]==1 && expr->get_arg_data()!=1) {
						currentVal = Builder.CreateGEP(currentVal->getType()->getPointerElementType(),currentVal,{c32(0),c32(0)});
					}
					else if (param_metadata[arg]==2) {
						currentVal = Builder.CreateLoad(currentVal->getType()->getPointerElementType(),currentVal);
					}
				}

				parameters.push_back(currentVal);
				Idx++;
			}
		}

		return Builder.CreateCall(calleeFunc, parameters);
	}

	void printAST (ostream &out) const override {
		out << "Proc_call(" << *id; 
		if (expr_list!= nullptr) {
			for (auto & expr : *expr_list) out << "," << *expr;
		}
		out << ")";
	}

private:
	Identifier* id;
	vector<Expression*>* expr_list;

};


// Class for function calls
class Func_call : public Expression {
public:
	Func_call (Identifier* Id, vector<Expression*>* Expr_list) :
		id(Id), expr_list(Expr_list) {}

	void sem_analysis() override {
		FuncSTEntry* func_info = fst.lookup(id->get_name(),this->get_line_num());
		char msg[1000];
		VarSTEntry expected_type;
		VarSTEntry given_type;

		if (func_info->return_type == nullptr) {
			snprintf(msg,sizeof(msg),"Function %s is a process",id->get_name().c_str());
			yyerror(msg, this->get_line_num());
		}

		if (expr_list != nullptr) {
			if (func_info->parameters.size() != expr_list->size()) {
				snprintf(msg,sizeof(msg),"Process %s called with incorrect number of parameters: called with %d but has %d",
					id->get_name().c_str(),expr_list->size(),func_info->parameters.size());
				yyerror(msg, this->get_line_num());
			}

			int i=0;
			for (auto &expr : *expr_list) {
				expr->sem_analysis();
				expected_type = func_info->parameters[i].second;
				given_type = expr->get_var_type();

				if (expected_type != given_type) {
					snprintf(msg,sizeof(msg),"Parameter type mismatch in process %s, parameter number %d: parameter is of type %s (dims %d) but got called as type %s (dims %d)",
						id->get_name().c_str(),i+1,data_name[expected_type.basic_type],expected_type.dims.size(),data_name[given_type.basic_type],given_type.dims.size());
					yyerror(msg, this->get_line_num());
				} 
				i++;
			}
		}
		else {
			if (func_info->parameters.size()!=0) {
				snprintf(msg,sizeof(msg),"Process %s called with incorrect number of parameters: called with 0 but has %d",id->get_name().c_str(),func_info->parameters.size());
				yyerror(msg, this->get_line_num());
			}
		}

		var_type = VarSTEntry(*func_info->return_type,vector<int>{});
	}

	Value* igen() const override {
		Function* calleeFunc = TheModule->getFunction(id->get_name());
		vector<Value*> parameters;
		Value* currentVal;
		Argument* arg;
		int Idx=0;

		if (expr_list!=nullptr) {
			for (auto & expr : *expr_list) {
				currentVal = expr->igen();
				
				if (expr->is_lvalue()) {
					arg = next(calleeFunc->arg_begin(), Idx);
					if (param_metadata[arg]==1 && expr->get_arg_data()!=1) {
						currentVal = Builder.CreateGEP(currentVal->getType()->getPointerElementType(),currentVal,{c32(0),c32(0)});
					}
					else if (param_metadata[arg]==2) {
						currentVal = Builder.CreateLoad(currentVal->getType()->getPointerElementType(),currentVal);
					}
				}

				parameters.push_back(currentVal);
				Idx++;
			}
		}

		return Builder.CreateCall(calleeFunc, parameters,"calltmp");
	}

	void printAST (ostream &out) const override {
		out << "Func_call(" << *id; 
		if (expr_list!= nullptr) {
			for (auto & expr : *expr_list) out << "," << *expr;
		}
		out << ")";
	}

private:
	Identifier* id;
	vector<Expression*>* expr_list;
};

// Class for expressions
class Operation : public Expression {
public:
    Operation(Expression* Expr1, string operand, Expression* Expr2 = nullptr)
        : expr1(Expr1), op(operand), expr2(Expr2) {}

    void err_message(Data_Type type1, Data_Type type2, int msg_type) {
        char msg[1000];
        if (msg_type == 1) {
            snprintf(msg, sizeof(msg), "Illegal operation %s between %s and %s",
                     op.c_str(), data_name[type1], data_name[type2]);
        } else if (msg_type == 2) {
            snprintf(msg, sizeof(msg), "Illegal operation %s for %s",
                     op.c_str(), data_name[type1]);
        } else {
            snprintf(msg, sizeof(msg), "Operands of operator %s cannot be multi-dimensional arrays",
                     op.c_str());
        }
        yyerror(msg, this->get_line_num());
    }

    void sem_analysis() override {
        expr1->sem_analysis();
        if (expr2) expr2->sem_analysis();

        VarSTEntry var1 = expr1->get_var_type();
        VarSTEntry var2 = expr2 ? expr2->get_var_type() : VarSTEntry(type_bool, {});

        if (!var1.dims.empty() || !var2.dims.empty()) err_message(type_bool, type_bool, 3);

        Data_Type type1 = var1.basic_type;
        Data_Type type2 = var2.basic_type;

        if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%") {
            if (type1 == type_integer && type2 == type_integer) var_type = VarSTEntry(type_integer, {});
            else if (type1 == type_byte && type2 == type_byte) var_type = VarSTEntry(type_byte, {});
            else err_message(type1, type2, 1);
        } else if (op == "++" || op == "--") {
            if (type1 == type_integer) var_type = VarSTEntry(type_integer, {});
            else err_message(type1, type_bool, 2);
        } else if (op == "|" || op == "&") {
            if (type1 == type_byte && type2 == type_byte) var_type = VarSTEntry(type_byte, {});
            else err_message(type1, type2, 1);
        } else if (op == "!") {
            if (type1 == type_byte) var_type = VarSTEntry(type_byte, {});
            else err_message(type1, type_bool, 2);
        } else if (op == "=" || op == "<" || op == ">" || op == "<>" || op == "<=" || op == ">=") {
            if ((type1 == type_integer && type2 == type_integer) ||
                (type1 == type_byte && type2 == type_byte)) var_type = VarSTEntry(type_bool, {});
            else err_message(type1, type2, 1);
        } else if (op == "not") {
            if (type1 == type_bool) var_type = VarSTEntry(type_bool, {});
            else err_message(type1, type_bool, 2);
        } else {  // and/or
            if (type1 == type_bool && type2 == type_bool) var_type = VarSTEntry(type_bool, {});
            else err_message(type1, type2, 1);
        }
    }

    Value* igen() const override {
        Value* val1 = expr1->igen();
        Value* val2 = expr2 ? expr2->igen() : nullptr;

        if (expr1->is_lvalue()) val1 = Builder.CreateLoad(val1->getType()->getPointerElementType(), val1);
        if (expr2 && expr2->is_lvalue()) val2 = Builder.CreateLoad(val2->getType()->getPointerElementType(), val2);

        auto get_const = [&](uint64_t v, Type* type) {
            return ConstantInt::get(type, v);
        };

        Type* type1 = val1->getType();
        Type* type2 = val2 ? val2->getType() : nullptr;

        if (op == "+") return Builder.CreateAdd(val1, val2, "addtmp");
        if (op == "-") return Builder.CreateSub(val1, val2, "subtmp");
        if (op == "*") return Builder.CreateMul(val1, val2, "multmp");
        if (op == "/") {
            if (type1->isIntegerTy(32)) return Builder.CreateSDiv(val1, val2, "sdivtmp");
            else return Builder.CreateUDiv(val1, val2, "udivtmp");
        }
        if (op == "%") {
            if (type1->isIntegerTy(32)) return Builder.CreateSRem(val1, val2, "sremtmp");
            else return Builder.CreateURem(val1, val2, "uremtmp");
        }
        if (op == "++") return val1;
        if (op == "--") return Builder.CreateNeg(val1, "negtmp");
        if (op == "=") return Builder.CreateICmpEQ(val1, val2, "is_equal");
        if (op == "<>") return Builder.CreateICmpNE(val1, val2, "is_not_equal");
        if (op == ">") return type1->isIntegerTy(32) ? Builder.CreateICmpSGT(val1, val2, "is_greater")
                                                    : Builder.CreateICmpUGT(val1, val2, "is_greater");
        if (op == "<") return type1->isIntegerTy(32) ? Builder.CreateICmpSLT(val1, val2, "is_less")
                                                    : Builder.CreateICmpULT(val1, val2, "is_less");
        if (op == ">=") return type1->isIntegerTy(32) ? Builder.CreateICmpSGE(val1, val2, "is_greater_or_eq")
                                                     : Builder.CreateICmpUGE(val1, val2, "is_greater_or_eq");
        if (op == "<=") return type1->isIntegerTy(32) ? Builder.CreateICmpSLE(val1, val2, "is_less_or_eq")
                                                     : Builder.CreateICmpULE(val1, val2, "is_less_or_eq");

        if (op == "!") {
            Value* zero = get_const(0, type1);
            Value* one = get_const(1, type1);
            Value* is_zero = Builder.CreateICmpEQ(val1, zero);
            return Builder.CreateSelect(is_zero, one, zero, "boolnottmp");
        }

        if (op == "|" || op == "&") {
            // Extend bytes to i32, do op, truncate back
            if (type1->isIntegerTy(8)) {
                val1 = Builder.CreateZExt(val1, i32, "ext1");
                val2 = Builder.CreateZExt(val2, i32, "ext2");
                Value* res = (op == "|") ? Builder.CreateOr(val1, val2, "ortmp")
                                         : Builder.CreateAnd(val1, val2, "andtmp");
                return Builder.CreateTrunc(res, i8, "trunctmp");
            } else {
                return (op == "|") ? Builder.CreateOr(val1, val2, "ortmp")
                                   : Builder.CreateAnd(val1, val2, "andtmp");
            }
        }

        if (op == "and" || op == "or") {
            // short-circuit PHI
            BasicBlock* prevBB = Builder.GetInsertBlock();
            Function* F = prevBB->getParent();
            BasicBlock* leftBB = BasicBlock::Create(TheContext, "left", F);
            BasicBlock* rightBB = BasicBlock::Create(TheContext, "right", F);
            BasicBlock* mergeBB = BasicBlock::Create(TheContext, "merge", F);

            if (op == "and") {
                Value* is_zero = Builder.CreateICmpEQ(val1, get_const(0, type1));
                Builder.CreateCondBr(is_zero, mergeBB, rightBB);
            } else { // or
                Value* is_one = Builder.CreateICmpEQ(val1, get_const(1, type1));
                Builder.CreateCondBr(is_one, mergeBB, rightBB);
            }

            // Right block
            Builder.SetInsertPoint(rightBB);
            Value* val2_eval = expr2->igen();
            if (expr2->is_lvalue()) val2_eval = Builder.CreateLoad(val2_eval->getType()->getPointerElementType(), val2_eval);
            Value* rhs_result = (op == "and") ? Builder.CreateAnd(val1, val2_eval, "andtmp")
                                              : Builder.CreateOr(val1, val2_eval, "ortmp");
            Builder.CreateBr(mergeBB);

            // Merge block
            Builder.SetInsertPoint(mergeBB);
            PHINode* phi = Builder.CreatePHI(type1, 2, "phi_tmp");
            phi->addIncoming(val1, prevBB);
            phi->addIncoming(rhs_result, rightBB);
            return phi;
        }

        if (op == "not") {
            Value* zero = get_const(0, type1);
            Value* one = get_const(1, type1);
            Value* is_zero = Builder.CreateICmpEQ(val1, zero);
            return Builder.CreateSelect(is_zero, one, zero, "nottmp");
        }

        return nullptr;
    }

    void printAST(ostream& out) const override {
        out << op << "(" << *expr1;
        if (expr2) out << "," << *expr2;
        out << ")";
    }

private:
    Expression* expr1;
    Expression* expr2;
    string op;
};


#endif
