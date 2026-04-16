#include "o2/O2.h"

#include "o2AssetBuilder/AssetsBuilder.h"
#include "o2/Utils/System/CommandLineOptions.h"

using namespace o2;

extern void InitializeTypesGameLib();
extern void InitializeTypesAssetsBuildTool();

int main(int argc, char* argv[])
{
    INITIALIZE_O2;
    InitializeTypesGameLib();
    InitializeTypesAssetsBuildTool();

    const auto platformKey = "-platform";
    const auto sourceDirKey = "-source";
    const auto targetDirKey = "-target";
    const auto targetAssetsTreeKey = "-target-tree";
    const auto compressorConfigPathKey = "-compressor-config";
    const auto forcibleKey = "-forcible";

    Map<String, String> options = CommandLineOptions::Parse(argc, argv);

    Platform platform = Platform::Windows;
    if (options.ContainsKey(platformKey))
        platform = Reflection::Instance().GetEnumValue<Platform>(options[platformKey]);
    else
    {
        std::cout << "Platform must be specified via " << platformKey << std::endl;
        return -1;
    }

    String sourceDir;
    if (options.ContainsKey(sourceDirKey))
        sourceDir = options[sourceDirKey];
    else
    {
        std::cout << "Source assets directory must be specified via " << sourceDirKey << std::endl;
        return -1;
    }

    String targetDir;
    if (options.ContainsKey(targetDirKey))
        targetDir = options[targetDirKey];
    else
    {
        std::cout << "Target built assets directory must be specified via " << targetDirKey << std::endl;
        return -1;
    }

    String targetTreeDir;
    if (options.ContainsKey(targetAssetsTreeKey))
        targetTreeDir = options[targetAssetsTreeKey];
    else
    {
        std::cout << "Target built assets tree path must be specified via " << targetAssetsTreeKey << std::endl;
        return -1;
    }

    String compressorConfigPath;
    if (options.ContainsKey(compressorConfigPathKey))
        compressorConfigPath = options[compressorConfigPathKey];
    else
    {
        std::cout << "Images compressor config path must be specified via " << compressorConfigPathKey << std::endl;
        return -1;
    }

    bool forcible = false;
    if (options.ContainsKey(forcibleKey))
        forcible = (bool)options[forcibleKey];

    AssetsBuilder builder;
    builder.BuildAssets(platform, sourceDir, targetDir, targetTreeDir, compressorConfigPath, forcible);

    return 0;
}
