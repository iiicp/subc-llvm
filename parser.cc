#include "parser.h"

std::shared_ptr<Program> Parser::ParseProgram() {

    auto program = std::make_shared<Program>();
    program->fileName = lexer.GetFileName();
    while (tok.tokenType != TokenType::eof) {
        std::shared_ptr<AstNode> node;
        if (IsFuncDecl()) {
            node = ParseFuncDecl();
        }else {
            node = ParseDeclStmt(true);
        }
        if (node) {
            program->externalDecls.push_back(node);
        }
    }
    Expect(TokenType::eof);
    return program;
}

std::shared_ptr<AstNode> Parser::ParseFuncDecl() {
    auto baseType = ParseDeclSpec();

    sema.EnterScope();
    auto node = Declarator(baseType, true);

    std::shared_ptr<AstNode> blockStmt = nullptr;
    if (tok.tokenType != TokenType::semi) {
        blockStmt = ParseBlockStmt();
    }else {
        Consume(TokenType::semi);
    }
    sema.ExitScope();

    return sema.SemaFuncDecl(node->tok, node->ty, blockStmt);
}

std::shared_ptr<AstNode> Parser::ParseStmt() {
    /// null stmt
    if (tok.tokenType == TokenType::semi) {
        Advance();
        return nullptr;
    }
    /// decl stmt
    if (IsTypeName(tok.tokenType)) {
        return ParseDeclStmt();
    }
    /// if stmt
    else if(tok.tokenType == TokenType::kw_if) {
        return ParseIfStmt();
    }
    /// block stmt
    else if(tok.tokenType == TokenType::l_brace) {
        return ParseBlockStmt();
    }
    /// for stmt
    else if (tok.tokenType == TokenType::kw_for) {
        return ParseForStmt();
    }
    /// break stmt
    else if (tok.tokenType == TokenType::kw_break) {
        return ParseBreakStmt();
    }
    /// continue stmt
    else if (tok.tokenType == TokenType::kw_continue) {
        return ParseContinueStmt();
    }
    else if (tok.tokenType == TokenType::kw_return) {
        return ParseReturnStmt();
    }
    /// expr stmt
    else {
        return ParseExprStmt();
    }
}

std::shared_ptr<AstNode> Parser::ParseBlockStmt() {
    sema.EnterScope();
    auto blockStmt = std::make_shared<BlockStmt>();

    Consume(TokenType::l_brace);
    while (tok.tokenType != TokenType::r_brace) {
        auto stmt = ParseStmt();
        if (stmt) {
            blockStmt->nodeVec.push_back(stmt);
        }
    }
    Consume(TokenType::r_brace);

    sema.ExitScope();
    
    return blockStmt;
}

std::shared_ptr<CType> Parser::ParseDeclSpec() {
    if (tok.tokenType == TokenType::kw_int) {
        Consume(TokenType::kw_int);
        return CType::IntType;
    }else if (tok.tokenType == TokenType::kw_struct || tok.tokenType == TokenType::kw_union) {
        return ParseStructOrUnionSpec();
    }else if (tok.tokenType == TokenType::kw_void) {
        Consume(TokenType::kw_void);
        return CType::VoidType;
    }
    GetDiagEngine().Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_type);
    return nullptr;
}

std::shared_ptr<CType> Parser::ParseStructOrUnionSpec() {
    TagKind tagKind;
    if (tok.tokenType == TokenType::kw_struct) {
        tagKind = TagKind::kStruct;
    }else if (tok.tokenType == TokenType::kw_union) {
        tagKind = TagKind::kUnion;
    }else {
        assert(0);
    }
    Advance();

    bool anony = false;
    if (tok.tokenType != TokenType::identifier) {
        anony = true;
    }

    Token tag = tok;
    if (tok.tokenType == TokenType::identifier) {
        tag = tok;
        Consume(TokenType::identifier);
    }

    std::shared_ptr<CType> recordTy = nullptr;
    if (!anony) {
        recordTy = sema.SemaTagAccess(tag);
    }

    if (!recordTy) {
        llvm::StringRef name;
        if (anony) {
            name = CType::GenAnonyRecordName(tagKind);
        }else {
            name = llvm::StringRef(tag.ptr, tag.len);
        }
        recordTy = std::make_shared<CRecordType>(name, std::vector<Member>(), tagKind);
    }

    /*
        struct A{int a,b; int *p;}
        struct A;
    */
    if (tok.tokenType == TokenType::l_brace) {
        Consume(TokenType::l_brace);
        sema.EnterScope();
        std::vector<Member> members;
        while (tok.tokenType != TokenType::r_brace) {
            auto node = ParseDeclStmt();
            DeclStmt *decl = llvm::dyn_cast<DeclStmt>(node.get());
            for (const auto &n : decl->nodeVec) {
                Member m;
                m.ty = n->ty;
                m.name = llvm::StringRef(n->tok.ptr, n->tok.len);
                members.push_back(m);
            }
        }
        sema.ExitScope();
        Consume(TokenType::r_brace);

        CRecordType *ty = llvm::dyn_cast<CRecordType>(recordTy.get());
        ty->SetMembers(members);

        return sema.SemaTagDecl(tag, recordTy);
    }else {
       return recordTy;  
    }
}
/**
 declarator ::= "*"* direct-declarator
 direct-declarator ::= identifier | "(" declarator ")" | direct-declarator "[" assign "]"
 */

