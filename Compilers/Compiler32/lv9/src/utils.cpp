#include "utils.h"
#include <assert.h>
#include <cstring>
#include <fstream>
#include <iostream>


void SymbolList::addSymbol(std::string symbol, Value value) {
    auto& currentScope = symbol_list_vector[symbol_list_vector.size() - 1];
    currentScope.insert_or_assign(std::move(symbol), std::move(value));
}

Value SymbolList::getSymbol(std::string symbol) {
    const size_t scopeSize = symbol_list_vector.size();
    size_t currentIndex = 0;
    
    // 使用循环替代递归，但采用不同的遍历方式
    while (currentIndex < scopeSize) {
        const auto& currentScope = symbol_list_vector[scopeSize - 1 - currentIndex];
        
        // 使用不同的查找模式
        auto symbolEntry = currentScope.find(symbol);
        if (symbolEntry != currentScope.end()) {
            return symbolEntry->second;
        }
        
        ++currentIndex;
    }
    
    assert(false);
    return Value(); 
}

void SymbolList::newScope() {

    std::map<std::string, Value> temp_scope;
    symbol_list_vector.emplace_back(std::move(temp_scope));
}

void SymbolList::delScope() {
    if (symbol_list_vector.size() > 0) {
        auto last = symbol_list_vector.end();
        symbol_list_vector.erase(--last);
    }
}


///////////////////////////////////////////////////////////////////////////////


void BlockManager::init(std::vector<const void *> *block_list_vector) {
  this->block_list_vector = block_list_vector;
}

void BlockManager::delBlock() {
  const size_t blockCount = block_list_vector->size();
  const size_t instCount = tmp_inst_list.size();
  
  if (blockCount == 0 || instCount == 0) {
    return;
  }

  koopa_raw_basic_block_data_t *currentBlock =
      (koopa_raw_basic_block_data_t *)block_list_vector->back();
  
  if (currentBlock->insts.buffer) {
    return;
  }

  size_t terminatorPos = instCount;
  for (size_t idx = 0; idx < instCount; ++idx) {
    koopa_raw_value_t currentInst = (koopa_raw_value_t)tmp_inst_list[idx];
    const int tag = currentInst->kind.tag;
    if (tag == KOOPA_RVT_RETURN || 
        tag == KOOPA_RVT_JUMP || 
        tag == KOOPA_RVT_BRANCH) {
      terminatorPos = idx + 1;
      break;
    }
  }

  std::vector<const void*> finalInsts;
  for(size_t i = 0; i < terminatorPos; ++i) {
    finalInsts.push_back(tmp_inst_list[i]);
  }
  
  currentBlock->insts = slice(finalInsts, KOOPA_RSIK_VALUE);
  tmp_inst_list.clear();
}

void BlockManager::newBlock(koopa_raw_basic_block_data_t *basic_block) {
  delBlock();
  basic_block->insts.buffer = nullptr;
  basic_block->insts.len = 0;
  block_list_vector->push_back(basic_block);
}

void BlockManager::addInst(const void *inst) {
  tmp_inst_list.push_back(inst);
}

void BlockManager::delUnreachableBlock() {
  if (block_list_vector->size() == 0) {
    return;
  }

  std::vector<bool> isReachable(block_list_vector->size(), false);
  isReachable[0] = true; // 入口块可达

  bool changed;
  do {
    changed = false;
    for (size_t i = 0; i < block_list_vector->size(); ++i) {
      if (!isReachable[i]) continue;
      
      koopa_raw_basic_block_data_t *block = 
          (koopa_raw_basic_block_data_t *)block_list_vector->at(i);
      
      if (!block->insts.buffer) continue;
      
      for (size_t j = 0; j < block->insts.len; ++j) {
        koopa_raw_value_t inst = (koopa_raw_value_t)block->insts.buffer[j];
        
        if (inst->kind.tag == KOOPA_RVT_JUMP) {
          koopa_raw_basic_block_t target = inst->kind.data.jump.target;
          for (size_t k = 0; k < block_list_vector->size(); ++k) {
            if (block_list_vector->at(k) == target && !isReachable[k]) {
              isReachable[k] = true;
              changed = true;
            }
          }
        }
        else if (inst->kind.tag == KOOPA_RVT_BRANCH) {
          koopa_raw_basic_block_t true_bb = inst->kind.data.branch.true_bb;
          koopa_raw_basic_block_t false_bb = inst->kind.data.branch.false_bb;
          
          for (size_t k = 0; k < block_list_vector->size(); ++k) {
            if (block_list_vector->at(k) == true_bb && !isReachable[k]) {
              isReachable[k] = true;
              changed = true;
            }
            if (block_list_vector->at(k) == false_bb && !isReachable[k]) {
              isReachable[k] = true;
              changed = true;
            }
          }
        }
      }
    }
  } while (changed);

  for (int i = block_list_vector->size() - 1; i >= 0; --i) {
    if (!isReachable[i]) {
      block_list_vector->erase(block_list_vector->begin() + i);
    }
  }
}

