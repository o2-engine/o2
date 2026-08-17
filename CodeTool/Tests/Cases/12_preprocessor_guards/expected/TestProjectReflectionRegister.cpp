extern void __RegisterClass__Featured();
extern void __RegisterClass__Legacy();
extern void __RegisterClass__Guarded();


extern void InitializeTypesTestProject()
{
    __RegisterClass__Featured();
    __RegisterClass__Legacy();
    __RegisterClass__Guarded();
}