/// int a[3][4];
/// baseType -> int
std::shared_ptr<CType> Parser::DirectDeclaratorArraySuffix(std::shared_ptr<CType> baseType, bool isGlobal) {
    if (tok.tokenType != TokenType::l_bracket) {
        return baseType;
    }
    Consume(TokenType::l_bracket);
    int count = -1;
    if (tok.tokenType != TokenType::r_bracket) {
        Expect(TokenType::number);
        count = tok.value;
        Consume(TokenType::number);
    }
    Consume(TokenType::r_bracket);
    return std::make_shared<CArrayType>(DirectDeclaratorArraySuffix(baseType, isGlobal), count);
}

std::shared_ptr<CType> Parser::DirectDeclaratorFuncSuffix(Token iden, std::shared_ptr<CType> baseType, bool isGlobal) {
    Consume(TokenType::l_parent);

    std::vector<Param> params;
    int i = 0;
    while (tok.tokenType != TokenType::r_parent) {
        if (i > 0 && (tok.tokenType == TokenType::comma)) {
            Consume(TokenType::comma);
        }

        auto ty = ParseDeclSpec();
        auto node = Declarator(ty, isGlobal);

        Param p;
        if (node->ty->GetKind() == CType::TY_Array) {
            p.type = std::make_shared<CPointType>(node->ty);
        }else {
            p.type = node->ty;
        }
        p.name = llvm::StringRef(node->tok.ptr, node->tok.len);

        params.push_back(p);
        ++i;
    }

    Consume(TokenType::r_parent);

    return std::make_shared<CFuncType>(baseType, params, llvm::StringRef(iden.ptr, iden.len));
}

std::shared_ptr<CType> Parser::DirectDeclaratorSuffix(Token iden, std::shared_ptr<CType> baseType, bool isGlobal) {
    if (tok.tokenType == TokenType::l_bracket) {
        return DirectDeclaratorArraySuffix(baseType, isGlobal);
    }else if (tok.tokenType == TokenType::l_parent) {
        return DirectDeclaratorFuncSuffix(iden, baseType, isGlobal);
    }
    /// func
    return baseType;
}

std::shared_ptr<AstNode> Parser::DirectDeclarator(std::shared_ptr<CType> baseType, bool isGlobal) {
    std::shared_ptr<AstNode> declNode;
    if (tok.tokenType == TokenType::l_parent) {
        Token beginTok = tok;
        lexer.SaveState();
            sema.SetMode(Sema::Mode::Skip);
            Consume(TokenType::l_parent);
            Declarator(CType::IntType, isGlobal);
            Consume(TokenType::r_parent);
            
            baseType = DirectDeclaratorSuffix(tok, baseType, isGlobal); 

        lexer.RestoreState();
        sema.SetMode(Sema::Mode::Normal);
        tok = beginTok;

        Consume(TokenType::l_parent);
        declNode = Declarator(baseType, isGlobal);
        Consume(TokenType::r_parent);
        DirectDeclaratorSuffix(tok, CType::IntType, isGlobal);
    }else if (tok.tokenType == TokenType::identifier){
        Token iden = tok;
        Consume(TokenType::identifier);
        baseType = DirectDeclaratorSuffix(iden, baseType, isGlobal);
        declNode = sema.SemaVariableDeclNode(iden, baseType, isGlobal);
    }else {
        GetDiagEngine().Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_expected_ex, "identifier or '('");
    }
   
    if (tok.tokenType == TokenType::equal) {
        Advance();
        VariableDecl*varDecl = llvm::dyn_cast<VariableDecl>(declNode.get());
        // varDecl->init = ParseAssignExpr();
        std::vector<int> offsetList{0}; /// 0表示访问首元素
        ParseInitializer(varDecl->initValues, declNode->ty, offsetList, false);
    }
    return declNode;
}

