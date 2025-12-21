#pragma once
#include <string>
#include <unordered_map>
#include <vector>

enum class AssetType { Unknown, Model, Texture };

struct AssetRecord {
    std::string guid;
    AssetType type = AssetType::Unknown;
    std::string sourcePath;  // Assets/...
    std::string metaPath;    // Assets/...meta
    std::string libraryDir;  // Library/Imported/<guid>/
    std::vector<std::string> artifacts; // ficheros generados en Library
};

class AssetDatabase {
public:
    static void Init();
    static void Shutdown();

    // Importa un fichero externo (drag&drop, etc.) -> lo copia a Assets/ y crea Library cache
    static AssetRecord* ImportExternalFile(const std::string& externalPath);

    // Borrar desde editor: borra Assets + meta + Library
    static bool DeleteAsset(const std::string& guid, bool force = false);

    static const std::unordered_map<std::string, AssetRecord>& GetAll();
    static AssetRecord* FindByGuid(const std::string& guid);

private:
    static std::unordered_map<std::string, AssetRecord> sAssets;

    static void EnsureFolders();
    static void LoadDB();
    static void SaveDB();

    static AssetType GuessTypeFromExtension(const std::string& path);
    static std::string GenerateGuid();
    static bool WriteMeta(const AssetRecord& rec);

    static std::string MakeUniquePathInAssets(const std::string& filename);
    static bool ImportToLibrary(AssetRecord& rec);

    static std::string ToLower(std::string s);
};
