#ifndef AST_HPP
#define AST_HPP

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>

#include "symbol.hpp"

using namespace std;

inline const char* data_name[] = {"int","byte","bool"};
inline const char* param_name[] = {"value","reference"};


class AST {
public:
    // AST() : line_num(0) {}
    // AST(int line) : line_num(line) {}
    virtual void printAST(std::ostream &out) const = 0;
    virtual void sem_analysis() {}

    int get_line_num() const { return line_num; }
	void set_line(int line) { line_num = line; }

protected:
    int line_num;
};

inline ostream &operator << (ostream &out, const AST &ast) {
  ast.printAST(out);
  return out;
}

class Declaration : public AST {
public:
    Declaration() : AST() {}
    // Declaration(int line) : AST(line) {}
};

class Statement : public AST {
public:
    Statement() : AST() {}
    // Statement(int line) : AST(line) {}
};

class Expression : public AST {
public:
	VarSTEntry get_var_type() { return var_type; }
    Expression() : AST() {}        // calls default constructor
    // Expression(int line) : AST(line) {}
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
			//vst.insertVar(name, param_chars.basic_type, param_chars.dims);
		}
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
	Assignment (L_value* Lval, Expression* Expr) : lval(Lval), expr(Expr) {}

	void sem_analysis() override {
		lval->sem_analysis();
		expr->sem_analysis();
		if (lval->get_var_type() != expr->get_var_type()) yyerror("Type mismatch in assignmnent", this->get_line_num());
	}

	void printAST (ostream &out) const override {
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

	void printAST (ostream &out) const override {
		out << "Loop(";
		if (id!=nullptr) out << *id << ",";
		out << *block << ")";
	}

private:
	Identifier* id;
	Block* block;
};


// Class for elif list
class Elif_list : public Statement {
public:
	Elif_list () : cond_list(), elif_list() {}

	void append (Expression* cond, Block* blk) { 
		cond_list.push_back(cond);
		elif_list.push_back(blk);
	}

	void sem_analysis() override {
		for (int i=0; i<cond_list.size(); i++) {
			cond_list[i]->sem_analysis();
			if (cond_list[i]->get_var_type() != VarSTEntry(type_bool,vector<int>{})) yyerror("Type mismatch in condition", this->get_line_num());
			elif_list[i]->sem_analysis();
		}
	}

	void printAST (ostream &out) const override {
		int i=0;

		if (!cond_list.empty()) {
			for (const auto &blk : elif_list) {
				out << ",Elif(" << *(cond_list[i]) <<  "," << *blk;
				i++;
			}
		}
	}

private:
	vector<Expression*> cond_list;
	vector<Block*> elif_list;
};


// Class for if command
class If : public Statement {
public:
	If (Expression* cond, Block* ifbody, Elif_list* eliflist, Block* elsebody) : 
		condition(cond), if_body(ifbody), elif_list(eliflist), else_body(elsebody) {}

	void sem_analysis() override {
		condition->sem_analysis();
		if (condition->get_var_type() != VarSTEntry(type_bool,vector<int>{})) yyerror("Type mismatch in condition", this->get_line_num());

		if_body->sem_analysis();
		elif_list->sem_analysis();
		if (else_body!=nullptr) else_body->sem_analysis();
	}

	void printAST (ostream &out) const override {
		out << "If(Cond(" << *condition << ")," << *if_body << *elif_list;
		if (else_body!=nullptr) out << "," << *else_body;
		out << ")"; 
	}

private:
	Expression* condition;
	Block* if_body;
	Elif_list* elif_list;
	Block* else_body;
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
	Operation (Expression* Expr1, string operand, Expression* Expr2) : 
		expr1(Expr1), op(operand), expr2(Expr2) {}
	
