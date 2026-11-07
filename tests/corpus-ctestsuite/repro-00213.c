int main(void)
{
    return 1 ? 0 : ({ int spare = 1; spare; });
}