bool Parser::ParseInitializer(std::vector<std::shared_ptr<VariableDecl::InitValue>> &arr, std::shared_ptr<CType> declType, std::vector<int> &offsetList, bool hasLBrace) {
    /// {}
    if (tok.tokenType == TokenType::r_brace) {
        if (!hasLBrace) {
            /// 会报错
            GetDiagEngine().Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_miss, "miss '{'");
        }
        return true;
    }
    if (tok.tokenType == TokenType::l_brace) {
        Consume(TokenType::l_brace);

        if (declType->GetKind() == CType::TY_Array) {
            CArrayType *arrType = llvm::dyn_cast<CArrayType>(declType.get());
            int size = arrType->GetElementCount();
            bool isFlex = size < 0 ? true : false;
            /// int a[10] = {1,2,3};
            int i = 0;
            for (; i < size || isFlex; ++i) {
                if (i > 0 && (tok.tokenType == TokenType::comma)) {
                    Consume(TokenType::comma);
                }
                offsetList.push_back(i);
                bool end = ParseInitializer(arr, arrType->GetElementType(), offsetList, true);
                offsetList.pop_back();
                if (end) {
                    break;
                }
            }
            if (isFlex) {
                arrType->SetElementCount(i);
            }
        }else if (declType->GetKind() == CType::TY_Record) {
            CRecordType *recordType = llvm::dyn_cast<CRecordType>(declType.get());
            TagKind tagKind = recordType->GetTagKind();
            const auto &members = recordType->GetMembers();

            if (tagKind == TagKind::kStruct) {    
                for (int i = 0; i < members.size(); ++i) {
                    if (i > 0 && (tok.tokenType == TokenType::comma)) {
                        Consume(TokenType::comma);
                    }
                    offsetList.push_back(i);
                    bool end = ParseInitializer(arr, members[i].ty, offsetList, true);
                    offsetList.pop_back();
                    if (end) {
                        break;
                    }
                }
            }else {
                /// union 赋值初值
                if (members.size() > 0) {
                    offsetList.push_back(0);
                    ParseInitializer(arr, members[0].ty, offsetList, true);
                    offsetList.pop_back();
                }
            }
        }
        Consume(TokenType::r_brace);
    }else {
        Token tmp = tok;
        // assign
        auto node = ParseAssignExpr();

        auto initValue = sema.SemaDeclInitValue(declType, node, offsetList, tmp);
        arr.push_back(initValue);
    }
    return false;
}

std::shared_ptr<AstNode> Parser::Declarator(std::shared_ptr<CType> baseType, bool isGlobal) {
    while (tok.tokenType == TokenType::star) {
        Consume(TokenType::star);
        baseType = std::make_shared<CPointType>(baseType);
    }
    return DirectDeclarator(baseType, isGlobal);
}

std::shared_ptr<AstNode> Parser::ParseDeclStmt(bool isGlobal) {
    
    auto baseTy = ParseDeclSpec();

    /// int ;
    /// 无意义的声明
    if (tok.tokenType == TokenType::semi) {
        Consume(TokenType::semi);
        return nullptr;
    }

    auto decl = std::make_shared<DeclStmt>();
    /// int a,b=3;
    /// a,b=3; 
    
    int i = 0;
    while (tok.tokenType != TokenType::semi) {
        if (i++ > 0) {
            assert(Consume(TokenType::comma));
        }
        decl->nodeVec.push_back(Declarator(baseTy, isGlobal));
    }

    Consume(TokenType::semi);

    return decl;
}

std::shared_ptr<AstNode> Parser::ParseExprStmt() {
    auto expr = ParseExpr();
    Consume(TokenType::semi);
    return expr;
}

