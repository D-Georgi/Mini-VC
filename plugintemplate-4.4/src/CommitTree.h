#pragma once
#include <memory>
#include <string>
#include <windows.h>
#undef max
#include <algorithm>

// A commit node in the persistent tree.
struct CommitNode {
    int commitCounter;            // The commit order
    std::wstring fileName;        // File name for the commit
    int height;                   // Height for balancing (AVL)
    std::shared_ptr<CommitNode> left;
    std::shared_ptr<CommitNode> right;

    CommitNode(int counter, const std::wstring& fname)
        : commitCounter(counter), fileName(fname), height(1), left(nullptr), right(nullptr) {
    }
};

// Return the height of a node (or 0 if null).
int height(const std::shared_ptr<CommitNode>& node) {
    return node ? node->height : 0;
}

// Create a copy of the node (shallow copy for children) and update its height.
std::shared_ptr<CommitNode> copyNode(const std::shared_ptr<CommitNode>& node) {
    if (!node) return nullptr;
    auto newNode = std::make_shared<CommitNode>(node->commitCounter, node->fileName);
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
    auto T2 = x->right; // No need to copy T2, it is shared.

    // Rotate: create new version of y with updated left pointer.
    auto newY = copyNode(y);
    newY->left = T2;
    updateHeight(newY);

    // Now, create new version of x and attach newY.
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

std::shared_ptr<CommitNode> insertNode(const std::shared_ptr<CommitNode>& root, int commitCounter, const std::wstring& fileName) {
    if (!root)
        return std::make_shared<CommitNode>(commitCounter, fileName);

    // Copy the current node.
    auto newRoot = copyNode(root);

    // Decide whether to go left or right.
    if (commitCounter < newRoot->commitCounter) {
        newRoot->left = insertNode(newRoot->left, commitCounter, fileName);
    }
    else {
        newRoot->right = insertNode(newRoot->right, commitCounter, fileName);
    }

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

// Assumes CommitNode is defined in CommitTree.h
void InOrderTraversal(const std::shared_ptr<CommitNode>& node,
    std::vector<std::pair<int, std::wstring>>& commits)
{
    if (!node) return;
    InOrderTraversal(node->left, commits);
    commits.push_back({ node->commitCounter, node->fileName });
    InOrderTraversal(node->right, commits);
}
