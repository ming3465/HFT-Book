#include <iostream>
using namespace std;

// Construct Node

struct Node {
    int data;
    Node* prev;
    Node* next;
    Node(int val) : data(val), prev(nullptr), next(nullptr) {}
};
class DoubleLinkedList {

private:
    Node* head;
    Node* tail;

public:
    DoubleLinkedList() : head(nullptr), tail(nullptr) {}
    ~DoubleLinkedList() {
        Node* current = head;
        while (current != nullptr) {
            Node* next = current->next;
            delete current;
            current = next;
        }
    }
    void append(int val) {
        Node* newNode = new Node(val);
        if (head == nullptr) {
            head = tail = newNode;
        } else {
            tail->next = newNode;
            newNode->prev = tail;
            tail = newNode;
        }
    }
    void printForward() {
        Node* current = head;
        while (current != nullptr) {
            cout << current->data << " ";
            current = current->next;
        }
        cout << '\n';
    }
    void printBackward() {
        Node* current = tail;
        while (current != nullptr) {
            cout << current->data << " ";
            current = current->prev;
        }
        cout << '\n';
    }
};

int main() {
    DoubleLinkedList DLL;
    DLL.append(7);
    DLL.append(8);
    DLL.append(9);
    DLL.append(10);
    DLL.append(11);

    cout << "Forward traversal of the list: ";
    DLL.printForward();
     // Output: Forward traversal of the list: 7 8 9 10 11

    cout << "Backward traversal of the list: ";
    DLL.printBackward(); 
    // Output: Backward traversal of the list: 11 10 9 8 7 

    return 0;
}