#include "type.h"

std::shared_ptr<CType> CType::IntType = std::make_shared<CPrimaryType>(Kind::TY_Int, 4, 4);
std::shared_ptr<CType> CType::VoidType = std::make_shared<CPrimaryType>(Kind::TY_Void, 0, 0);
std::shared_ptr<CType> CType::CharType = std::make_shared<CPrimaryType>(Kind::TY_Char, 1, 1);

llvm::StringRef CType::GenAnonyRecordName(TagKind tagKind) {
    static long long idx = 0;
    std::string name;
    if (tagKind == TagKind::kStruct) {
        name = "__1anony_struct_" + std::to_string(idx++) + "_";
    }else {
        name = "__1anony_union_" + std::to_string(idx++) + "_";
    }
    char *buf = (char *)malloc(name.size()+1);
    memset(buf, 0, name.size());
    strcpy(buf, name.data());
    return llvm::StringRef(buf, name.size());
}

/// 地址对齐算法
/// 找到大于等于x，且按照align(必须是2的倍数)对其的最小地址
/// (0, 1) => 0
/// (1, 4) => 4
static int roundup(int x, int align) {
  return (x + align - 1) & ~(align - 1);
}

CRecordType::CRecordType(llvm::StringRef name, const std::vector<Member> &members, TagKind tagKind) 
    : CType(CType::TY_Record, 0, 0), name(name), members(members), tagKind(tagKind){
    if (tagKind == TagKind::kStruct) {
        UpdateStructOffset();
    }else {
        UpdateUnionOffset();
    }
}

void CRecordType::SetMembers(const std::vector<Member>& members) {
    this->members = members;
    if (tagKind == TagKind::kStruct) {
        UpdateStructOffset();
    }else {
        UpdateUnionOffset();
    }
}
/**
 struct -> size (all element size + padding)
 every element -> (offset + size)
 */
void CRecordType::UpdateStructOffset() {
    int offset = 0;
    int idx = 0;
    int total_size = 0, max_align = 0, max_element_size = 0, max_element_idx = 0;
    for (auto &m : members) {
        offset = roundup(offset, m.ty->GetAlign());
        m.offset = offset;
        m.elemIdx = idx++;

        /// 求出最大member的对齐数
        if (max_align < m.ty->GetAlign()) {
            max_align = m.ty->GetAlign();
        }

        if (max_element_size < m.ty->GetSize()) {
            max_element_size = m.ty->GetSize();
            max_element_idx = m.elemIdx;
        }

        /// 下一个元素的offset
        offset += m.ty->GetSize();
    }
    // struct A{int a; int b;}
    total_size = roundup(offset, max_align);

    size = total_size;
    align = max_align;
    maxElementIdx = max_element_idx;
}

void CRecordType::UpdateUnionOffset() {
    int max_align = 0, max_size = 0, max_element_idx = 0;
    int idx = 0;
    for (auto &m : members) {
        m.offset = 0;
        m.elemIdx = idx++;

        if (max_align < m.ty->GetAlign()) {
            max_align = m.ty->GetAlign();
        }

        if (max_size < m.ty->GetSize()) {
            max_size = m.ty->GetSize();
            max_element_idx = m.elemIdx;
        }
    }

    max_size = roundup(max_size, max_align);

    size = max_size;
    align = max_align;
    maxElementIdx = max_element_idx;
}

CFuncType::CFuncType(std::shared_ptr<CType> retType, const std::vector<Param>& params, llvm::StringRef name, bool isVarArg) 
 : CType(CType::TY_Func, 1, 1), retType(retType), params(params), name(name), isVarArg(isVarArg) {

}