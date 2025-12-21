#include "AssetDatabase.h"

#include "SceneManager.h"
#include "Rendering/Renderer.h"


#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>

std::unordered_map<std::string, AssetRecord> AssetDatabase::sAssets;

static const char* kAssetsFolder = "Assets";
static const char* kLibraryFolder = "Library";
static const char* kImportedFolder = "Library/Imported";
static const char* kDBPath = "Library/AssetDB.txt";

std::string AssetDatabase::ToLower(std::string s) {
    for (char& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

void AssetDatabase::EnsureFolders() {
    std::error_code ec;
    std::filesystem::create_directories(kAssetsFolder, ec);
    std::filesystem::create_directories(kLibraryFolder, ec);
    std::filesystem::create_directories(kImportedFolder, ec);
}

AssetType AssetDatabase::GuessTypeFromExtension(const std::string& path) {
    std::filesystem::path p(path);
    std::string ext = ToLower(p.extension().string());

    if (ext == ".fbx") return AssetType::Model;
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp") return AssetType::Texture;

    return AssetType::Unknown;
}

std::string AssetDatabase::GenerateGuid() {
    static std::mt19937 rng{ std::random_device{}() };
    static const char* hex = "0123456789abcdef";
    std::string g; g.resize(32);
    for (int i = 0; i < 32; ++i) g[i] = hex[rng() % 16];
    return g;
}

bool AssetDatabase::WriteMeta(const AssetRecord& rec) {
    std::ofstream f(rec.metaPath, std::ios::out | std::ios::trunc);
    if (!f.is_open()) return false;

    f << "guid=" << rec.guid << "\n";
    f << "type=" << (int)rec.type << "\n";
    return true;
}

void AssetDatabase::SaveDB() {
    EnsureFolders();
    std::ofstream f(kDBPath, std::ios::out | std::ios::trunc);
    if (!f.is_open()) return;

    // Formato simple por líneas:
    // guid|type|sourcePath|metaPath|libraryDir
    for (const auto& kv : sAssets) {
        const AssetRecord& r = kv.second;
        f << r.guid << "|"
            << (int)r.type << "|"
            << r.sourcePath << "|"
            << r.metaPath << "|"
            << r.libraryDir << "\n";
    }
}

void AssetDatabase::LoadDB() {
    EnsureFolders();
    sAssets.clear();

    std::ifstream f(kDBPath);
    if (!f.is_open()) return;

    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;

        AssetRecord r;
        size_t a = line.find('|');
        size_t b = line.find('|', a + 1);
        size_t c = line.find('|', b + 1);
        size_t d = line.find('|', c + 1);

        if (a == std::string::npos || b == std::string::npos || c == std::string::npos || d == std::string::npos)
            continue;

        r.guid = line.substr(0, a);
        r.type = (AssetType)std::stoi(line.substr(a + 1, b - (a + 1)));
        r.sourcePath = line.substr(b + 1, c - (b + 1));
        r.metaPath = line.substr(c + 1, d - (c + 1));
        r.libraryDir = line.substr(d + 1);

        // Si el archivo ya no existe, no lo cargamos
        if (!r.sourcePath.empty() && std::filesystem::exists(r.sourcePath)) {
            sAssets[r.guid] = r;
        }
    }
}

void AssetDatabase::Init() {
    EnsureFolders();
    LoadDB();
    std::cout << "[AssetDB] Init. Assets loaded: " << sAssets.size() << "\n";
}

void AssetDatabase::Shutdown() {
    SaveDB();
    std::cout << "[AssetDB] Shutdown\n";
}

const std::unordered_map<std::string, AssetRecord>& AssetDatabase::GetAll() {
    return sAssets;
}

AssetRecord* AssetDatabase::FindByGuid(const std::string& guid) {
    auto it = sAssets.find(guid);
    if (it == sAssets.end()) return nullptr;
    return &it->second;
}

std::string AssetDatabase::MakeUniquePathInAssets(const std::string& filename) {
    std::filesystem::path base = std::filesystem::path(kAssetsFolder) / filename;
    if (!std::filesystem::exists(base)) return base.string();

    // Si existe, añade _1, _2, etc.
    std::filesystem::path stem = base.stem();
    std::filesystem::path ext = base.extension();

    for (int i = 1; i < 10000; ++i) {
        std::filesystem::path candidate = std::filesystem::path(kAssetsFolder) / (stem.string() + "_" + std::to_string(i) + ext.string());
        if (!std::filesystem::exists(candidate)) return candidate.string();
    }

    // fallback (muy raro)
    return base.string();
}

bool AssetDatabase::ImportToLibrary(AssetRecord& rec) {
    // Crea carpeta Library/Imported/<guid>/
    rec.libraryDir = std::string(kImportedFolder) + "/" + rec.guid;
    std::error_code ec;
    std::filesystem::create_directories(rec.libraryDir, ec);
    if (ec) return false;

    // Unity-lite: generamos un archivo “info” como prueba de caché
    std::string infoPath = rec.libraryDir + "/import_info.txt";
    std::ofstream f(infoPath, std::ios::out | std::ios::trunc);
    if (!f.is_open()) return false;

    f << "GUID: " << rec.guid << "\n";
    f << "Source: " << rec.sourcePath << "\n";
    f << "Type: " << (int)rec.type << "\n";
    f.close();

    rec.artifacts.clear();
    rec.artifacts.push_back(infoPath);
    return true;
}

AssetRecord* AssetDatabase::ImportExternalFile(const std::string& externalPath) {
    if (externalPath.empty() || !std::filesystem::exists(externalPath)) {
        std::cout << "[AssetDB] ImportExternalFile: file does not exist\n";
        return nullptr;
    }

    EnsureFolders();

    AssetType type = GuessTypeFromExtension(externalPath);
    if (type == AssetType::Unknown) {
        std::cout << "[AssetDB] Unsupported file type: " << externalPath << "\n";
        return nullptr;
    }

    std::filesystem::path src(externalPath);
    std::string filename = src.filename().string();

    // Copiar a Assets/
    std::string dstPath = MakeUniquePathInAssets(filename);
    std::error_code ec;
    std::filesystem::copy_file(src, dstPath, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        std::cout << "[AssetDB] copy_file failed: " << ec.message() << "\n";
        return nullptr;
    }

    AssetRecord rec;
    rec.guid = GenerateGuid();
    rec.type = type;
    rec.sourcePath = dstPath;
    rec.metaPath = dstPath + ".meta";

    if (!WriteMeta(rec)) {
        std::cout << "[AssetDB] Failed to write meta: " << rec.metaPath << "\n";
        return nullptr;
    }

    if (!ImportToLibrary(rec)) {
        std::cout << "[AssetDB] Failed to import to library\n";
        return nullptr;
    }

    sAssets[rec.guid] = rec;
    SaveDB();

    std::cout << "[AssetDB] Imported: " << rec.sourcePath << " GUID=" << rec.guid << "\n";

    // Si es modelo, lo cargamos (comportamiento cómodo por ahora)
    if (rec.type == AssetType::Model) {
        Renderer::LoadModelFromPath(rec.sourcePath);
        // Como tu sistema usa un solo modelo cargado, registramos el mesh 0 como demo
        SceneManager::RegisterMesh(std::filesystem::path(rec.sourcePath).stem().string(), 0);
    }

    return &sAssets[rec.guid];
}

bool AssetDatabase::DeleteAsset(const std::string& guid, bool force) {
    auto it = sAssets.find(guid);
    if (it == sAssets.end()) return false;

    AssetRecord rec = it->second;

    // (UNITY-LITE) “force” no se usa aún: en el futuro aquí comprobamos si está en uso.
    (void)force;

    // Si borras un modelo, limpiamos el modelo actual (porque tu renderer actual es “single loaded model”)
    if (rec.type == AssetType::Model) {
        Renderer::UnloadLoadedModel();

    }

    std::error_code ec;

    // Borrar source
    if (!rec.sourcePath.empty()) {
        std::filesystem::remove(rec.sourcePath, ec);
        if (ec) std::cout << "[AssetDB] remove source failed: " << ec.message() << "\n";
    }

    // Borrar meta
    ec.clear();
    if (!rec.metaPath.empty()) {
        std::filesystem::remove(rec.metaPath, ec);
        if (ec) std::cout << "[AssetDB] remove meta failed: " << ec.message() << "\n";
    }

    // Borrar Library cache
    ec.clear();
    if (!rec.libraryDir.empty()) {
        std::filesystem::remove_all(rec.libraryDir, ec);
        if (ec) std::cout << "[AssetDB] remove library failed: " << ec.message() << "\n";
    }

    sAssets.erase(it);
    SaveDB();

    std::cout << "[AssetDB] Deleted GUID=" << guid << "\n";
    return true;
}