// if-stmt : "if" "(" expr ")" stmt ( "else" stmt )?
std::shared_ptr<AstNode> Parser::ParseIfStmt() {
    Consume(TokenType::kw_if);
    Consume(TokenType::l_parent);
    auto condExpr = ParseExpr();
    Consume(TokenType::r_parent);
    auto thenStmt = ParseStmt();
    std::shared_ptr<AstNode> elseStmt = nullptr;
    /// peek tok is 'else'
    if (tok.tokenType == TokenType::kw_else) {
        Consume(TokenType::kw_else);
        elseStmt = ParseStmt();
    }
    return sema.SemaIfStmtNode(condExpr, thenStmt, elseStmt);
}

std::shared_ptr<AstNode> Parser::ParseForStmt() {
    Consume(TokenType::kw_for);
    Consume(TokenType::l_parent);

    sema.EnterScope();
    auto node = std::make_shared<ForStmt>();
    
    breakNodes.push_back(node);
    continueNodes.push_back(node);

    std::shared_ptr<AstNode> initNode = nullptr;
    std::shared_ptr<AstNode> condNode = nullptr;
    std::shared_ptr<AstNode> incNode = nullptr;
    std::shared_ptr<AstNode> bodyNode = nullptr;

    if (IsTypeName(tok.tokenType)) {
        initNode = ParseDeclStmt();
    }else {
        if (tok.tokenType != TokenType::semi) {
            initNode = ParseExpr();
        }
        Consume(TokenType::semi);
    }

    if (tok.tokenType != TokenType::semi) {
        condNode = ParseExpr();
    }
    Consume(TokenType::semi);

    if (tok.tokenType != TokenType::r_parent) {
        incNode = ParseExpr();
    }
    Consume(TokenType::r_parent);

    bodyNode = ParseStmt();

    node->initNode = initNode;
    node->condNode = condNode;
    node->incNode = incNode;
    node->bodyNode = bodyNode;

    breakNodes.pop_back();
    continueNodes.pop_back();

    sema.ExitScope();
    return node;
}

std::shared_ptr<AstNode> Parser::ParseBreakStmt() {
    if (breakNodes.size() == 0) {
        GetDiagEngine().Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_break_stmt);
    }
    Consume(TokenType::kw_break);
    auto node = std::make_shared<BreakStmt>();
    node->target = breakNodes.back(); 
    Consume(TokenType::semi);
    return node;
}

std::shared_ptr<AstNode> Parser::ParseContinueStmt() {
    if (breakNodes.size() == 0) {
        GetDiagEngine().Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_continue_stmt);
    }
    Consume(TokenType::kw_continue);
    auto node = std::make_shared<ContinueStmt>();
    node->target = continueNodes.back();
    Consume(TokenType::semi);
    return node;
}

std::shared_ptr<AstNode> Parser::ParseReturnStmt() {
    Consume(TokenType::kw_return);
    auto node = std::make_shared<ReturnStmt>();
    if (tok.tokenType != TokenType::semi) {
        node->expr = ParseExpr();
    }
    Consume(TokenType::semi);
    return node;
}

/// 左结合
/// expr : assign-expr | add-expr
/// assign-expr: identifier "=" expr
/// add-expr : mult-expr (("+" | "-") mult-expr)* 

/// LLn  
std::shared_ptr<AstNode> Parser::ParseExpr() {
    auto left = ParseAssignExpr();
    while (tok.tokenType == TokenType::comma) {
        Consume(TokenType::comma);
        auto right = ParseAssignExpr();
        left = sema.SemaBinaryExprNode(left, right, BinaryOp::comma);
    }
    return left;
}

std::shared_ptr<AstNode> Parser::ParseEqualExpr() {
    auto left = ParseRelationalExpr();
    while (tok.tokenType == TokenType::equal_equal || tok.tokenType == TokenType::not_equal) {
        BinaryOp op;
        if (tok.tokenType == TokenType::equal_equal) {
            op = BinaryOp::equal;
        }else {
            op = BinaryOp::not_equal;
        }
        Advance();
        left = sema.SemaBinaryExprNode(left, ParseRelationalExpr(), op);
    }
    return left;
}

std::shared_ptr<AstNode> Parser::ParseRelationalExpr() {
    auto left = ParseShiftExpr();
    while (tok.tokenType == TokenType::less || tok.tokenType == TokenType::less_equal || 
        tok.tokenType == TokenType::greater || tok.tokenType == TokenType::greater_equal) {
        BinaryOp op;
        if (tok.tokenType == TokenType::less) {
            op = BinaryOp::less;
        }else if (tok.tokenType == TokenType::less_equal){
            op = BinaryOp::less_equal;
        }else if (tok.tokenType == TokenType::greater){
            op = BinaryOp::greater;
        }else if (tok.tokenType == TokenType::greater_equal){
            op = BinaryOp::greater_equal;
        }
        Advance();
        left = sema.SemaBinaryExprNode(left, ParseShiftExpr(), op);
    }
    return left;
}

