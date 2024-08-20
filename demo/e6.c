
struct TreeNode
{
    int id;
    int height;
    struct TreeNode* right;
    struct TreeNode* left;
};

struct TreeNode* rotateLeft(struct TreeNode* pivot, struct TreeNode* parent);

struct TreeNode* rotateRight(struct TreeNode* pivot, struct TreeNode* parent);

int getBalance(struct TreeNode* h);

int height(struct TreeNode* h);

void recursiveAdd(struct TreeNode* h, struct TreeNode* p, struct TreeNode* parent, struct TreeNode** root);

void addNode(struct TreeNode* root, struct TreeNode* p);

void Rotate(struct TreeNode* h, struct TreeNode** parent, struct TreeNode** root);

void updateHeight(struct TreeNode* h);


int abs(int number)
{
    return number < 0 ? -number : number;
}

struct TreeNode* rotateLeft(struct TreeNode* pivot, struct TreeNode* parent)
{
    struct TreeNode* h = pivot->right;
    pivot->right = h->left;
    h->left = pivot;
    updateHeight(pivot);
    updateHeight(h);
    if (parent)
    {
        if (parent->left == pivot)
        {
            parent->left = h;
        }
        else
        {
            parent->right = h;
        }
        updateHeight(parent);
    }

    return h;
}

struct TreeNode* rotateRight(struct TreeNode* pivot, struct TreeNode* parent)
{
    struct TreeNode* h = pivot->left;
    pivot->left = h->right;
    h->right = pivot;
    updateHeight(pivot);
    updateHeight(h);
    if (parent)
    {
        if (parent->left == pivot)
        {
            parent->left = h;
        }
        else
        {
            parent->right = h;
        }
        updateHeight(parent);
    }

    return h;
}

void updateHeight(struct TreeNode* h)
{
    if (height(h->right) > height(h->left))
    {
        h->height = height(h->right);
    }
    else
    {
        h->height = height(h->left);
    }
}

void Rotate(struct TreeNode* h, struct TreeNode** parent, struct TreeNode** root)
{
    //Neue Höhe bekommen
    updateHeight(h);
    //balanz checken
    int balance = getBalance(h);
    if (abs(balance) == 2)
    {
        //korrigiern
        if (balance > 0)
        {
            if (getBalance(h->left) < 0)
            {
                rotateLeft(h->left, h);
            }
            struct TreeNode* newparent = rotateRight(h, *parent);
            if (h == *root)
            {
                *root = newparent;
            }
        }
        else
        {
            if (getBalance(h->right) > 0)
            {
                rotateRight(h->right, h);
            }
            struct TreeNode* newparent = rotateLeft(h, *parent);
            if (h == *root)
            {
                *root = newparent;
            }
        }
    }
}

int getBalance(struct TreeNode* h)
{
    //Balance = höhe des linkensubtree minus höhe des rechten subtree
    //0 ist voll balanziert, 1 ist linkslehnend, -1 ist rechtslehnen, -2 und 2 sind unbalanziert
    return height(h->left) - height(h->right);
}

int height(struct TreeNode* h)
{
    //height = Maximale Anzahl der Kanten von der Note zur untersten Ebene (Kanten...Verbindung zwischen zwei Noten)
    return (h ? h->height + 1 : 0);
}

void recursiveAdd(struct TreeNode* h, struct TreeNode* p, struct TreeNode* parent, struct TreeNode** root)
{
    //suchen nach Platz für neue Note, durch rekursiver Aufruf wird kein Elternpointer gebraucht
    if (p->id >= h->id)
    {
        if (h->right)
        {
            recursiveAdd(h->right, p, h, root);
        }
        else
        {
            h->right = p;
        }
    }
    else
    {
        if (h->left)
        {
            recursiveAdd(h->left, p, h, root);
        }
        else
        {
            h->left = p;
        }
    }
    //Balancieren
    Rotate(h, &parent, root);
}

void addNode(struct TreeNode* root, struct TreeNode* p)
{
    recursiveAdd(root, p, 0, &root);
}

int main()
{
    struct TreeNode root = {1001, 0, 0, 0};
    struct TreeNode newNode1 = {1002, 0, 0, 0};
    struct TreeNode newNode2 = {1003, 0, 0, 0};
    struct TreeNode newNode3 = {888, 0, 0, 0};
    struct TreeNode newNode4 = {10010, 0, 0, 0};
    struct TreeNode newNode5 = {1006, 0, 0, 0};
    struct TreeNode newNode6 = {1006, 0, 0, 0};
    struct TreeNode newNode7 = {1006, 0, 0, 0};
    struct TreeNode newNode8 = {1006, 0, 0, 0};
    struct TreeNode newNode9 = {1006, 0, 0, 0};
    struct TreeNode newNode10 = {1006, 0, 0, 0};
    addNode(&root, &newNode1);
    addNode(&root, &newNode2);
    addNode(&root, &newNode3);
    addNode(&root, &newNode4);
    addNode(&root, &newNode5);
    addNode(&root, &newNode6);
    addNode(&root, &newNode7);
    addNode(&root, &newNode8);
    addNode(&root, &newNode9);
    addNode(&root, &newNode10);
    return newNode4.height;
}
