extern void __RegisterClass__o2__AtlasAssetConverter();
extern void __RegisterClass__o2__AtlasAssetConverter__Image();
extern void __RegisterClass__o2__FolderAssetConverter();
extern void __RegisterClass__o2__IAssetConverter();
extern void __RegisterClass__o2__ImageAssetConverter();
extern void __RegisterClass__o2__StdAssetConverter();
extern void __RegisterClass__o2__VectorImageAssetConverter();
extern void __RegisterClass__o2__ImageCompressor__Config();


extern void InitializeTypesAssetsBuildTool()
{
    __RegisterClass__o2__AtlasAssetConverter();
    __RegisterClass__o2__AtlasAssetConverter__Image();
    __RegisterClass__o2__FolderAssetConverter();
    __RegisterClass__o2__IAssetConverter();
    __RegisterClass__o2__ImageAssetConverter();
    __RegisterClass__o2__StdAssetConverter();
    __RegisterClass__o2__VectorImageAssetConverter();
    __RegisterClass__o2__ImageCompressor__Config();
}