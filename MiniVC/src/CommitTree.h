#pragma once
#include <memory>
#include <string>
#include <windows.h>
#include <algorithm>
#include <vector>
#undef max


struct CommitInfo {
    int commitNumber;
    std::wstring fileName;
    std::wstring diffData;
    std::wstring commitMessage;
};


struct CommitNode;


struct ModificationRecord {
    enum Field { LEFT, RIGHT, HEIGHT };
    int version;      // The version (or commit key) when this update occurred.
    Field field;      // Which field was updated.
    // For pointer fields (LEFT or RIGHT)
    std::shared_ptr<CommitNode> newChild;
    // For the height field.
    int newHeight;

    ModificationRecord()
        : version(0), field(LEFT), newChild(nullptr), newHeight(0) {
    }

    // pointer modifications.
    ModificationRecord(int ver, Field f, std::shared_ptr<CommitNode> child)
        : version(ver), field(f), newChild(child), newHeight(0) {
    }

    // height modification.
    ModificationRecord(int ver, Field f, int h)
        : version(ver), field(f), newChild(nullptr), newHeight(h) {
    }
};


// A commit node in the partially persistent tree: uses fat node approach from Driscoll
struct CommitNode {
    int commitCounter;
    std::wstring fileName;
    std::wstring diffData;
    std::wstring commitMessage;
    int height;
    std::shared_ptr<CommitNode> left;
    std::shared_ptr<CommitNode> right;

    // Fat node fields: a fixed-size modification log.
    static const int MAX_MODS = 2;
    ModificationRecord mods[MAX_MODS];
    int modCount;

    CommitNode(int counter, const std::wstring& fname, const std::wstring& diff = L"", const std::wstring& msg = L"")
        : commitCounter(counter), fileName(fname), diffData(diff), commitMessage(msg),
        height(1), left(nullptr), right(nullptr), modCount(0) {
    }
};


std::shared_ptr<CommitNode> getLeft(const std::shared_ptr<CommitNode>& node, int version) {
    if (!node) return nullptr;
    std::shared_ptr<CommitNode> result = node->left;
    for (int i = 0; i < node->modCount; i++) {
        if (node->mods[i].field == ModificationRecord::LEFT && node->mods[i].version <= version) {
            result = node->mods[i].newChild;
        }
    }
    return result;
}


std::shared_ptr<CommitNode> getRight(const std::shared_ptr<CommitNode>& node, int version) {
    if (!node) return nullptr;
    std::shared_ptr<CommitNode> result = node->right;
    for (int i = 0; i < node->modCount; i++) {
        if (node->mods[i].field == ModificationRecord::RIGHT && node->mods[i].version <= version) {
            result = node->mods[i].newChild;
        }
    }
    return result;
}

int getHeight(const std::shared_ptr<CommitNode>& node, int version) {
    if (!node) return 0;
    int result = node->height;
    for (int i = 0; i < node->modCount; i++) {
        if (node->mods[i].field == ModificationRecord::HEIGHT && node->mods[i].version <= version) {
            result = node->mods[i].newHeight;
        }
    }
    return result;
}


// full mod list triggers a new node and leaves old node alone
std::shared_ptr<CommitNode> copyFullNode(const std::shared_ptr<CommitNode>& node, int version) {
    if (!node) return nullptr;
    auto newNode = std::make_shared<CommitNode>(node->commitCounter, node->fileName, node->diffData, node->commitMessage);
    newNode->left = getLeft(node, version);
    newNode->right = getRight(node, version);
    newNode->height = getHeight(node, version);
    newNode->modCount = 0;
    return newNode;
}


std::shared_ptr<CommitNode> updateLeft(const std::shared_ptr<CommitNode>& node,
    const std::shared_ptr<CommitNode>& newLeft, int version) {
    if (!node) return nullptr;
    if (node->modCount < CommitNode::MAX_MODS) {
        node->mods[node->modCount] = ModificationRecord(version, ModificationRecord::LEFT, newLeft);
        node->modCount++;
        return node;
    }
    else {
        auto newNode = copyFullNode(node, version);
        newNode->left = newLeft;
        return newNode;
    }
}


std::shared_ptr<CommitNode> updateRight(const std::shared_ptr<CommitNode>& node,
    const std::shared_ptr<CommitNode>& newRight, int version) {
    if (!node) return nullptr;
    if (node->modCount < CommitNode::MAX_MODS) {
        node->mods[node->modCount] = ModificationRecord(version, ModificationRecord::RIGHT, newRight);
        node->modCount++;
        return node;
    }
    else {
        auto newNode = copyFullNode(node, version);
        newNode->right = newRight;
        return newNode;
    }
}


