#include <IronHull/asset/AssetRegistry.hpp>

namespace IronHull
{
    AssetRegistry& AssetRegistry::get_singleton()
    {
        static AssetRegistry singleton;
        return singleton;
    }

    bool AssetRegistry::has(const std::string& key)
    {
        AssetRegistry& self = AssetRegistry::get_singleton();
        return self.assets.find(key) != self.assets.end();
    }

    void AssetRegistry::unload(const std::string& key)
    {
        AssetRegistry& self = AssetRegistry::get_singleton();

        auto it = self.assets.find(key);
        if (it == self.assets.end()) {
            return;
        }

        it->second.unload(it->second.data);
        self.assets.erase(it);
    }

    void AssetRegistry::dispose()
    {
        AssetRegistry& self = AssetRegistry::get_singleton();

        for (auto& [key, entry] : self.assets) {
            entry.unload(entry.data);
        }

        self.assets.clear();
    }
}
