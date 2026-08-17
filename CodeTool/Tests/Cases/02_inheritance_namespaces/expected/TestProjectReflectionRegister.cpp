extern void __RegisterClass__outer__Derived();
extern void __RegisterClass__outer__MultiDerived();
extern void __RegisterClass__outer__inner__Base();


extern void InitializeTypesTestProject()
{
    __RegisterClass__outer__Derived();
    __RegisterClass__outer__MultiDerived();
    __RegisterClass__outer__inner__Base();
}