std::shared_ptr<AstNode> Parser::ParseAddExpr() {
     /// add-expr : mult-expr (("+" | "-") mult-expr)* 
    auto left = ParseMultiExpr();
    while (tok.tokenType == TokenType::plus || tok.tokenType == TokenType::minus) {
        BinaryOp op;
        if (tok.tokenType == TokenType::plus) {
            op = BinaryOp::add;
        }else {
            op = BinaryOp::sub;
        }
        Advance();
        left = sema.SemaBinaryExprNode(left, ParseMultiExpr(), op);
    }
    return left;
}

/// 左结合
std::shared_ptr<AstNode> Parser::ParseMultiExpr() {
    auto left = ParseUnaryExpr();
    while (tok.tokenType == TokenType::star || 
            tok.tokenType == TokenType::slash || 
            tok.tokenType == TokenType::percent) {
        BinaryOp op;
        if (tok.tokenType == TokenType::star) {
            op = BinaryOp::mul;
        }else if (tok.tokenType == TokenType::slash) {
            op = BinaryOp::div;
        }
        else {
            op = BinaryOp::mod;
        }
        Advance();
        left = sema.SemaBinaryExprNode(left, ParseUnaryExpr(), op);
    }
    return left;
}

std::shared_ptr<AstNode> Parser::ParseUnaryExpr() {
    if (!IsUnaryOperator()) {
        return ParsePostFixExpr();
    }
    
    /// check sizeof
    if (tok.tokenType == TokenType::kw_sizeof) {
        bool isTypeName = false;
        Consume(TokenType::kw_sizeof);

        if (tok.tokenType == TokenType::l_parent) {
            lexer.SaveState();
            Token nextTok;
            lexer.NextToken(nextTok);
            if (IsTypeName(nextTok.tokenType)) {
                isTypeName = true;
            }
            lexer.RestoreState();
        }

        if (isTypeName) {
            
            Consume(TokenType::l_parent);
            auto type = ParseType();
            Consume(TokenType::r_parent);
            return sema.SemaSizeofExprNode(nullptr, type);
        }else {
            auto node = ParseUnaryExpr();
            return sema.SemaSizeofExprNode(node, nullptr);
        }
    }

    UnaryOp op;
    switch (tok.tokenType)
    {
    case TokenType::plus:
        op = UnaryOp::positive;
        break;
    case TokenType::minus:
        op = UnaryOp::negative;
        break;
    case TokenType::star:
        op = UnaryOp::deref;
        break;
    case TokenType::amp:
        op = UnaryOp::addr;
        break;
    case TokenType::plus_plus:
        op = UnaryOp::inc;
        break;
    case TokenType::minus_minus:
        op = UnaryOp::dec;
        break;
    case TokenType::exclaim:
        op = UnaryOp::logical_not;
        break;
    case TokenType::tilde:
        op = UnaryOp::bitwise_not;
        break;                                                          
    default:
        break;
    }
    Advance();
    Token tmp = tok;
    return sema.SemaUnaryExprNode(ParseUnaryExpr(), op, tmp);
}


std::shared_ptr<AstNode> Parser::ParsePostFixExpr() {
    auto left = ParsePrimary();
    for (;;) {
        if (tok.tokenType == TokenType::plus_plus) {
            left = sema.SemaPostIncExprNode(left, tok);
            Consume(TokenType::plus_plus);
            continue;
        }
        if (tok.tokenType == TokenType::minus_minus) {
            left = sema.SemaPostDecExprNode(left, tok);
            Consume(TokenType::minus_minus);
            continue;
        }
        /// a[3][5]
        if (tok.tokenType == TokenType::l_bracket) {
            Token tmp = tok;
            Consume(TokenType::l_bracket);
            auto node = ParseExpr();
            Consume(TokenType::r_bracket);
            left = sema.SemaPostSubscriptNode(left, node, tmp);
            continue;
        }

        if (tok.tokenType == TokenType::dot) {
            Token dotTok = tok;
            Consume(TokenType::dot);
            Token tmp = tok;
            Consume(TokenType::identifier);
            left = sema.SemaPostMemberDotNode(left, tmp, dotTok);
            continue;
        }

        if (tok.tokenType == TokenType::arrow) {
            Token arrowTok = tok;
            Consume(TokenType::arrow);
            Token tmp = tok;
            Consume(TokenType::identifier);
            left = sema.SemaPostMemberArrowNode(left, tmp, arrowTok);
            continue;
        }

        if (tok.tokenType == TokenType::l_parent) {
            Consume(TokenType::l_parent);

            std::vector<std::shared_ptr<AstNode>> args;
            int i = 0;
            while (tok.tokenType != TokenType::r_parent) {
                if (i > 0 && (tok.tokenType == TokenType::comma)) {
                    Consume(TokenType::comma);
                }
                args.push_back(ParseAssignExpr());
                ++i;
            }

            Consume(TokenType::r_parent);
            left = sema.SemaFuncCall(left, args);
            continue;
        }

        break;
    }
    return left;
}

