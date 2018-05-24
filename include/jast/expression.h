#ifndef EXPRESSION_H_
#define EXPRESSION_H_

#include "jast/common.h"
#include "jast/source-locator.h"
#include <vector>
#include <string>
#include <map>

namespace jast {
#ifdef NO_EMIT_FUNCTION
#   define EMIT_FUNCTION
#else
#   define EMIT_FUNCTION
#endif

#define DEFINE_NODE_TYPE(class) \
    bool Is##class() const override { return true; }   \
    class* As##class() override { return this; }  \
    ASTNodeType type() const override { return ASTNodeType::k##class; }  \
    void Accept(ASTVisitor *visitor) override

#define AST_NODE_LIST(M)    \
    M(NullLiteral)          \
    M(UndefinedLiteral)     \
    M(ThisHolder)           \
    M(IntegralLiteral)      \
    M(StringLiteral)        \
    M(TemplateLiteral)      \
    M(ArrayLiteral)         \
    M(ObjectLiteral)        \
    M(Identifier)           \
    M(BooleanLiteral)       \
    M(RegExpLiteral)       \
    M(ArgumentList)         \
    M(CallExpression)       \
    M(MemberExpression)     \
    M(NewExpression)        \
    M(PrefixExpression)     \
    M(PostfixExpression)    \
    M(BinaryExpression)     \
    M(AssignExpression)     \
    M(TernaryExpression)    \
    M(CommaExpression)      \
    M(Declaration)          \
    M(DeclarationList)      \
    M(IfStatement)          \
    M(IfElseStatement)      \
    M(ForStatement)         \
    M(WhileStatement)       \
    M(LabelledStatement)    \
    M(BreakStatement)       \
    M(ContinueStatement)    \
    M(SwitchStatement)      \
    M(CaseClauseStatement)  \
    M(TryCatchStatement)    \
    M(ThrowStatement)       \
    M(DoWhileStatement)     \
    M(BlockStatement)       \
    M(FunctionPrototype)    \
    M(FunctionStatement)    \
    M(ReturnStatement)

class ASTVisitor;

#define FORWARD_DECLARE(Type)   \
class Type;
AST_NODE_LIST(FORWARD_DECLARE)
#undef FORWARD_DECLARE

enum class ASTNodeType {
    kUnknownType,
#define AST_NODE_TYPE(type) k##type,
AST_NODE_LIST(AST_NODE_TYPE)
#undef AST_NODE_TYPE
    kNrType
};

extern const char *type_as_string[(int)ASTNodeType::kNrType];

class Expression {
protected:
    Expression(Position &loc) :
        loc_{ loc }
    { }
public:
    virtual ~Expression() { }
    virtual void Accept(ASTVisitor *visitor) = 0;
    virtual bool ProduceRValue() { return true; }
    virtual ASTNodeType type() const = 0;

// helper conversion functions
#define AS_EXPRESSION_FUNCTION(Type)    \
    virtual Type *As##Type() { return nullptr; }
AST_NODE_LIST(AS_EXPRESSION_FUNCTION)
#undef AS_EXPRESSION_FUNCTION

    // helpful for constant folding
#define IS_EXPRESSION_FUNCTION(Type)    \
    virtual bool Is##Type() const { return false; }
AST_NODE_LIST(IS_EXPRESSION_FUNCTION)
#undef IS_EXPRESSION_FUNCTION

private:
    Position loc_;
};

using ProxyArray = std::vector<std::unique_ptr<Expression>>;
using ProxyObject = std::map<std::string, std::unique_ptr<Expression>>;

// ExpressionList ::= helper class representing a list of expressions
class ExpressionList {
public:
    using iterator = std::vector<Expression*>::iterator;

    ExpressionList()
        : exprs_{ }
    { }

    void Insert(Expression *expr)
    {
        exprs_.push_back(expr);
    }

    size_t Size()
    {
        return exprs_.size();
    }

    std::vector<Expression *> *raw_list() { return &exprs_; }

    ~ExpressionList()
    {
        for (auto &expr : exprs_) {
            delete expr;
        }
        exprs_.clear();
    }

    iterator begin() { return exprs_.begin(); }
    iterator end() { return exprs_.end(); }
private:
    std::vector<Expression *> exprs_;
};

class NullLiteral : public Expression {
public:
    NullLiteral(Position &loc)
        : Expression(loc)
    { }

    DEFINE_NODE_TYPE(NullLiteral);
};

class UndefinedLiteral : public Expression {
public:
    UndefinedLiteral(Position &loc)
        : Expression(loc)
    { }

    DEFINE_NODE_TYPE(UndefinedLiteral);
};

class ThisHolder : public Expression {
public:
    ThisHolder(Position &loc)
        : Expression(loc)
    { }

    DEFINE_NODE_TYPE(ThisHolder);
    bool ProduceRValue() override { return false; }
};

class IntegralLiteral : public Expression {
private:
    double value_;
public:
    IntegralLiteral(Position &loc, double value)
        : Expression(loc), value_(value)
    { }
    
    double value() { return value_; }
    DEFINE_NODE_TYPE(IntegralLiteral);
};

class StringLiteral : public Expression {
private:
    std::string str_;
public:
    StringLiteral(Position &loc, const std::string &str)
        : Expression(loc), str_(str)
    { }

    std::string &string() { return str_; }
    DEFINE_NODE_TYPE(StringLiteral);
};

class TemplateLiteral : public Expression {
private:
    std::string template_string_;

public:
    TemplateLiteral(Position &loc, const std::string &template_string)
        : Expression(loc), template_string_{ template_string }
    { }

    std::string &template_string() { return template_string_; }
    DEFINE_NODE_TYPE(TemplateLiteral);
};

class ArrayLiteral : public Expression {
public:
    ArrayLiteral(Position &loc, ProxyArray exprs)
        : Expression(loc), exprs_{ std::move(exprs) }
    { }

    ProxyArray &exprs() { return exprs_; }

    typename ProxyArray::size_type length() { return exprs_.size(); }

    DEFINE_NODE_TYPE(ArrayLiteral);
private:
    ProxyArray exprs_;
};

class ObjectLiteral : public Expression {
public:
    ObjectLiteral(Position &loc, ProxyObject props)
        : Expression(loc), Props{ std::move(props) }
    { }

    ProxyObject &proxy() { return Props; }
    bool IsEmpty() { return Props.empty(); }
    ProxyObject::size_type GetPropertyCount() { return Props.size(); }

    DEFINE_NODE_TYPE(ObjectLiteral);
private:
    ProxyObject Props;
};

class Identifier : public Expression {
private:
    std::string name_;
public:
    Identifier(Position &loc, const std::string &name)
        : Expression(loc), name_(name)
    { }

    const std::string &GetName() const { return name_; }
    bool ProduceRValue() override { return false; }
    DEFINE_NODE_TYPE(Identifier);
};

class BooleanLiteral : public Expression {
    bool pred_;
public:
    BooleanLiteral(Position &loc, bool val)
        : Expression(loc), pred_(val) { }   

    bool pred() { return pred_; }
    DEFINE_NODE_TYPE(BooleanLiteral);
};

class RegExpLiteral : public Expression {
public:
    RegExpLiteral(Position &loc)
        : Expression(loc)
    { }

    DEFINE_NODE_TYPE(RegExpLiteral);

    // TODO
};

class ArgumentList : public Expression {
public:
    ArgumentList(Position &loc, ExpressionList *args)
        : Expression(loc), args_{ std::move(args) }
    { }

    ExpressionList *args() { return args_.get(); }
    auto length() { return args()->Size(); }

    DEFINE_NODE_TYPE(ArgumentList);
private:
    std::unique_ptr<ExpressionList> args_;
};

enum class MemberAccessKind {
    kCall,
    kDot,
    kIndex,
    kNew    // to support expression like new function(a) { this.a = a; }(a);
};

// Example of member expression AST (a(g).b.e[3](f))
//                      kCall
//                      /    \          |
//                   kIndex    (f)
//                  /      \            |
//                kDot      [3]
//               /   \                  |
//            kCall   e
//           /     \                    |
//          a       (g)
class CallExpression : public Expression {
public:
    CallExpression(Position &loc, MemberAccessKind kind,
        Expression *expr, Expression *member)
        : Expression(loc), kind_{ kind }, expr_(expr), member_(member)
    { }
    DEFINE_NODE_TYPE(CallExpression);

    Expression *member() { return member_.get(); }
    MemberAccessKind kind() { return kind_; }

    Expression *expr() { return expr_.get(); }
    bool ProduceRValue() override { return false; }
private:
    MemberAccessKind kind_;
    std::unique_ptr<Expression> expr_;
    std::unique_ptr<Expression> member_;
};

// class DotMemberExpression : public Expression {
// public:
//     DotMemberExpression(Position &loc, Expression *mem)
//         : Expression(loc), mem_(mem)
//     { }
//     DEFINE_NODE_TYPE(DotMemberExpression);

//     Expression *member() { return mem_.get(); }
//     bool ProduceRValue() override { return false; }
// private:
//     std::unique_ptr<Expression> mem_;
// };

// class IndexMemberExpression : public Expression {
// public:
//     IndexMemberExpression(Position &loc, Expression *expr)
//         : Expression(loc), expr_{ expr }
//     { }

//     DEFINE_NODE_TYPE(IndexMemberExpression);
//     Expression *expr() { return expr_.get(); }
//     bool ProduceRValue() override { return false; }
// private:
//     std::unique_ptr<Expression> expr_;
// };

class MemberExpression : public Expression {
public:
    MemberExpression(Position &loc, MemberAccessKind kind,
        Expression *expr, Expression *member)
        : Expression(loc), kind_{ kind }, expr_(expr), member_(member)
    { }
    DEFINE_NODE_TYPE(MemberExpression);

    Expression *member() { return member_.get(); }
    MemberAccessKind kind() { return kind_; }

    Expression *expr() { return expr_.get(); }
    bool ProduceRValue() override { return false; }
private:
    MemberAccessKind kind_;
    std::unique_ptr<Expression> expr_;
    std::unique_ptr<Expression> member_;
};

class NewExpression : public Expression {
public:
    NewExpression(Position &loc, Expression *member)
        : Expression(loc), member_{ member }
    { }

    DEFINE_NODE_TYPE(NewExpression);
    Expression *member() { return member_.get(); }
    bool ProduceRValue() override { return false; }
private:
    std::unique_ptr<Expression> member_; 
};

enum class PrefixOperation {
    kIncrement,
    kDecrement,
    kTypeOf,
    kDelete,
    kBitNot,
    kNot,
    kVoid
};

class PrefixExpression : public Expression {
public:

    PrefixExpression(Position &loc, PrefixOperation op, Expression *expr)
        : Expression(loc), op_{ op }, expr_{ expr }
    { }

    DEFINE_NODE_TYPE(PrefixExpression);

    PrefixOperation op() { return op_; }
    Expression *expr() { return expr_.get(); }
private:
    PrefixOperation op_;
    std::unique_ptr<Expression> expr_;
};

enum class PostfixOperation {
    kIncrement,
    kDecrement
};

class PostfixExpression : public Expression {
public:
    PostfixExpression(Position &loc, PostfixOperation op, Expression *expr)
        : Expression(loc), op_{ op }, expr_{ expr }
    { }
    
    DEFINE_NODE_TYPE(PostfixExpression);

    PostfixOperation op() { return op_; }
    Expression *expr() { return expr_.get(); }
private:
    PostfixOperation op_;
    std::unique_ptr<Expression> expr_;
};

enum class BinaryOperation {
    kAddition,
    kSubtraction,
    kMultiplication,
    kDivision,
    kMod,
    kShiftRight,
    kShiftLeft,
    kShiftZeroRight,
    kLessThan,
    kGreaterThan,
    kLessThanEqual,
    kGreaterThanEqual,
    kEqual,
    kNotEqual,
    kStrictEqual,
    kStrictNotEqual,
    kAnd,
    kOr,
    kBitAnd,
    kBitOr,
    kBitXor,
    kInstanceOf,
    kIn
};

class BinaryExpression : public Expression {
public:
    DEFINE_NODE_TYPE(BinaryExpression);

    BinaryOperation op() { return op_; }
    Expression *lhs() { return lhs_.get(); }
    Expression *rhs() { return rhs_.get(); }
private:
    BinaryOperation op_;
    std::unique_ptr<Expression> lhs_;
    std::unique_ptr<Expression> rhs_;
public:
    BinaryExpression(Position &loc, BinaryOperation op,
        Expression *lhs, Expression *rhs)
        : Expression(loc), op_(op), lhs_(lhs), rhs_(rhs) { }
};

class AssignExpression : public Expression {
public:
    AssignExpression(Position &loc, Expression *lhs, Expression *rhs)
        : Expression(loc), lhs_(lhs), rhs_(rhs) { }
    DEFINE_NODE_TYPE(AssignExpression);

    Expression *lhs() { return lhs_.get(); }
    Expression *rhs() { return rhs_.get(); }
private:
    std::unique_ptr<Expression> lhs_;
    std::unique_ptr<Expression> rhs_;
};

class TernaryExpression : public Expression {
public:
    TernaryExpression(Position &loc, Expression *first,
                      Expression *second, Expression *third)
    : Expression(loc), first_(first), second_(second),
        third_(third)
    { }
    DEFINE_NODE_TYPE(TernaryExpression);

    Expression *first() { return first_.get(); }
    Expression *second() { return second_.get(); }
    Expression *third() { return third_.get(); }
private:
    std::unique_ptr<Expression> first_;
    std::unique_ptr<Expression> second_;
    std::unique_ptr<Expression> third_;
};

class CommaExpression : public Expression {
public:
    CommaExpression(Position &loc, ExpressionList *exprs)
        : Expression(loc), exprs_{ exprs }
    { }
    DEFINE_NODE_TYPE(CommaExpression);

    ExpressionList *exprs() { return exprs_.get(); }
private:
    std::unique_ptr<ExpressionList> exprs_;
};

class Declaration : public Expression {
public:
    Declaration(Position &loc, std::string name, Expression *init)
        : Expression(loc), name_{ name }, init_{ init }
    { }

    DEFINE_NODE_TYPE(Declaration);

    std::string &name() { return name_; }

    Expression *expr() { return init_.get(); }
private:
    std::string name_;
    std::unique_ptr<Expression> init_;
};

class DeclarationList : public Expression {
public:
    DeclarationList(Position &loc, 
        std::vector<std::unique_ptr<Declaration>> exprs)
        : Expression(loc), exprs_{ std::move(exprs) }
    { }
    DEFINE_NODE_TYPE(DeclarationList);

    std::vector<std::unique_ptr<Declaration>> &exprs() { return exprs_; }

    void Append(Declaration *decl)
    {
        exprs_.push_back(std::unique_ptr<Declaration>(decl));
    }
private:
    std::vector<std::unique_ptr<Declaration>> exprs_;
};

} // namespace jast

#endif  /* Expression.h */
