/*
 * @lc app=leetcode.cn id=1 lang=c
 *
 * [1] 两数之和
 */

// @lc code=start
/**
 * Note: The returned array must be malloced, assume caller calls free().
 */
#include <stdio.h>
#include <stdlib.h>

// 哈希表节点结构
typedef struct HashNode {
    int key;    // 数组中的值
    int value;  // 对应的索引
    struct HashNode* next;  // 处理冲突的链表
} HashNode;

// 哈希表结构
typedef struct {
    HashNode** buckets;
    int size;
} HashTable;

// 哈希函数
int hash(int key, int size) {
    return abs(key) % size;
}

// 创建哈希表
HashTable* createHashTable(int size) {
    HashTable* table = (HashTable*)malloc(sizeof(HashTable));
    table->size = size;
    table->buckets = (HashNode**)calloc(size, sizeof(HashNode*));
    return table;
}

// 插入键值对到哈希表
void insert(HashTable* table, int key, int value) {
    int index = hash(key, table->size);
    
    HashNode* newNode = (HashNode*)malloc(sizeof(HashNode));
    newNode->key = key;
    newNode->value = value;
    newNode->next = table->buckets[index];
    table->buckets[index] = newNode;
}

// 在哈希表中查找键
int search(HashTable* table, int key) {
    int index = hash(key, table->size);
    HashNode* node = table->buckets[index];
    
    while (node) {
        if (node->key == key) {
            return node->value;
        }
        node = node->next;
    }
    return -1;  // 未找到
}

// 释放哈希表内存
void freeHashTable(HashTable* table) {
    for (int i = 0; i < table->size; i++) {
        HashNode* node = table->buckets[i];
        while (node) {
            HashNode* temp = node;
            node = node->next;
            free(temp);
        }
    }
    free(table->buckets);
    free(table);
}

/**
 * 哈希表解法：一次遍历，用哈希表存储已访问的数字和索引
 * 时间复杂度：O(n)
 * 空间复杂度：O(n)
 */
int* twoSum(int* nums, int numsSize, int target, int* returnSize) {
    // 创建哈希表
    HashTable* table = createHashTable(numsSize * 2);
    
    // 分配返回数组的内存
    int* result = (int*)malloc(2 * sizeof(int));
    *returnSize = 2;
    
    for (int i = 0; i < numsSize; i++) {
        int complement = target - nums[i];
        
        // 查找complement是否在哈希表中
        int complementIndex = search(table, complement);
        if (complementIndex != -1) {
            result[0] = complementIndex;
            result[1] = i;
            freeHashTable(table);
            return result;
        }
        
        // 将当前数字和索引插入哈希表
        insert(table, nums[i], i);
    }
    
    // 如果没有找到答案
    *returnSize = 0;
    freeHashTable(table);
    free(result);
    return NULL;
}

// @lc code=end