std::shared_ptr<AstNode> Parser::ParsePrimary() {
    if (tok.tokenType == TokenType::l_parent) {
        Advance();
        auto expr = ParseExpr();
        assert (Expect(TokenType::r_parent));
        Advance();
        return expr;
    } else if (tok.tokenType == TokenType::identifier) {
        auto expr = sema.SemaVariableAccessNode(tok);
        Advance();
        return expr;
    }
    else {
        Expect(TokenType::number);
        auto factor = sema.SemaNumberExprNode(tok, tok.ty);
        Advance();
        return factor;
    }
}

/// a = b = 3;
std::shared_ptr<AstNode> Parser::ParseAssignExpr() {
    auto left = ParseConditionalExpr();

    if (!IsAssignOperator()) {
        return left;
    }

    BinaryOp op;
    switch (tok.tokenType)
    {
    case TokenType::equal:
        op = BinaryOp::assign;
        break;
    case TokenType::plus_equal:
        op = BinaryOp::add_assign;
        break;
    case TokenType::minus_equal:
        op = BinaryOp::sub_assign;
        break;
    case TokenType::star_equal:
        op = BinaryOp::mul_assign;
        break;
    case TokenType::slash_equal:
        op = BinaryOp::div_assign;
        break;      
    case TokenType::percent_equal:
        op = BinaryOp::mod_assign;
        break;
    case TokenType::pipe_equal:
        op = BinaryOp::bitwise_or_assign;
        break;
    case TokenType::amp_equal:
        op = BinaryOp::bitwise_and_assign;
        break;
    case TokenType::caret_equal:
        op = BinaryOp::bitwise_xor_assign;
        break;
    case TokenType::less_less_equal:
        op = BinaryOp::left_shift_assign;
        break;  
    case TokenType::greater_greater_equal:
        op = BinaryOp::right_shift_assign;
        break;                                        
    default:
        break;
    }
    Advance();
    return sema.SemaBinaryExprNode(left, ParseAssignExpr(), op);
}


std::shared_ptr<AstNode> Parser::ParseConditionalExpr() {
    auto left = ParseLogOrExpr();
    if (tok.tokenType != TokenType::question) {
        return left;
    }
    Token tmp = tok;
    Consume(TokenType::question);
    auto then = ParseExpr();
    Consume(TokenType::colon);
    auto els = ParseConditionalExpr();
    return sema.SemaThreeExprNode(left, then, els, tmp);
}

std::shared_ptr<AstNode> Parser::ParseLogOrExpr() {
    auto left = ParseLogAndExpr();
    while (tok.tokenType == TokenType::pipepipe) {
        BinaryOp op = BinaryOp::logical_or;
        Advance();
        left = sema.SemaBinaryExprNode(left, ParseLogAndExpr(), op);
    }
    return left;
}

