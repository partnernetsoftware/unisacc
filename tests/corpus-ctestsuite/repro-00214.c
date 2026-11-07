int main(void)
{
    int value = 3;
    ({ if (0) value = 9; value; });
    return value != 3;
}