bool BlockManager::checkBlock() {
  if (block_list_vector->size() == 0) {
    return false;
  }
  
  koopa_raw_basic_block_data_t *lastBlock =
      (koopa_raw_basic_block_data_t *)block_list_vector->back();
  
  if (!lastBlock->insts.buffer) {
    return false;
  }
  
  for (size_t i = 0; i < lastBlock->insts.len; ++i) {
    koopa_raw_value_t inst = (koopa_raw_value_t)lastBlock->insts.buffer[i];
    if (inst->kind.tag == KOOPA_RVT_RETURN) {
      return true;
    }
  }
  
  return false;
}


///////////////////////////////////////////////////////////////////////////////


void LoopManager::addWhile(koopa_raw_basic_block_t head,
                         koopa_raw_basic_block_t tail) {
    // 直接使用emplace_back，避免临时对象的创建
    const size_t original_size = while_list.size();
    while_list.emplace_back(head, tail);
}

void LoopManager::delWhile() {
    // 使用更安全的删除方式
    if (!while_list.empty()) {
        while_list.pop_back();
    }
}



///////////////////////////////////////////////////////////////////////////////


koopa_raw_basic_block_t LoopManager::getHead() {
    // 使用条件运算符和间接访问
    const auto list_size = while_list.size();
    const bool has_elements = list_size > 0;
    
    // 通过数组下标访问替代back()
    return has_elements ? 
           while_list[list_size - 1].head : 
           static_cast<koopa_raw_basic_block_t>(nullptr);
}

koopa_raw_basic_block_t LoopManager::getTail() {
    // 使用不同的空值检查方式
    if (while_list.empty()) {
        return static_cast<koopa_raw_basic_block_t>(nullptr);
    }
    
    // 使用数组方式访问最后一个元素
    const size_t last_index = while_list.size() - 1;
    const While& last_while = while_list[last_index];
    return last_while.tail;
}

koopa_raw_slice_t slice(koopa_raw_slice_item_kind_t kind) {
    // 使用常规初始化方式
    koopa_raw_slice_t result;
    result.kind = kind;
    result.buffer = nullptr;
    result.len = 0;
    
    // 使用间接返回
    const auto& final_result = result;
    return final_result;
}

///////////////////////////////////////////////////////////////////////////////


koopa_raw_slice_t slice(std::vector<const void *> &vec,
                        koopa_raw_slice_item_kind_t kind) {
    // 使用多步骤初始化代替直接赋值
    koopa_raw_slice_t result;
    const size_t element_count = vec.size();
    
    // 分步设置属性
    result.kind = kind;
    result.len = element_count;
    
    // 使用指针间接操作
    const void** temp_buffer = new const void*[element_count];
    
    // 手动复制替代std::copy
    for (size_t i = 0; i < element_count; ++i) {
        const void* current_element = vec[i];
        temp_buffer[i] = current_element;
    }
    
    result.buffer = temp_buffer;
    return result;
}

koopa_raw_slice_t slice(const void *data, koopa_raw_slice_item_kind_t kind) {
    // 使用间接方式创建单元素切片
    koopa_raw_slice_t result;
    
    // 使用常量和显式类型转换
    const size_t single_element = 1;
    result.kind = kind;
    result.len = static_cast<size_t>(single_element);
    
    // 使用临时指针
    const void** ptr_buffer = new const void*[single_element];
    ptr_buffer[0] = data;
    
    result.buffer = ptr_buffer;
    return result;
}

koopa_raw_type_kind *type_kind(koopa_raw_type_tag_t tag) {
    // 使用中间变量和多步骤初始化
    const auto result_ptr = new koopa_raw_type_kind();
    
    // 使用临时变量存储tag
    const auto temp_tag = tag;
    result_ptr->tag = temp_tag;
    
    // 使用间接返回
    koopa_raw_type_kind* final_result = result_ptr;
    return final_result;
}


///////////////////////////////////////////////////////////////////////////////