std::shared_ptr<AstNode> Parser::ParseLogAndExpr() {
    auto left = ParseBitOrExpr();
    while (tok.tokenType == TokenType::ampamp) {
        BinaryOp op = BinaryOp::logical_and;
        Advance();
        left = sema.SemaBinaryExprNode(left, ParseBitOrExpr(), op);
    }
    return left;
}
std::shared_ptr<AstNode> Parser::ParseBitOrExpr() {
    auto left = ParseBitXorExpr();
    while (tok.tokenType == TokenType::pipe) {
        BinaryOp op = BinaryOp::bitwise_or;
        Advance();
        left = sema.SemaBinaryExprNode(left, ParseBitXorExpr(), op);
    }
    return left;
}
std::shared_ptr<AstNode> Parser::ParseBitXorExpr() {
    auto left = ParseBitAndExpr();
    while (tok.tokenType == TokenType::caret) {
        BinaryOp op = BinaryOp::bitwise_xor;
        Advance();
        left = sema.SemaBinaryExprNode(left, ParseBitAndExpr(), op);
    }
    return left;
}
std::shared_ptr<AstNode> Parser::ParseBitAndExpr() {
    auto left = ParseEqualExpr();
    while (tok.tokenType == TokenType::amp) {
        BinaryOp op = BinaryOp::bitwise_and;
        Advance();
        left = sema.SemaBinaryExprNode(left, ParseEqualExpr(), op);
    }
    return left;
}
std::shared_ptr<AstNode> Parser::ParseShiftExpr() {
    auto left = ParseAddExpr();
    while (tok.tokenType == TokenType::less_less || tok.tokenType == TokenType::greater_greater) {
        BinaryOp op;
        if (tok.tokenType == TokenType::less_less) {
            op = BinaryOp::left_shift;
        }else {
            op = BinaryOp::right_shift;
        }
        Advance();
        left = sema.SemaBinaryExprNode(left, ParseAddExpr(), op);
    }
    return left;
}

bool Parser::IsAssignOperator() {
    return tok.tokenType == TokenType::equal
    || tok.tokenType == TokenType::plus_equal
    || tok.tokenType == TokenType::minus_equal
    || tok.tokenType == TokenType::star_equal
    || tok.tokenType == TokenType::slash_equal
    || tok.tokenType == TokenType::percent_equal
    || tok.tokenType == TokenType::pipe_equal
    || tok.tokenType == TokenType::amp_equal
    || tok.tokenType == TokenType::caret_equal
    || tok.tokenType == TokenType::less_less_equal
    || tok.tokenType == TokenType::greater_greater_equal;
}


bool Parser::IsUnaryOperator() {
    return tok.tokenType == TokenType::plus_plus
    || tok.tokenType == TokenType::minus_minus
    || tok.tokenType == TokenType::plus
    || tok.tokenType == TokenType::minus
    || tok.tokenType == TokenType::star
    || tok.tokenType == TokenType::amp
    || tok.tokenType == TokenType::exclaim
    || tok.tokenType == TokenType::tilde
    || tok.tokenType == TokenType::kw_sizeof;
}

/// @brief  sizeof (int* [5][6]);
std::shared_ptr<CType> Parser::ParseType() {
    std::shared_ptr<CType> baseType = ParseDeclSpec();
    assert(baseType);

    while (tok.tokenType == TokenType::star) {
        baseType = std::make_shared<CPointType>(baseType);
        Consume(TokenType::star);
    }

    baseType = DirectDeclaratorSuffix(tok, baseType, false);
    
    return baseType;
}

bool Parser::IsTypeName(TokenType tokenType) {
    if (tokenType == TokenType::kw_int) {
        return true;
    }else if (tokenType == TokenType::kw_struct || tokenType == TokenType::kw_union) {
        return true;
    }else if (tokenType == TokenType::kw_void) {
        return true;
    }
    return false;
}

bool Parser::IsFuncDecl() {
    sema.SetMode(Sema::Mode::Skip);
    bool isFunc = false;
    Token begin = tok;
    lexer.SaveState();

    auto baseType = ParseDeclSpec();
    if (tok.tokenType == TokenType::semi) {
        isFunc = false;
    }else {
        auto node = Declarator(baseType, true);
        if (node->ty->GetKind() == CType::TY_Func) {
            isFunc = true;
        }
    }
    lexer.RestoreState();
    tok = begin;
    sema.SetMode(Sema::Mode::Normal);
    return isFunc;
}

bool Parser::Expect(TokenType tokenType) {
    if (tok.tokenType == tokenType) {
        return true;
    }
    GetDiagEngine().Report(
        llvm::SMLoc::getFromPointer(tok.ptr), 
        diag::err_expected, 
        Token::GetSpellingText(tokenType), 
        llvm::StringRef(tok.ptr, tok.len));
    return false;
}

bool Parser::Consume(TokenType tokenType) {
    if (Expect(tokenType)) {
        Advance();
        return true;
    }
    return false;
}

void Parser::Advance() {
    lexer.NextToken(tok);
}