std::shared_ptr<CommitNode> updateHeight(const std::shared_ptr<CommitNode>& node,
    int version, int newHeight) {
    if (!node) return nullptr;
    if (node->modCount < CommitNode::MAX_MODS) {
        node->mods[node->modCount] = ModificationRecord(version, ModificationRecord::HEIGHT, newHeight);
        node->modCount++;
        return node;
    }
    else {
        auto newNode = copyFullNode(node, version);
        newNode->height = newHeight;
        return newNode;
    }
}


std::shared_ptr<CommitNode> rightRotate(const std::shared_ptr<CommitNode>& y, int version) {
    // x will be the effective left child of y.
    auto x = copyFullNode(getLeft(y, version), version);
    auto T2 = getRight(x, version);
    auto newY = updateLeft(y, T2, version);
    newY = updateHeight(newY, version, 1 + std::max(getHeight(getLeft(newY, version), version),
        getHeight(getRight(newY, version), version)));
    x = updateRight(x, newY, version);
    x = updateHeight(x, version, 1 + std::max(getHeight(getLeft(x, version), version),
        getHeight(getRight(x, version), version)));
    return x;
}

std::shared_ptr<CommitNode> leftRotate(const std::shared_ptr<CommitNode>& x, int version) {
    auto y = copyFullNode(getRight(x, version), version);
    auto T2 = getLeft(y, version);
    auto newX = updateRight(x, T2, version);
    newX = updateHeight(newX, version, 1 + std::max(getHeight(getLeft(newX, version), version),
        getHeight(getRight(newX, version), version)));
    y = updateLeft(y, newX, version);
    y = updateHeight(y, version, 1 + std::max(getHeight(getLeft(y, version), version),
        getHeight(getRight(y, version), version)));
    return y;
}


std::shared_ptr<CommitNode> insertNode(const std::shared_ptr<CommitNode>& root, int commitCounter,
    const std::wstring& fileName, const std::wstring& diffData,
    const std::wstring& commitMessage = L"") {
    int version = commitCounter;  // Each new insertion uses its commit number as its version.
    if (!root)
        return std::make_shared<CommitNode>(commitCounter, fileName, diffData, commitMessage);

    // “Copy” the root using its effective fields for the current version.
    auto newRoot = copyFullNode(root, version);
    if (commitCounter < newRoot->commitCounter) {
        auto updatedLeft = insertNode(getLeft(newRoot, version), commitCounter, fileName, diffData, commitMessage);
        newRoot = updateLeft(newRoot, updatedLeft, version);
    }
    else {
        auto updatedRight = insertNode(getRight(newRoot, version), commitCounter, fileName, diffData, commitMessage);
        newRoot = updateRight(newRoot, updatedRight, version);
    }
    int newHeight = 1 + std::max(getHeight(getLeft(newRoot, version), version), getHeight(getRight(newRoot, version), version));
    newRoot = updateHeight(newRoot, version, newHeight);

    int balance = getHeight(getLeft(newRoot, version), version) - getHeight(getRight(newRoot, version), version);

    // Balance the tree following AVL rotations.
    // Left Left case.
    if (balance > 1 && commitCounter < getLeft(newRoot, version)->commitCounter)
        return rightRotate(newRoot, version);
    // Right Right case.
    if (balance < -1 && commitCounter >= getRight(newRoot, version)->commitCounter)
        return leftRotate(newRoot, version);
    // Left Right case.
    if (balance > 1 && commitCounter >= getLeft(newRoot, version)->commitCounter) {
        auto updatedLeft = leftRotate(getLeft(newRoot, version), version);
        newRoot = updateLeft(newRoot, updatedLeft, version);
        return rightRotate(newRoot, version);
    }
    // Right Left case.
    if (balance < -1 && commitCounter < getRight(newRoot, version)->commitCounter) {
        auto updatedRight = rightRotate(getRight(newRoot, version), version);
        newRoot = updateRight(newRoot, updatedRight, version);
        return leftRotate(newRoot, version);
    }
    return newRoot;
}


void InOrderTraversal(const std::shared_ptr<CommitNode>& node, int version, std::vector<CommitInfo>& commits) {
    if (!node)
        return;
    InOrderTraversal(getLeft(node, version), version, commits);
    commits.push_back({ node->commitCounter, node->fileName, node->diffData, node->commitMessage });
    InOrderTraversal(getRight(node, version), version, commits);
}