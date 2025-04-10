#pragma once
#include <memory>
#include <string>
#include <windows.h>
#undef max
#include <algorithm>


struct CommitInfo {
    int commitNumber;
    std::wstring fileName;
    std::wstring diffData;
};


// A commit node in the partially persistent tree.
struct CommitNode {
    int commitCounter;              // The commit order
    std::wstring fileName;          // File name for the commit
    std::wstring diffData;          // text summary of changes vs. the previous version
    int height;                     // Height for balancing (AVL)
    std::shared_ptr<CommitNode> left;
    std::shared_ptr<CommitNode> right;

    // Update the constructor to include an optional diff parameter.
    CommitNode(int counter, const std::wstring& fname, const std::wstring& diff = L"")
        : commitCounter(counter), fileName(fname), diffData(diff), height(1), left(nullptr), right(nullptr) {
    }
};


// Return the height of a node (or 0 if null).
int height(const std::shared_ptr<CommitNode>& node) {
    return node ? node->height : 0;
}


// Create a copy of the node and update its height.
std::shared_ptr<CommitNode> copyNode(const std::shared_ptr<CommitNode>& node) {
    if (!node) return nullptr;
    auto newNode = std::make_shared<CommitNode>(node->commitCounter, node->fileName, node->diffData);
    newNode->left = node->left;
    newNode->right = node->right;
    newNode->height = node->height;
    return newNode;
}


// Update the height based on the children.
std::shared_ptr<CommitNode> updateHeight(const std::shared_ptr<CommitNode>& node) {
    if (node)
        node->height = 1 + std::max(height(node->left), height(node->right));
    return node;
}


// Get the balance factor of a node.
int getBalance(const std::shared_ptr<CommitNode>& node) {
    return node ? height(node->left) - height(node->right) : 0;
}


std::shared_ptr<CommitNode> rightRotate(const std::shared_ptr<CommitNode>& y) {
    // Copy nodes for path copying.
    auto x = copyNode(y->left);
    auto T2 = x->right;

    // Rotate: create new version of y with updated left pointer.
    auto newY = copyNode(y);
    newY->left = T2;
    updateHeight(newY);

    // create new version of x and attach newY.
    x->right = newY;
    updateHeight(x);
    return x;
}


std::shared_ptr<CommitNode> leftRotate(const std::shared_ptr<CommitNode>& x) {
    auto y = copyNode(x->right);
    auto T2 = y->left;

    auto newX = copyNode(x);
    newX->right = T2;
    updateHeight(newX);

    y->left = newX;
    updateHeight(y);
    return y;
}


std::shared_ptr<CommitNode> insertNode(const std::shared_ptr<CommitNode>& root, int commitCounter, const std::wstring& fileName, const std::wstring& diffData) {
    if (!root)
        return std::make_shared<CommitNode>(commitCounter, fileName, diffData);

    auto newRoot = copyNode(root);
    if (commitCounter < newRoot->commitCounter)
        newRoot->left = insertNode(newRoot->left, commitCounter, fileName, diffData);
    else
        newRoot->right = insertNode(newRoot->right, commitCounter, fileName, diffData);

    updateHeight(newRoot);

    // Balance the tree.
    int balance = getBalance(newRoot);

    // Left Left case
    if (balance > 1 && commitCounter < newRoot->left->commitCounter)
        return rightRotate(newRoot);

    // Right Right case
    if (balance < -1 && commitCounter > newRoot->right->commitCounter)
        return leftRotate(newRoot);

    // Left Right case
    if (balance > 1 && commitCounter > newRoot->left->commitCounter) {
        newRoot->left = leftRotate(newRoot->left);
        return rightRotate(newRoot);
    }

    // Right Left case
    if (balance < -1 && commitCounter < newRoot->right->commitCounter) {
        newRoot->right = rightRotate(newRoot->right);
        return leftRotate(newRoot);
    }

    return newRoot;
}


void InOrderTraversal(const std::shared_ptr<CommitNode>& node,
    std::vector<CommitInfo>& commits)
{
    if (!node) return;
    InOrderTraversal(node->left, commits);
    commits.push_back({ node->commitCounter, node->fileName, node->diffData });
    InOrderTraversal(node->right, commits);
}