	void err_message(Data_Type type1, Data_Type type2, int msg_type) {
		char msg[1000];

		if (msg_type == 1) {
			snprintf(msg,sizeof(msg),"Illegal operation %s between %s and %s",op.c_str(),data_name[type1],data_name[type2]);
		}
		else if (msg_type == 2) {
			snprintf(msg,sizeof(msg),"Illegal operation %s for %s",op.c_str(),data_name[type1]);
		}
		else {
			snprintf(msg,sizeof(msg),"Operands of operator %s cannot be multi-dimensional arrays",op.c_str());
		}

		yyerror(msg, this->get_line_num());
	}

	void sem_analysis() override {
		VarSTEntry var_type1, var_type2;
		Data_Type type1, type2;
		var_type2 = VarSTEntry(type_bool,{});  // To avoid undefined behavior

		expr1->sem_analysis();
		if (expr2!=nullptr) expr2->sem_analysis();

		var_type1 = expr1->get_var_type();
		if (expr2!=nullptr) var_type2 = expr2->get_var_type();

		if (!var_type1.dims.empty() || !var_type2.dims.empty()) err_message(type_bool,type_bool,3);
		type1 = var_type1.basic_type;
		type2 = var_type2.basic_type;

		if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%") {
			if (type1 == type_integer && type2 == type_integer) var_type = VarSTEntry(type_integer,{});
			else if (type1 == type_byte && type2 == type_byte) var_type = VarSTEntry(type_byte,{});
			else err_message(type1,type2,1);
		}
		else if (op == "++" || op == "--") {
			if (type1 == type_integer) var_type = VarSTEntry(type_integer,{});
			else err_message(type1,type_bool,2);
		}
		else if (op == "|" || op == "&") {
			if (type1 == type_byte && type2 == type_byte) var_type = VarSTEntry(type_byte,{});
			else err_message(type1,type2,1);
		}
		else if (op == "!") {
			if (type1 == type_byte) var_type = VarSTEntry(type_byte,{});
			else err_message(type1,type_bool,2);
		}
		else if (op == "=" || op == "<" || op == ">" || op == "<>" || op == "<=" || op == ">=") {
			if (type1 == type_integer && type2 == type_integer) var_type = VarSTEntry(type_bool,{});
			else if (type1 == type_byte && type2 == type_byte) var_type = VarSTEntry(type_bool,{});
			else err_message(type1,type2,1);
		}
		else if (op == "not") {
			if (type1 == type_bool) var_type = VarSTEntry(type_bool,{});
			else err_message(type1,type_bool,2);
		}
		else {  // Case for operations and,or
			if (type1 == type_bool && type2 == type_bool) var_type = VarSTEntry(type_bool,{});
			else err_message(type1,type2,1);
		}
	}

	void printAST (ostream &out) const override {
		if (op == "+") { out << "Add"; }
		else if (op == "-") { out << "Sub"; }
		else if (op == "*") { out << "Mult"; }
		else if (op == "/") { out << "Div"; }
		else if (op == "%") { out << "Mod"; }
		else if (op == "&") { out << "And_Bitwise"; }
		else if (op == "|") { out << "Or_Bitwise"; }
		else if (op == "!") { out << "Not_Bitwise"; }
		else if (op == "++") { out << "Plus_Sign"; }
		else if (op == "--") { out << "Minus_Sign"; }
		else if (op == "and") { out << "And"; }
		else if (op == "or") { out << "Or"; }
		else if (op == "not") { out << "Not"; }
		else if (op == "=")  { out << "Equal"; }
		else if (op == "<")  { out << "Less"; }
		else if (op == ">")  { out << "More"; }
		else if (op == "<=") { out << "Less_or_eq"; }
		else if (op == ">=") { out << "More_or_eq"; }
		else if (op == "<>") { out << "Not_eq"; }
		
		out << "(" << *expr1;
		if (expr2 != nullptr) out << "," << *expr2;
		out << ")";
	}

private:
	Expression* expr1;
	Expression* expr2;
	string op;
};

#endif
