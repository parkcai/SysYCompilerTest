#pragma once

#include <unordered_map>
#include <vector>
#include <koopa.h>

using namespace std;

enum ItemType{
    SYMBOLTABLE_ITEM_CONST,
    SYMBOLTABLE_ITEM_VAR,
    SYMBOLTABLE_ITEM_FUNC,
    SYMBOLTABLE_ITEM_UNKNOWN,
};

struct Item {
public:
    ItemType type;
    union {
        int c;
        koopa_raw_value_data_t *v;
        koopa_raw_function_data_t *f;
    } data;
    Item() { type = SYMBOLTABLE_ITEM_UNKNOWN; }
    Item(int c) { type = SYMBOLTABLE_ITEM_CONST; data.c = c; }
    Item(koopa_raw_value_data_t *v) { type = SYMBOLTABLE_ITEM_VAR; data.v = v; }
    Item(koopa_raw_function_data_t *f) { type = SYMBOLTABLE_ITEM_FUNC; data.f = f; }
};

class SymbolTable {
public:
    static Item getItem(string ident);
    static Item getItem(string ident, ItemType type);
    static void getAvailableName(string &ident);
    static void addItem(string ident, int c);
    static void addItem(string ident, koopa_raw_value_data_t *v);
    static void addItem(string ident, koopa_raw_function_data_t *f);
    static void addTable();
    static void removeTable();
private:
    inline static vector<unordered_map<string, Item> > tables = {};
};