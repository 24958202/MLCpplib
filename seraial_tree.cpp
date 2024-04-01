#include <iostream>
#include <fstream>
#include <vector>

// Define a simple node structure
struct Node {
    int data;
    std::vector<Node*> children;
};

// Serialize the node tree to a binary file
void serialize(Node* root, std::ofstream& outFile) {
    if (!root) return;

    // Write the data of the current node
    outFile.write(reinterpret_cast<char*>(&root->data), sizeof(root->data));

    // Write the number of children
    int numChildren = root->children.size();
    outFile.write(reinterpret_cast<char*>(&numChildren), sizeof(numChildren));

    // Recursively serialize children
    for (Node* child : root->children) {
        serialize(child, outFile);
    }
}

// Deserialize the node tree from a binary file
Node* deserialize(std::ifstream& inFile) {
    int data;
    inFile.read(reinterpret_cast<char*>(&data), sizeof(data));

    int numChildren;
    inFile.read(reinterpret_cast<char*>(&numChildren), sizeof(numChildren));

    Node* root = new Node{data};
    for (int i = 0; i < numChildren; ++i) {
        root->children.push_back(deserialize(inFile));
    }

    return root;
}

// Print the entire tree in a depth-first manner
void printTree(Node* root, int depth = 0) {
    if (!root) return;

    // Print the data of the current node
    for (int i = 0; i < depth; ++i) std::cout << "  ";
    std::cout << root->data << std::endl;

    // Recursively print children
    for (Node* child : root->children) {
        printTree(child, depth + 1);
    }
}

int main() {
    // Create a sample node tree
    Node* root = new Node{1};
    Node* child1 = new Node{2};
    Node* child2 = new Node{3};
    root->children.push_back(child1);
    root->children.push_back(child2);

    // Save the node tree to a binary file
    std::ofstream outFile("/home/ronnieji/lib/db_tools/data.bin", std::ios::binary);
    serialize(root, outFile);
    outFile.close();

    // Load the node tree from the binary file
    std::ifstream inFile("/home/ronnieji/lib/db_tools/data.bin", std::ios::binary);
    Node* loadedRoot = deserialize(inFile);
    inFile.close();

    // Print out the entire tree
    std::cout << "Tree structure:" << std::endl;
    printTree(loadedRoot);

    // Clean up memory
    // (Note: You should implement a proper memory management strategy)
    delete root;
    delete loadedRoot;

    return 0;
}