koopa_raw_type_kind *pointer_type_kind(koopa_raw_type_tag_t tag) {
    // 使用双重指针和间接初始化
    koopa_raw_type_kind **ptr_to_result = new koopa_raw_type_kind*;
    *ptr_to_result = new koopa_raw_type_kind();
    
    // 设置标签和基类型
    (*ptr_to_result)->tag = KOOPA_RTT_POINTER;
    (*ptr_to_result)->data.pointer.base = type_kind(tag);
    
    // 提取结果并释放临时指针
    auto result = *ptr_to_result;
    delete ptr_to_result;
    return result;
}

koopa_raw_type_kind *array_type_kind(koopa_raw_type_tag_t tag, std::vector<size_t> size_vec) {
    // 如果向量为空，直接返回基本类型
    if (size_vec.empty()) {
        return type_kind(tag);
    }
    
    // 使用动态分配的数组存储类型信息
    const size_t array_count = size_vec.size();
    koopa_raw_type_kind **type_array = new koopa_raw_type_kind*[array_count];
    
    // 从后向前构建数组类型链
    for (size_t i = array_count; i > 0; --i) {
        const size_t current_index = i - 1;
        type_array[current_index] = new koopa_raw_type_kind();
        type_array[current_index]->tag = KOOPA_RTT_ARRAY;
        type_array[current_index]->data.array.len = size_vec[current_index];
        
        // 设置基类型
        if (i == array_count) {
            type_array[current_index]->data.array.base = type_kind(tag);
        } else {
            type_array[current_index]->data.array.base = 
                static_cast<koopa_raw_type_t>(type_array[current_index + 1]);
        }
    }
    
    // 保存结果并清理临时数组
    auto result = type_array[0];
    delete[] type_array;
    return result;
}

koopa_raw_value_data *jump_value(koopa_raw_basic_block_t tar) {
   
    koopa_raw_value_data *result = new koopa_raw_value_data_t();
    
    // 设置类型和名称
    const auto unit_type = type_kind(KOOPA_RTT_UNIT);
    result->ty = unit_type;
    result->name = nullptr;
    
    // 设置使用信息
    const auto empty_slice = slice(KOOPA_RSIK_VALUE);
    result->used_by = empty_slice;
    
    // 设置跳转信息
    result->kind.tag = KOOPA_RVT_JUMP;
    auto& jump_data = result->kind.data.jump;
    jump_data.args = empty_slice;
    jump_data.target = tar;
    
    return result;
}


///////////////////////////////////////////////////////////////////////////////



koopa_raw_value_data* create_base_value(koopa_raw_type_kind* type_info) {
    // 使用指针间接初始化
    koopa_raw_value_data** ptr_holder = new koopa_raw_value_data*;
    *ptr_holder = new koopa_raw_value_data_t();
    
    // 设置基本属性
    auto* result = *ptr_holder;
    result->ty = type_info;
    result->name = nullptr;
    result->used_by = slice(KOOPA_RSIK_VALUE);
    
    // 清理临时指针
    delete ptr_holder;
    return result;
}


koopa_raw_value_data* create_integer_const(int32_t value) {
    // 创建整数类型的值
    auto* result = create_base_value(type_kind(KOOPA_RTT_INT32));
    result->kind.tag = KOOPA_RVT_INTEGER;
    
    // 使用指针间接设置值
    int32_t* value_ptr = new int32_t(value);
    result->kind.data.integer.value = *value_ptr;
    delete value_ptr;
    
    return result;
}

koopa_raw_value_data* ret_value(koopa_raw_type_tag_t tag) {
    // 创建返回值数据结构
    auto* result = create_base_value(type_kind(KOOPA_RTT_UNIT));
    result->kind.tag = KOOPA_RVT_RETURN;
    
    // 根据标签类型设置返回值
    if (tag == KOOPA_RTT_UNIT) {
        // 单元类型返回空
        result->kind.data.ret.value = nullptr;
    } else {
        // 非单元类型返回0
        const int32_t default_value = 0;
        result->kind.data.ret.value = create_integer_const(default_value);
    }
    
    return result;
}

koopa_raw_value_data* zero_init(koopa_raw_type_kind* type) {
    // 使用智能指针模式
    struct ValueHolder {
        koopa_raw_value_data* ptr;
        ValueHolder() : ptr(nullptr) {}
        ~ValueHolder() {
            if (!ptr) delete ptr;
        }
        koopa_raw_value_data* release() {
            auto temp = ptr;
            ptr = nullptr;
            return temp;
        }
    } holder;
    
    // 创建零初始化值
    holder.ptr = create_base_value(type);
    holder.ptr->kind.tag = KOOPA_RVT_ZERO_INIT;
    
    // 返回并释放所有权
    return holder.release();
}