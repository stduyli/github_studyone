/*
 * @lc app=leetcode.cn id=2 lang=c
 *
 * [2] 两数相加
 */

// @lc code=start
/**
 * Definition for singly-linked list.
 * struct ListNode {
 *     int val;
 *     struct ListNode *next;
 * };
 */
struct ListNode* addTwoNumbers(struct ListNode* l1, struct ListNode* l2) {
    struct ListNode *head = NULL, *tail = NULL;
    int carry = 0;  // 进位
    
    // 当l1或l2任一个还有节点，或者还有进位时，继续循环
    while (l1 || l2 || carry) {
        int sum = carry;  // 加上上一位的进位
        
        // 分别取出l1和l2当前节点的值
        if (l1) {
            sum += l1->val;
            l1 = l1->next;
        }
        if (l2) {
            sum += l2->val;
            l2 = l2->next;
        }
        
        // 计算进位和当前位的值
        carry = sum / 10;
        sum = sum % 10;
        
        // 创建新节点
        struct ListNode* newNode = (struct ListNode*)malloc(sizeof(struct ListNode));
        newNode->val = sum;
        newNode->next = NULL;
        
        // 将新节点添加到结果链表中
        if (!head) {
            head = newNode;
            tail = newNode;
        } else {
            tail->next = newNode;
            tail = newNode;
        }
    }
    
    return head;
}
// @lc code=end

