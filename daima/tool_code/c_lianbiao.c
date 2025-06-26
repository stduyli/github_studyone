#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    int data;  // 节点数据域
    struct Node *next;  // 指针域
}Node_lian;

int main() {
    // 创建头节点
    Node_lian *head = (Node_lian *)malloc(sizeof(Node_lian));
    head->next = NULL;
    Node_lian *p = head;  // p为工作指针
    int n;
    
    // 输入链表长度
    printf("请输入链表长度：");
    scanf("%d", &n);
    
    // 创建n个节点的链表
    for (int i = 0; i < n; i++) {
        // 创建新节点
        Node_lian *newNode = (Node_lian *)malloc(sizeof(Node_lian));
        
        // 输入节点数据
        printf("请输入第%d个节点的数据:", i + 1);
        scanf("%d", &newNode->data);
        
        // 初始化新节点的指针域
        newNode->next = NULL;
        
        // 将新节点连接到链表末尾
        p->next = newNode;
        p = p->next;
    }
    
    // 输出链表数据
    p = head->next;
    while (p != NULL) {
        printf("%d ", p->data);
        p = p->next;
    }
    
    // 输出链表中各节点的指针域地址
    p = head->next;
    while (p != NULL) {
        printf("%p ", p->next);
        p = p->next;
    }
}