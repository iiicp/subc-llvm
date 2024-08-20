struct Point
{
    struct Point* next;
};

int getListCount(struct Point* first)
{
    struct Point* current = first;
    int i = 0;
    for(;current;)
    {
        current = current->next;
        i++;
    }
    return i;
}

int main()
{
    struct Point first = {0};
    struct Point second = {0};
    struct Point third = {0};
    first.next = &second;
    second.next = &third;
    third.next = 0;
    return getListCount(&first);
}
