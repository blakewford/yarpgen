/*
Copyright (c) 2015-2016, Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

//////////////////////////////////////////////////////////////////////////////

#include "stmt.h"

using namespace rl;

DeclStmt::DeclStmt (std::shared_ptr<Data> _data, std::shared_ptr<Expr> _init, bool _is_extern) :
                  Stmt(Node::NodeID::DECL), data(_data), init(_init), is_extern(_is_extern) {
    if (init == NULL)
        return;
    if (data->get_class_id() != Data::VarClassID::VAR || init->get_value()->get_class_id() != Data::VarClassID::VAR) {
        std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": can init only ScalarVariable in DeclStmt::DeclStmt" << std::endl;
        exit(-1);
    }
    if (is_extern) {
        std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": init of extern var in DeclStmt::DeclStmt" << std::endl;
        exit(-1);
    }
    std::shared_ptr<ScalarVariable> data_var = std::static_pointer_cast<ScalarVariable>(data);
    std::shared_ptr<TypeCastExpr> cast_type = std::make_shared<TypeCastExpr>(init, data_var->get_type());
    data_var->set_init_value(std::static_pointer_cast<ScalarVariable>(cast_type->get_value())->get_cur_value());
}

std::string DeclStmt::emit (std::string offset) {
    std::string ret = offset;
    ret += data->get_type()->get_is_static() && !is_extern ? "static " : "";
    ret += is_extern ? "extern " : "";
    switch (data->get_type()->get_modifier()) {
        case Type::Mod::VOLAT:
            ret += "volatile ";
            break;
        case Type::Mod::CONST:
            ret += "const ";
            break;
        case Type::Mod::CONST_VOLAT:
            ret += "const volatile ";
            break;
        case Type::Mod::NTHG:
            break;
        case Type::Mod::MAX_MOD:
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": bad modifier in DeclStmt::emit" << std::endl;
            exit(-1);
            break;
    }
    ret += data->get_type()->get_simple_name() + " " + data->get_name();
    if (data->get_type()->get_align() != 0 && is_extern) // TODO: Should we set __attribute__ to non-extern variable?
        ret += " __attribute__((aligned(" + std::to_string(data->get_type()->get_align()) + ")))";
    if (init != NULL) {
        if (data->get_class_id() == Data::VarClassID::STRUCT) {
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": emit init of struct in DeclStmt::emit" << std::endl;
            exit(-1);
        }
        if (is_extern) {
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": init of extern var in DeclStmt::emit" << std::endl;
            exit(-1);
        }
        ret += " = " + init->emit();
    }
    ret += ";";
    return ret;
}

std::string ScopeStmt::emit (std::string offset) {
    std::string ret = offset + "{\n";
    for (auto i : scope)
        ret += i->emit(offset + "    ") + "\n";
    ret += offset + "}\n";
    return ret;
}

bool IfStmt::count_if_taken (std::shared_ptr<Expr> cond) {
    std::shared_ptr<TypeCastExpr> cond_to_bool = std::make_shared<TypeCastExpr> (cond, IntegerType::init(Type::IntegerTypeID::BOOL), true);
    if (cond_to_bool->get_value()->get_class_id() != Data::VarClassID::VAR) {
        std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": bad class id in IfStmt::count_if_taken" << std::endl;
        exit(-1);
    }
    std::shared_ptr<ScalarVariable> cond_var = std::static_pointer_cast<ScalarVariable> (cond_to_bool->get_value());
    return cond_var->get_cur_value().val.bool_val;
}

IfStmt::IfStmt (std::shared_ptr<Expr> _cond, std::shared_ptr<ScopeStmt> _if_br, std::shared_ptr<ScopeStmt> _else_br) :
                Stmt(Node::NodeID::IF), cond(_cond), if_branch(_if_br), else_branch(_else_br) {
    if (cond == NULL || if_branch == NULL) {
        std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": if branchescan't be empty in IfStmt::IfStmt" << std::endl;
        exit(-1);
    }
    taken = count_if_taken(cond);
}

std::string IfStmt::emit (std::string offset) {
    std::string ret = offset;
    ret += "if (" + cond->emit() + ")\n";
    ret += if_branch->emit();
    if (else_branch != NULL) {
        ret += offset + "else\n";
        ret += else_branch->emit();
    }
    return ret;
}