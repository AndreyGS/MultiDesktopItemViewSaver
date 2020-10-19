#pragma once

#include "../agshelpers/agshelpers.h"

// This class is a singleton. It's instance obtains by DDRegistryExtension::getDDRegExt()
// and when it no more need it's must be closed by DDRegistryExtension::closeDDRegExt()
class DDRegistryExtension
    : public DesktopDisplays
{
public:
    struct IndexRetriveResult {
        LSTATUS status;
        unsigned char index;
        unsigned char subkeysnum;
        bool keyexist;
    };

    DDRegistryExtension(const DDRegistryExtension&) = delete;
    DDRegistryExtension(DDRegistryExtension&&) = delete;
    DDRegistryExtension& operator=(const DDRegistryExtension&) = delete;
    DDRegistryExtension& operator=(DDRegistryExtension&&) = delete;

    inline virtual void refreshData();

    void retriveCurrentProfileIndexFromRegistry();
    LSTATUS saveCurrentDesktopMetricsToRegistry() const;

    inline unsigned char getRegistryIndex() const { return index; }
    inline unsigned char getRegistrySubkeysNum() const { return subkeysnum; }

    // If key is not exist for now, value of index (getRegistryIndex())
    // indicates where potential saving to registry will be done
    inline bool isRegistryKeyExist() const { return keyexist; }

    static DDRegistryExtension* getDDRegExt();
    static void closeDDRegExt();
private:
    inline DDRegistryExtension();
    DDRegistryExtension(int regnum);

    unsigned char index;
    unsigned char subkeysnum;
    bool keyexist;

    static DDRegistryExtension* instance;
    static int usage;
};
