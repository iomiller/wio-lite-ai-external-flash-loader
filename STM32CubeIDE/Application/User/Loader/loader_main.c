/*
 * Dummy main required by the normal STM32H725 startup file.
 *
 * STM32CubeProgrammer does not use this as the loader entry point.
 * The loader is linked with ENTRY(Init), and CubeProgrammer calls Init().
 */
int main(void)
{
    while (1)
    {
    }
}
